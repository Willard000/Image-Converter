#ifndef IMAGE_H
#define IMAGE_H

#include <memory>
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>

#define TYPE_BMP 0
#define TYPE_PNG 1

class Image;
class BMP;
class PNG;

class ImageReader {
public:
	ImageReader(std::string_view file);

	Image* image();
private:
	std::unique_ptr<Image> _image;
};

class Image {
public:
	std::vector<char> *get_data();

	virtual void read() = 0;
	virtual void save(const char* name) = 0;
	virtual void print_info() = 0;
	virtual int get_type() = 0;

	virtual BMP to_bmp() = 0;
	virtual PNG to_png() = 0;

	std::string _file;
	std::string _file_type;

	int _file_size;

	std::vector<char> _data;
};

// *********************************************************************************************************************************************************************************************************************

class BMP : public Image {  // BITMAPV4HEADER
public:
	struct BitMasks {
		unsigned int _red;
		unsigned int _green;
		unsigned int _blue;
		unsigned int _alpha;
	};

	BMP();

	void read();
	void save(const char* name);
	void print_info();
	int get_type();

	BMP to_bmp();
	PNG to_png();

	short _signature;
	int _file_size;
	int _reserved;
	int _data_offset;

	int _size;
	int _width;
	int _height;
	short _planes;
	short _bits_per_pixel;
	int _compression;
	int _image_size;
	int _x_pixels_per_m;
	int _y_pixels_per_m;
	int _colors_used;
	int _important_colors;

	BitMasks _bit_masks;

	int _lcs_windows_color_space;
	char _ciexyz_endpoints[36] = { 0 };
	int _red_gamma;
	int _green_gamma;
	int _blue_gamma;

	std::vector<char> _pixel_data;
private:
};

// *********************************************************************************************************************************************************************************************************************

class PNG : public Image {
public:
	struct Chunk {
		int _length;
		char _type[4] = { '0', '0', '0', '0' };
		int _crc;
	};

	struct IHDR : public Chunk {
		int _width;
		int _height;
		char _bit_depth;
		char _color_type;
		char _compression;
		char _filter;
		char _interlace;
	};

	struct PLTE : public Chunk {
		char _red;
		char _green;
		char _blue;
	};

	struct IDAT : public Chunk {
		std::vector<char> _pixel_data;
		std::vector<char> _pixel_data_uncompressed;

		size_t _length_uncompressed;

		uint8_t _compression_method;
		uint8_t _compression_info;
		uint8_t _f_check;
		uint8_t _f_dict;
		uint8_t _f_level;
	};

	struct sRGB : public Chunk {
		char _rendering_intent;
	};

	struct gAMA : public Chunk {
		int _gamma;
	};

	struct pHYs : public Chunk {
		unsigned int _pixels_per_unit_x;
		unsigned int _pixels_per_unit_y;
		char _unit_specifier;
	};

	PNG();

	void read();
	void save(const char* name);
	void print_info();
	int get_type();

	BMP to_bmp();
	PNG to_png();

	std::vector<char> raw_pixels();

	char* read_IHDR(int chunk_length, char* ptr);
	char* read_sRGB(int chunk_length, char* ptr);
	char* read_gAMA(int chunk_length, char* ptr);
	char* read_pHYs(int chunk_length, char* ptr);
	char* read_IDAT(int chunk_length, char* ptr);
	char* read_tEXt(int chunk_length, char* ptr);
	char* read_zTXt(int chunk_length, char* ptr);
	char* read_iTXt(int chunk_length, char* ptr);
	char* read_cHRM(int chunk_length, char* ptr);

	long long _signature;

	IHDR _ihdr_chunk;
	IDAT _idat_chunk;

	std::unique_ptr<PLTE> _plte_chunk;
	std::unique_ptr<sRGB> _srgb_chunk;
	std::unique_ptr<gAMA> _gama_chunk;
	std::unique_ptr<pHYs> _phys_chunk;
private:
};

// *********************************************************************************************************************************************************************************************************************


#endif