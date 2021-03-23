#include "Image.h"

#include <fstream>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <filesystem>

#include <zlib.h>

#define HI_NIBBLE(byte) (((byte) >> 4) & 0x0F)
#define LOW_NIBBLE(byte) ((byte) & 0x0F)

#define Bytes12 255
#define Bytes34 65280
#define Bytes56 16711680
#define Bytes78 4278190080

template<typename T>
void fmt_out(std::string_view str1, T val) {
	std::cout << std::setw(30) << std::left << str1 << std::setw(30) << std::left << val << '\n';
}

void print_status(std::string_view process, int a, int b) {
	std::cout << std::setw(30) << std::left << process << " (" << (float(a) / float(b)) * 100.0f << "%) \n";
}

char* read_bytes(char* ptr, char* data) {
	memcpy(data, ptr, sizeof(char));
	return ptr + sizeof(char);
}

char* read_bytes(char* ptr, short* data) {
	memcpy(data, ptr, sizeof(short));
	return ptr + sizeof(short);
}

char* read_bytes(char* ptr, int* data) {
	memcpy(data, ptr, sizeof(int));
	return ptr + sizeof(int);
}

char* read_bytes(char* ptr, unsigned int* data) {
	memcpy(data, ptr, sizeof(unsigned int));
	return ptr + sizeof(unsigned int);
}

char* read_bytes(char* ptr, long long* data) {
	memcpy(data, ptr, sizeof(long long));
	return ptr + sizeof(long long);
}

std::string change_ext(std::string file, std::string_view ext) {
	return file.substr(0, file.find('.') + 1) + ext.data();
}

std::vector<char> flip_scanlines(std::vector<char> pixels, int bytes_per_pixel, int image_width) {
	const int bytes_per_row = bytes_per_pixel * image_width;
	int i = 0;
	int j = pixels.size() - bytes_per_row;

	std::vector<char> new_pixels;
	new_pixels.resize(pixels.size());

	while (j >= 0) {
		memcpy(&new_pixels[i], &pixels[j], bytes_per_row);

		j -= bytes_per_row;
		i += bytes_per_row;
	}

	return new_pixels;
}

std::string read_file_extension(std::string_view file) {
	return file.substr(file.find('.') + 1).data();
}

ImageReader::ImageReader(std::string_view file) {
	auto ext = read_file_extension(file);

	if(ext == "bmp") {
		_image = std::make_unique<BMP>();
	}
	else if (ext == "png") {
		_image = std::make_unique<PNG>();
	}
	else {
		std::cout << "Cannot read file type -- " << ext << '\n';
		return;
	}

	std::ifstream image_file(file.data(), std::ios::binary);

	if(!image_file) {
		std::cout << "Unable to read file" << '\n';
		return;
	}

	image_file.seekg(0, image_file.end);
	int length = image_file.tellg();
	image_file.seekg(0, image_file.beg);

	_image->get_data()->resize(length);

	image_file.read(&(*_image->get_data())[0], length);

	_image->_file = file;
	_image->_file_type = ext;

	_image->_file_size = std::filesystem::file_size(file);

	_image->read();
}

Image* ImageReader::image() {
	return _image.get();
}

std::vector<char> *Image::get_data() {
	return &_data;
}

// *********************************************************************************************************************************************************************************************************************

BMP::BMP() :
	_signature			( 19778 ),
	_file_size			( 0 ),
	_reserved			( 0 ),
	_data_offset		( 0 ),
	_size				( 0 ),
	_width				( 0 ),
	_height				( 0 ),
	_planes				( 0 ),
	_bits_per_pixel		( 0 ),
	_compression		( 0 ),
	_image_size			( 0 ),
	_x_pixels_per_m		( 0 ),
	_y_pixels_per_m		( 0 ),
	_colors_used		( 0 ),
	_important_colors	( 0 )
{}

void BMP::read() {
	char* ptr = &_data[0];

	print_status("Reading BMP File", 0, _file_size);
	
	ptr = read_bytes(ptr, &_signature);
	ptr = read_bytes(ptr, &_file_size);
	ptr = read_bytes(ptr, &_reserved);
	ptr = read_bytes(ptr, &_data_offset);
	ptr = read_bytes(ptr, &_size);
	ptr = read_bytes(ptr, &_width);
	ptr = read_bytes(ptr, &_height);
	ptr = read_bytes(ptr, &_planes);
	ptr = read_bytes(ptr, &_bits_per_pixel);
	ptr = read_bytes(ptr, &_compression);
	ptr = read_bytes(ptr, &_image_size);
	ptr = read_bytes(ptr, &_x_pixels_per_m);
	ptr = read_bytes(ptr, &_y_pixels_per_m);
	ptr = read_bytes(ptr, &_colors_used);
	ptr = read_bytes(ptr, &_important_colors);

	print_status("Reading BMP File", _size, _file_size);

	_pixel_data.resize(_size);
	std::copy(ptr, ptr + _size, _pixel_data.begin());

	print_status("Reading BMP File", 100, 100);
}

void BMP::save(const char* name) {
	std::string path = name;
	path.append(".bmp");

	std::ofstream file(path.c_str(), std::ios::binary | std::ios::trunc);

	print_status("Saving BMP File", 0, 100);

	file.write((char*)&_signature, sizeof(_signature));
	file.write((char*)&_file_size, sizeof(_file_size));
	file.write((char*)&_reserved, sizeof(_reserved));
	file.write((char*)&_data_offset, sizeof(_data_offset));

	file.write((char*)&_size, sizeof(_size));
	file.write((char*)&_width, sizeof(_width));
	file.write((char*)&_height, sizeof(_height));
	file.write((char*)&_planes, sizeof(_planes));
	file.write((char*)&_bits_per_pixel, sizeof(_bits_per_pixel));
	file.write((char*)&_compression, sizeof(_compression));
	file.write((char*)&_image_size, sizeof(_image_size));
	file.write((char*)&_x_pixels_per_m, sizeof(_x_pixels_per_m));
	file.write((char*)&_y_pixels_per_m, sizeof(_y_pixels_per_m));
	file.write((char*)&_colors_used, sizeof(_colors_used));
	file.write((char*)&_important_colors, sizeof(_important_colors));
	file.write((char*)&_bit_masks._red, sizeof(_bit_masks._red));
	file.write((char*)&_bit_masks._green, sizeof(_bit_masks._green));
	file.write((char*)&_bit_masks._blue, sizeof(_bit_masks._blue));
	file.write((char*)&_bit_masks._alpha, sizeof(_bit_masks._alpha));
	file.write((char*)&_lcs_windows_color_space, sizeof(_lcs_windows_color_space));
	file.write((char*)&_ciexyz_endpoints, 36);
	file.write((char*)&_red_gamma, sizeof(_red_gamma));
	file.write((char*)&_green_gamma, sizeof(_green_gamma));
	file.write((char*)&_blue_gamma, sizeof(_blue_gamma));

	print_status("Saving BMP File", _size, _file_size);

	file.write(&_pixel_data[0], _image_size);

	print_status("Saving BMP File", 100, 100);
}

void BMP::print_info() {
	fmt_out("Originial File", _file);
	fmt_out("View", change_ext(_file, "bmp"));
	fmt_out("File Size", BMP::_file_size);
	fmt_out("BMP Header", _size);
	fmt_out("Width", _width);
	fmt_out("Height", _height);
	fmt_out("Planes", _planes);
	fmt_out("Bits Per Pixel", _bits_per_pixel);
	fmt_out("Compression", _compression);
}

int BMP::get_type() {
	return TYPE_BMP;
}

BMP BMP::to_bmp() {
	return *this;
}

PNG BMP::to_png() {
	PNG png;

	return png;
}

// *********************************************************************************************************************************************************************************************************************

constexpr char IHDR_CHUNK[4] = { 'I', 'H', 'D', 'R' };
constexpr char IDAT_CHUNK[4] = { 'I', 'D', 'A', 'T' };
constexpr char IEND_CHUNK[4] = { 'I', 'E', 'N', 'D' };
constexpr char sRGB_CHUNK[4] = { 's', 'R', 'G', 'B' };
constexpr char gAMA_CHUNK[4] = { 'g', 'A', 'M', 'A' };
constexpr char pHYs_CHUNK[4] = { 'p', 'H', 'Y', 's' };
constexpr char tEXt_CHUNK[4] = { 't', 'E', 'X', 't' };
constexpr char zTXt_CHUNK[4] = { 'z', 'T', 'X', 't' };
constexpr char iTXt_CHUNK[4] = { 'i', 'T', 'X', 't' };
constexpr char cHRM_CHUNK[4] = { 'c', 'H', 'R', 'M' };

bool compare_chunk_type (char* chunk1, const char* chunk2) {
	for (int i = 0; i < 4; ++i) {
		if (chunk1[i] != chunk2[i]) {
			return false;
		}
	}
	return true;
}

PNG::PNG() :
	_signature ( 727905341920923785 )
{}

void PNG::read() {
	char* ptr = &_data[0];

	print_status("Reading PNG File", 0, 100);

	ptr = read_bytes(ptr, &_signature);
	_signature = _byteswap_uint64(_signature);

	int chunk_length = 0;
	char chunk_type[4] = { ' ', ' ', ' ', ' ' };

	do {
		ptr = read_bytes(ptr, &chunk_length);
		chunk_length = _byteswap_ulong(chunk_length);
		memcpy(chunk_type, ptr, 4);
		ptr += 4;
		
		if(compare_chunk_type(chunk_type, IHDR_CHUNK)) {
			ptr = read_IHDR(chunk_length, ptr);
		}

		if(compare_chunk_type(chunk_type, sRGB_CHUNK)) {
			ptr = read_sRGB(chunk_length, ptr);
		}
		
		else if(compare_chunk_type(chunk_type, gAMA_CHUNK)) {
			ptr = read_gAMA(chunk_length, ptr);
		}

		else if(compare_chunk_type(chunk_type, pHYs_CHUNK)) {
			ptr = read_pHYs(chunk_length, ptr);
		}

		else if(compare_chunk_type(chunk_type, IDAT_CHUNK)) {
			ptr = read_IDAT(chunk_length, ptr);
		}
		
		else if(compare_chunk_type(chunk_type, tEXt_CHUNK)) {
			ptr = read_tEXt(chunk_length, ptr);
		}

		else if(compare_chunk_type(chunk_type, zTXt_CHUNK)) {
			ptr = read_zTXt(chunk_length, ptr);
		}

		else if(compare_chunk_type(chunk_type, iTXt_CHUNK)) {
			ptr = read_iTXt(chunk_length, ptr);
		}

		else if (compare_chunk_type(chunk_type, cHRM_CHUNK)) {
			ptr = read_cHRM(chunk_length, ptr);
		}

		else {
			ptr -= 7;
		}

	} while (!compare_chunk_type(chunk_type, IEND_CHUNK)); // end chunk

}

char* PNG::read_IHDR(int chunk_length, char* ptr) {
	print_status("Reading IHDR", 0, 100);

	_ihdr_chunk._length = chunk_length;

	memcpy(_ihdr_chunk._type, IHDR_CHUNK, 4);

	ptr = read_bytes(ptr, &_ihdr_chunk._width);
	_ihdr_chunk._width = _byteswap_ulong(_ihdr_chunk._width);
	ptr = read_bytes(ptr, &_ihdr_chunk._height);
	_ihdr_chunk._height = _byteswap_ulong(_ihdr_chunk._height);
	ptr = read_bytes(ptr, &_ihdr_chunk._bit_depth);
	ptr = read_bytes(ptr, &_ihdr_chunk._color_type);
	ptr = read_bytes(ptr, &_ihdr_chunk._compression);
	ptr = read_bytes(ptr, &_ihdr_chunk._filter);
	ptr = read_bytes(ptr, &_ihdr_chunk._interlace);

	ptr = read_bytes(ptr, &_ihdr_chunk._crc);
	_ihdr_chunk._crc = _byteswap_ulong(_ihdr_chunk._crc);
	
	print_status("Reading IHDR", 100, 100);

	return ptr;
}

char* PNG::read_sRGB(int chunk_length, char* ptr) {
	print_status("Reading sRGB", 0, 100);

	_srgb_chunk = std::make_unique<sRGB>();
	_srgb_chunk->_length = chunk_length;
	memcpy(_srgb_chunk->_type, sRGB_CHUNK, 4);

	ptr = read_bytes(ptr, &_srgb_chunk->_rendering_intent);
	ptr = read_bytes(ptr, &_srgb_chunk->_crc);
	_srgb_chunk->_crc = _byteswap_ulong(_srgb_chunk->_crc);

	print_status("Reading sRGB", 100, 100);

	return ptr;
}

char* PNG::read_gAMA(int chunk_length, char* ptr) {
	print_status("Reading gAMA", 0, 100);

	_gama_chunk = std::make_unique<gAMA>();
	_gama_chunk->_length = chunk_length;
	memcpy(_gama_chunk->_type, gAMA_CHUNK, 4);

	ptr = read_bytes(ptr, &_gama_chunk->_gamma);
	_gama_chunk->_gamma = _byteswap_ulong(_gama_chunk->_gamma);
	ptr = read_bytes(ptr, &_gama_chunk->_crc);
	_gama_chunk->_crc = _byteswap_ulong(_gama_chunk->_crc);

	print_status("Reading gAMA", 100, 100);

	return ptr;
}

char* PNG::read_pHYs(int chunk_length, char* ptr) {
	print_status("Reading pHYs", 0, 100);

	_phys_chunk = std::make_unique<pHYs>();
	_phys_chunk->_length = chunk_length;
	memcpy(_phys_chunk->_type, pHYs_CHUNK, 4);

	ptr = read_bytes(ptr, &_phys_chunk->_pixels_per_unit_x);
	_phys_chunk->_pixels_per_unit_x = _byteswap_ulong(_phys_chunk->_pixels_per_unit_x);
	ptr = read_bytes(ptr, &_phys_chunk->_pixels_per_unit_y);
	_phys_chunk->_pixels_per_unit_y = _byteswap_ulong(_phys_chunk->_pixels_per_unit_y);
	ptr = read_bytes(ptr, &_phys_chunk->_unit_specifier);
	ptr = read_bytes(ptr, &_phys_chunk->_crc);
	_phys_chunk->_crc = _byteswap_ulong(_phys_chunk->_crc);

	print_status("Reading pHYs", 100, 100);

	return ptr;
}

char* PNG::read_IDAT(int chunk_length, char* ptr) {
	print_status("Reading IDAT", 0, 100);

	_idat_chunk._length = chunk_length;
	memcpy(_idat_chunk._type, IDAT_CHUNK, 4);

	// Read all IDAT chunks and add compressed data to _pixel_data
	char next_chunk[4] = { ' ', ' ', ' ', ' ' };
	do {
		auto length = _idat_chunk._pixel_data.size();
		_idat_chunk._pixel_data.resize(_idat_chunk._pixel_data.size() + chunk_length);
		memcpy(&_idat_chunk._pixel_data[length], ptr, chunk_length);
		ptr += chunk_length + 4; // + crc

		ptr = read_bytes(ptr, &chunk_length);
		chunk_length = _byteswap_ulong(chunk_length);
		memcpy(next_chunk, ptr, 4);
		ptr += 4;

	} while (compare_chunk_type(next_chunk, IDAT_CHUNK));

	ptr -= 8;

	print_status("Reading IDAT", 50, 100);
	print_status("Decompressing", 0, 100);

	const uint8_t first_byte = _idat_chunk._pixel_data[0];
	const uint8_t second_byte = _idat_chunk._pixel_data[1];

	_idat_chunk._compression_method = LOW_NIBBLE(first_byte);
	_idat_chunk._compression_info = HI_NIBBLE(first_byte);

	_idat_chunk._f_check = ((second_byte) & 0x1F);
	_idat_chunk._f_dict = ((HI_NIBBLE(second_byte)) & 0x02);
	_idat_chunk._f_level = ((HI_NIBBLE(second_byte)) >> 2);

	float f_check_value = ((first_byte * 256 + second_byte) / 31.0f);
	if(f_check_value != (int)f_check_value) { // must be multiple of 31
		assert(0);
	}

	if(_idat_chunk._f_dict == 1) {
		assert(0); // idk how to read this
	}


	if(_ihdr_chunk._color_type == 6) {
		_idat_chunk._length_uncompressed = ((_ihdr_chunk._width * _ihdr_chunk._height) * 4) + _ihdr_chunk._height;
	}

	_idat_chunk._pixel_data_uncompressed.resize(_idat_chunk._length_uncompressed);

	z_stream stream;

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.avail_in = _idat_chunk._pixel_data.size();
	stream.next_in = (Bytef*)&_idat_chunk._pixel_data[0];
	stream.avail_out = _idat_chunk._length_uncompressed;
	stream.next_out = (Bytef*)&_idat_chunk._pixel_data_uncompressed[0];

	int ret = inflateInit(&stream);
	assert(ret == Z_OK);

	ret = inflate(&stream, Z_NO_FLUSH);

	ret = inflateEnd(&stream);

	print_status("Decompressing", 100, 100);

	print_status("Reading IDAT", 100, 100);

	return ptr;
}

char *PNG::read_tEXt(int chunk_length, char* ptr) {
	print_status("Reading tEXt", 0, 100);

	char *start_ptr = ptr;

	std::string keyword = ptr;
	ptr += keyword.size() + 1;

	std::string text;
	text.resize(chunk_length - (keyword.size() + 1));
	std::copy(ptr, ptr + text.size(), text.begin());

	ptr += text.size() + 4;

	print_status("Reading tEXt", 100, 100);

	return ptr;
}

char *PNG::read_zTXt(int chunk_length, char* ptr) {
	print_status("Reading zTXt", 0, 100);

	ptr += chunk_length + 4;

	print_status("Reading zTXt", 100, 100);

	return ptr;
}

char *PNG::read_iTXt(int chunk_length, char* ptr) {
	print_status("Reading iTXt", 0, 100);

	ptr += chunk_length + 4;

	print_status("Reading iTXt", 100, 100);

	return ptr;
}

char *PNG::read_cHRM(int chunk_length, char* ptr) {
	print_status("Reading cHRM", 0, 100);

	ptr += chunk_length + 4;

	print_status("Reading cHRM", 100, 100);

	return ptr;
}

void PNG::save(const char* name) {

}

char paeth_filter(char a, char b, char c) { // a = left pixel, b = up pixel, c = up left pixel
	const short _a = uint8_t(a);
	const short _b = uint8_t(b);
	const short _c = uint8_t(c);

	const short p = _a + _b - _c;
	const short pa = abs(p - _a);
	const short pb = abs(p - _b);
	const short pc = abs(p - _c);

	if		(pa <= pb && pa <= pc)   return a;
	else if (pb <= pc)			     return b;
	else							 return c;
}

char average_filter(char a, char b) { // a = left pixel, b = up pixel
	const short _a = uint8_t(a);
	const short _b = uint8_t(b);

	return (_a + _b) / 2;
}

std::vector<char> PNG::raw_pixels() {
	std::vector<char> pixels;
	pixels.resize(_idat_chunk._length_uncompressed - _ihdr_chunk._height);

	const int bytes_per_pixel = 4;
	const int bytes_per_row = bytes_per_pixel * _ihdr_chunk._width;
	char* start_ptr = &_idat_chunk._pixel_data_uncompressed[0];
	char* end_ptr = &_idat_chunk._pixel_data_uncompressed[0] + _idat_chunk._length_uncompressed;

	char* ptr = start_ptr;

	for (int i = 0; ptr <= end_ptr; i += bytes_per_row) {

		char filter_method;
		memcpy(&filter_method, ptr, 1);

		if (filter_method == 0) { // no filter
			memcpy(&pixels[i], ptr + 1, bytes_per_row);
		}
		else if (filter_method == 1) { // sub filter 
			for (int byte = 0; byte < bytes_per_row; ++byte) {
				if (byte < 4) {
					pixels[i + byte] = *(ptr + byte + 1);
				}
				else {
					pixels[i + byte] = pixels[i + byte - 4] + *(ptr + byte + 1);
				}
			}
		}
		else if (filter_method == 2) { // up filter
			for (int byte = 0; byte < bytes_per_row; ++byte) {
				pixels[i + byte] = pixels[i + byte - bytes_per_row] + *(ptr + byte + 1);
			}
		}
		else if(filter_method == 3) { // average filter
			for (int byte = 0; byte < bytes_per_row; ++byte) {
				if(byte < 4) {
					pixels[i + byte] = average_filter(char(0), pixels[i + byte - bytes_per_row]) + *(ptr + byte + 1);
				}
				else {
					pixels[i + byte] = average_filter(pixels[i + byte - 4], pixels[i + byte - bytes_per_row]) + *(ptr + byte + 1);
 				}
			}
		}
		else if(filter_method == 4) { // Paeth filter
			for (int byte = 0; byte < bytes_per_row; ++byte) {
				if (byte < 4) {
					pixels[i + byte] = paeth_filter(char(0), pixels[i + byte - bytes_per_row], char(0)) + *(ptr + byte + 1);
				}
				else {
					pixels[i + byte] = paeth_filter(pixels[i + byte - 4], pixels[i + byte - bytes_per_row], pixels[i + byte - bytes_per_row - 4]) + *(ptr + byte + 1);
				}
			}
		}

		ptr += bytes_per_row + 1;

		print_status("Defiltering", i, pixels.size());
	}

	/*
	std::ofstream test_file("raw_uncompressed.x", std::ios::binary | std::ios::trunc);

	test_file.write(&_idat_chunk._pixel_data_uncompressed[0], _idat_chunk._length_uncompressed);

	test_file.close();
	*/

	return pixels;
}

void PNG::print_info() {
	fmt_out("Orignial File", _file);
	fmt_out("View", change_ext(_file, "png"));
	fmt_out("File Size", _file_size);
	fmt_out("Width", _ihdr_chunk._width);
	fmt_out("Height", _ihdr_chunk._height);
	fmt_out("Bit Depth", (int)_ihdr_chunk._bit_depth);
	fmt_out("Color Type", (int)_ihdr_chunk._color_type);
	fmt_out("Compression", (int)_ihdr_chunk._compression);
	fmt_out("Filter", (int)_ihdr_chunk._filter);
	fmt_out("Interlace", (int)_ihdr_chunk._interlace);
}

int PNG::get_type() {
	return TYPE_PNG;
}

BMP PNG::to_bmp() {
	BMP bmp;
	
	bmp._file = _file;
	bmp._file_size = -1;
	bmp._file_type = "bmp";

	bmp._data_offset = 122;
	bmp._size = 108;
	bmp._width = _ihdr_chunk._width;
	bmp._height = _ihdr_chunk._height;
	bmp._planes = 1;
	bmp._bits_per_pixel = 32;
	bmp._compression = 3;
	bmp._x_pixels_per_m = _phys_chunk ? _phys_chunk->_pixels_per_unit_x : 0;
	bmp._y_pixels_per_m = _phys_chunk ? _phys_chunk->_pixels_per_unit_y : 0;
	bmp._colors_used = 0;
	bmp._important_colors = 0;
	bmp._lcs_windows_color_space = 0;
	bmp._red_gamma = 0;
	bmp._green_gamma = 0;
	bmp._blue_gamma = 0;

	bmp._bit_masks._red = Bytes12;
	bmp._bit_masks._green = Bytes34;
	bmp._bit_masks._blue = Bytes56;
	bmp._bit_masks._alpha = Bytes78;

	bmp._pixel_data = flip_scanlines(raw_pixels(), 4, bmp._width);
	bmp._image_size = bmp._pixel_data.size();
	bmp._file_size = 122 + bmp._image_size;

	_file_size = bmp._file_size;

	return bmp;
}

PNG PNG::to_png() {
	PNG png;
	return png;
}

// *********************************************************************************************************************************************************************************************************************