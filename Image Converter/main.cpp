#include <iostream>

#include "Image.h"

int main() {

	ImageReader image_reader("test.png");

	auto image = image_reader.image();
	image->print_info();

	auto bmp = image->to_bmp();
	bmp.print_info();
	bmp.save("new");

	system("PAUSE");

	return 0;
}