#include <SFML/Graphics.hpp>
#include <iostream>
#include <array>
#include <vector>
#include <thread>	

static constexpr int IMAGE_WIDTH = 960;
static constexpr int IMAGE_HEIGHT = 540;

class Mandelbrot {
public:
	Mandelbrot();
	void UpdateImage(double zoom, double offset_X, double offset_Y, sf::Image& image) const;
private:
	static const int kMaxIterations = 1000; // maximum number of iterations for ComputeMandelbrot()
	std::array<sf::Color, kMaxIterations + 1> colors;
	int ComputeMandelbrot(double c_real, double c_imag) const;
	sf::Color GetColor(int iterations) const;
	void UpdateImageSlice(double zoom, double offset_X, double offset_Y, sf::Image& image, int minY, int maxY) const;
};

Mandelbrot::Mandelbrot() {
	for (int i = 0; i <= kMaxIterations; ++i) {
		colors[i] = GetColor(i);
	}
}

int Mandelbrot::ComputeMandelbrot(double c_real, double c_imag) const {
  double z_real = c_real;
  double z_imag = c_imag;
  for (int counter = 0; counter < kMaxIterations; ++counter) {
    double r2 = z_real * z_real;
    double i2 = z_imag * z_imag;
    if (r2 + i2 > 4.0) {
      return counter;
    }
    z_imag = 2.0 * z_real * z_imag + c_imag;
    z_real = r2 - i2 + c_real;
  }
  return kMaxIterations;
}

sf::Color Mandelbrot::GetColor(int iterations) const {
	/*
	To obtain a smooth transition from one color to another, we need to use
	three smooth, continuous functions that will map every number t.
	A slightly modified version of the Bernstein polynomials will do, as they are
	continuous, smooth and have values in the [0, 1) interval.
	Therefore, mapping the results to the range for r, g, b is as easy as multiplying
	each value by 255.
	*/
	int r, g, b;
	double t = (double)iterations / (double)kMaxIterations;
	r = (int)(9 * (1 - t)*t*t*t * 255);
	g = (int)(15 * (1 - t)*(1 - t)*t*t * 255);
	b = (int)(8.5*(1 - t)*(1 - t)*(1 - t)*t * 255);
	return sf::Color(r, g, b);
}

void Mandelbrot::UpdateImageSlice(double zoom, double offset_X, double offset_Y, sf::Image& image, int minY, int maxY) const
{
	double c_real = 0 * zoom - IMAGE_WIDTH / 2.0 * zoom + offset_X;
	double c_imag_start = minY * zoom - IMAGE_HEIGHT / 2.0 * zoom + offset_Y;
	for (int x = 0; x < IMAGE_WIDTH; x++, c_real += zoom) {
		double c_imag = c_imag_start;
		for (int y = minY; y < maxY; y++, c_imag += zoom) {
			int value = ComputeMandelbrot(c_real, c_imag);
			image.setPixel(x, y, colors[value]);
		}
	}
}

void Mandelbrot::UpdateImage(double zoom, double offset_X, double offset_Y, sf::Image& image) const
{
	std::vector<std::thread> threads;
	const int STEP = IMAGE_HEIGHT / std::thread::hardware_concurrency();
	for (int i = 0; i < IMAGE_HEIGHT; i += STEP) {
		threads.push_back(std::thread(&Mandelbrot::UpdateImageSlice, *this, zoom, offset_X, offset_Y, std::ref(image), i, std::min(i + STEP, IMAGE_HEIGHT)));
	}
	for (auto &t : threads) {
		t.join();
	}
}

int main() {
	double offset_X = -0.7; // and move around
	double offset_Y = 0.0;
	double zoom = 0.004; // allow the user to zoom in and out...
	double zoom_factor = 0.9;
	Mandelbrot mb;
	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;

	sf::RenderWindow window(sf::VideoMode(IMAGE_WIDTH, IMAGE_HEIGHT), "Mandelbrot", sf::Style::Default, settings);
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);

	sf::Image image;
	image.create(IMAGE_WIDTH, IMAGE_HEIGHT, sf::Color(0, 0, 0));
	sf::Texture texture;
	sf::Sprite sprite;

	bool stateChanged = true; // track whether the image needs to be regenerated

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
				stateChanged = true; // image needs to be recreated when the user changes zoom or offset
				switch (event.key.code) {
				case sf::Keyboard::Escape:
					window.close();
					break;
				case sf::Keyboard::Equal:
					zoom *= zoom_factor;
					break;
				case sf::Keyboard::Dash:
					zoom /= zoom_factor;
					break;
				case sf::Keyboard::W:
					offset_Y -= 40 * zoom;
					break;
				case sf::Keyboard::S:
					offset_Y += 40 * zoom;
					break;
				case sf::Keyboard::A:
					offset_X -= 40 * zoom;
					break;
				case sf::Keyboard::D:
					offset_X += 40 * zoom;
					break;
				default:
					stateChanged = false;
					break;
				}
			case sf::Event::MouseWheelScrolled:
				if (event.mouseWheelScroll.delta == 1) {
					stateChanged = true;
					zoom *= zoom_factor;
				}
				if (event.mouseWheelScroll.delta == -1) {
					stateChanged = true;
					zoom /= zoom_factor;
				}
				break;

			default:
				break;
			}
		}

		if (stateChanged) {
			mb.UpdateImage(zoom, offset_X, offset_Y, image);
			texture.loadFromImage(image);
			sprite.setTexture(texture);
			stateChanged = false;
		}
		window.draw(sprite);
		window.display();
	}
}