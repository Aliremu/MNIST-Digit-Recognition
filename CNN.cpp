#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <memory>
#include <numeric>
#include <random>
#include <fstream>
#include <iostream>
#include <thread>
#include <future>

#include "Window.h"

typedef std::vector<float_t> vec_t;

using namespace std;

template <typename T>
using Ref = shared_ptr<T>;

float sigmoid_fast(float x) {
	return x / (1 + abs(x));
}

float sigmoid(float x) {
	return 1 / (1 + exp(-x));
}

float df(float x) {
	return sigmoid(x) * (1 - sigmoid(x));
}

vec_t df(const vec_t& y, size_t i) {
	vec_t v(y.size(), 0);
	v[i] = df(y[i]);
	return v;
}

float activation(float x) {
	return sigmoid(x);
}

float dot(vec_t a, vec_t b) {
	if (a.size() != b.size()) { throw std::runtime_error("different sizes"); }

	return inner_product(a.begin(), a.end(), b.begin(), 0.0);
}

float cost(vec_t a, vec_t b) {
	if (a.size() != b.size()) { throw std::runtime_error("different sizes"); }

	vec_t diff;
	diff.resize(a.size());

	transform(a.begin(), a.end(), b.begin(), diff.begin(), [&](float x, float y) {
		return (x - y) * (x - y);
		});

	return accumulate(diff.begin(), diff.end(), 0.0);
}

vec_t error(vec_t a, vec_t b) {
	if (a.size() != b.size()) { throw std::runtime_error("different sizes"); }

	vec_t error;
	error.resize(a.size());

	transform(a.begin(), a.end(), b.begin(), error.begin(), [&](float x, float y) {
		return x - y;
		});
}

float sum(vec_t a) {
	return accumulate(a.begin(), a.end(), 0.0);
}

class Layer {
public:
	Layer* prev;
	Layer* next;

	vec_t values;
	vec_t biases;
	vector<vec_t> weights;

	uint32_t in_size;
	uint32_t out_size;
	uint32_t weight_size;
	uint32_t bias_size;

	Layer(uint32_t in_size, uint32_t out_size, uint32_t weight_size, uint32_t bias_size) : in_size{ in_size }, out_size{ out_size }, weight_size{ weight_size }, bias_size{ bias_size }, prev{ nullptr }, next{ nullptr } {
		values.resize(in_size);
		biases.resize(in_size);
	}

	void init_weights() {
		if (!next) {
			return;
		}
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<> dis(-1, 1);

		weights.resize(out_size);

		for (size_t i = 0; i < out_size; i++) {
			weights[i].resize(in_size);
			for (size_t j = 0; j < in_size; j++) {
				float r = dis(gen);
				weights[i][j] = r;
			}
		}
	}

	void print_weights() {
		if (!next) {
			return;
		}

		for (size_t i = 0; i < in_size; i++) {
			for (size_t j = 0; j < out_size; j++) {
				printf("%f - %d %d", weights[i][j], i, j);
			}
		}
	}

	void add_tail(shared_ptr<Layer> layer) {
		next = layer.get();
		layer->prev = this;
	}

	void forward_propogate() {
		if (!next) return;


		for (size_t i = 0; i < out_size; i++) {
			vec_t w = weights[i];
			float d = dot(w, values) + next->biases[i];
			next->values[i] = activation(d);
		}

		next->forward_propogate();
	}

	void back_propogate(vec_t delta) {
		if (!prev) return;

		vec_t prev_delta;
		prev_delta.resize(prev->in_size);

		for (size_t i = 0; i < in_size; i++) {
			vec_t& w = prev->weights[i];

			for (size_t j = 0; j < w.size(); j++) {
				w[j] = w[j] + (0.1f * delta[i] * prev->values[j]);

				prev_delta[j] = delta[i] * w[j] * df(prev->values[j]);
			}
		}



		prev->back_propogate(prev_delta);
	}
};

class CNN {
public:
	vector<Ref<Layer>> layers;

	void forward_propogate() {
		head()->forward_propogate();
	}

	void back_propogate(vec_t labels) {
		vec_t delta(tail()->in_size);
		for (size_t i = 0; i < tail()->in_size; i++) {
			delta[i] = (labels[i] - tail()->values[i]) * df(tail()->values[i]);
		}

		tail()->back_propogate(delta);
	}

	void init() {
		for (auto& layer : layers) layer->init_weights();
	}

	void input(vec_t input) {
		head()->values = input;
	}

	void print_output() {
		for (float v : tail()->values) {
			printf("%f\n", v);
		}
	}

	vec_t& get_output() {
		return tail()->values;
	}

	void add(shared_ptr<Layer> layer) {
		if (tail()) tail()->add_tail(layer);
		layers.push_back(layer);
	}

	bool empty() const { return layers.size() == 0; }

	Layer* head() const { return empty() ? 0 : layers[0].get(); }

	Layer* tail() const { return empty() ? 0 : layers[layers.size() - 1].get(); }
};

int reverseInt(int i) {
	unsigned char c1, c2, c3, c4;

	c1 = i & 255;
	c2 = (i >> 8) & 255;
	c3 = (i >> 16) & 255;
	c4 = (i >> 24) & 255;

	return ((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4;
}
vector<vec_t> read_images(string path) {
	ifstream file(path, ios::binary);
	if (file.is_open()) {
		int magic_number = 0;
		int number_of_images = 0;
		int n_rows = 0;
		int n_cols = 0;

		file.read((char*)&magic_number, sizeof(magic_number));
		magic_number = reverseInt(magic_number);
		file.read((char*)&number_of_images, sizeof(number_of_images));
		number_of_images = reverseInt(number_of_images);
		file.read((char*)&n_rows, sizeof(n_rows));
		n_rows = reverseInt(n_rows);
		file.read((char*)&n_cols, sizeof(n_cols));
		n_cols = reverseInt(n_cols);

		vector<vec_t> images(number_of_images);

		printf("%d - %d x %d\n", number_of_images, n_cols, n_rows);

		for (int i = 0; i < number_of_images; ++i) {
			for (int r = 0; r < n_rows; ++r) {
				for (int c = 0; c < n_cols; ++c) {
					unsigned char temp = 0;
					file.read((char*)&temp, sizeof(temp));

					float gray = temp / 255.0f;

					images[i].push_back(gray);
					//printf("%s", gray > 0.5f ? "@" : "-");
				}

				//printf("\n");
			}
		}

		return images;
	} else {
		printf("wtf\n");
	}
}

vector<vec_t> read_labels(string path) {
	ifstream file(path, ios::binary);
	if (file.is_open()) {
		int magic_number = 0;
		int number_of_labels = 0;
		
		file.read((char*)&magic_number, sizeof(magic_number));
		magic_number = reverseInt(magic_number);
		file.read((char*)&number_of_labels, sizeof(number_of_labels));
		number_of_labels = reverseInt(number_of_labels);

		vector<vec_t> labels(number_of_labels);

		for (int i = 0; i < number_of_labels; ++i) {
			labels[i].resize(10);

			unsigned char temp = 0;
			file.read((char*)&temp, sizeof(temp));

			labels[i][temp] = 1.0f;
		}

		return labels;
	} else {
		printf("wtf\n");
	}
}

void print_image(vec_t image) {
	for (int r = 0; r < 28; ++r) {
		for (int c = 0; c < 28; ++c) {
			float gray = image[28 * r + c];

			printf("%s", gray > 0.5f ? "@" : "-");
		}

		printf("\n");
	}
}

CNN cnn{};

void update() {
	vector<vec_t> a = get_grid();
	vec_t image(28 * 28);

	for (int y = 0; y < 28; y++) {
		for (int x = 0; x < 28; x++) {
			image[28 * y + x] = (float) a[x][y];
		}
	}

	cnn.input(image);
	cnn.forward_propogate();
	
	vec_t output = cnn.get_output();
	draw_results(output);
}

void do_stuff() {
	vector<vec_t> images = read_images("data/train-images.idx3-ubyte");
	vector<vec_t> labels = read_labels("data/train-labels.idx1-ubyte");

	Ref<Layer> input = make_shared<Layer>(28 * 28, 300, 0, 0);
	//Ref<Layer> hidden1 = make_shared<Layer>(200, 80, 0, 0);
	Ref<Layer> hidden2 = make_shared<Layer>(300, 10, 0, 0);
	Ref<Layer> output = make_shared<Layer>(10, 10, 0, 0);

	cnn.add(input);
	//cnn.add(hidden1);
	cnn.add(hidden2);
	cnn.add(output);
	cnn.init();

	cout << "Training..." << endl;

	for (int i = 0; i < 3000; i++) {
		input->values = images[i];

		cnn.forward_propogate();
		cnn.back_propogate(labels[i]);
	}

	for (int i = 0; i < 3000; i++) {
		input->values = images[i];

		cnn.forward_propogate();
		cnn.back_propogate(labels[i]);
	}

	for (int i = 0; i < 3000; i++) {
		input->values = images[i];

		cnn.forward_propogate();
		cnn.back_propogate(labels[i]);
	}

	cout << "Results!" << endl;

	int correct = 0;

	for (int i = 20000; i < 21000; i++) {
		cnn.input(images[i]);

		cnn.forward_propogate();

		vec_t output = cnn.get_output();

		int guess = std::max_element(output.begin(), output.end()) - output.begin();
		int actual = std::max_element(labels[i].begin(), labels[i].end()) - labels[i].begin();

		if (guess == actual) {
			correct++;
		} else {
			/*for (int j = 0; j < 10; j++) {
				printf("%d - PRE: %f | ACT: %f\n", i, cnn.get_output()[i], labels[i][j]);
			}*/
		}

		printf("%d | %d\n", guess, actual);
	}

	printf("Accuracy: %.3f\n", correct / 1000.0f);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	create_window(hInstance, hPrevInstance, pCmdLine, nCmdShow);
	bind_handler(&update);
	draw_results({ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 });

	thread background(do_stuff);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	background.join();

	return msg.wParam;
}
