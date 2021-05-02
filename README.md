# Image-Converter

Simple image file conveter  
  
Reads png data from a file, decompresses and defilters pixel data into a bmp file format
  
Supports Truecolor RGBA PNGS

# Table of Contencts

[Libraries](#libraries)  
[How it works](#how-it-works)
  - [Image Class](#image-class)
  - [Reading Images](#reading-images)
  - [Compression](#compression)
  - [Filtering](#filtering)
  - [Conversion](#conversion)
  - [Result](#result)

[End Note](#end-note)

## Libraries
[zlib](https://zlib.net/) DEFLATE Compression library.

## How it works

### Image Class

```C++ 
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
```

Base image class, which in theory would read any image format and be able to convert to another image format. For example to_bmp() is called on a PNG image and returns the image as a BMP.

### Reading Images  

```C++ 
void BMP::read() {
	char* ptr = &_data[0];
	
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

	_pixel_data.resize(_size);
	std::copy(ptr, ptr + _size, _pixel_data.begin());

}
```

Interprets the data block read from a file according the the image format specifications. In this case a BMP file is as simple as copying bytes from addresses.

#### Compression

Some images use compression, so for these cases we first must uncompress the image data. For PNG files DEFLATE is the compression algorithm used, and zlib provides a libaray for compressing and uncompressing DEFALTE.

```C++

  // Read all IDAT chunks and add compressed data to _pixel_data

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

  // Uncompress pixel data
  
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
```

#### Filtering

After decompression we may get filtered image data. For PNG files we need to unfilter this data by reversing the filtering functions.
Filtering specifications for PNG Files can be found [here](https://www.w3.org/TR/2003/REC-PNG-20031110/#9FtIntro).

```C++
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
	}
  
  ```
  
### Conversion

Now that we have the raw pixel data after decompressing, and defiltering, we need to interpret the PNG data as a BMP.
This step is about matching as the data we know about our PNG to the BMP file format.  
In this case we need to specify a 32 bits per pixel BMP and flip the scanlines from our PNG raw pixel data in reverse order.

```C++
BMP PNG::to_bmp() {
	BMP bmp;
	
	bmp._file = _file;
	bmp._file_size = -1;
	bmp._file_type = "bmp";

	bmp._data_offset = 122;
	bmp._size = 108;                      //  BITMAPV4HEADER <- type of BMP we use for RGBA
	bmp._width = _ihdr_chunk._width;      //  same as png
	bmp._height = _ihdr_chunk._height;    //  same as png
	bmp._planes = 1;                      //  must be 1
	bmp._bits_per_pixel = 32;             //  RGBA PNG (8 bits per channel == 32 bits per pixel)
	bmp._compression = 3;                 //  BI_BITFIELDS <- 32 bits per pixel
	bmp._x_pixels_per_m = _phys_chunk ? _phys_chunk->_pixels_per_unit_x : 0;  // same as png
	bmp._y_pixels_per_m = _phys_chunk ? _phys_chunk->_pixels_per_unit_y : 0;  // same as png
	bmp._colors_used = 0;                 // not concerned with the rest
	bmp._important_colors = 0;
	bmp._lcs_windows_color_space = 0;
	bmp._red_gamma = 0;
	bmp._green_gamma = 0;
	bmp._blue_gamma = 0;

	bmp._bit_masks._red = Bytes12;    // 0x000000FF
	bmp._bit_masks._green = Bytes34;  // 0x0000FF00
	bmp._bit_masks._blue = Bytes56;   // 0x00FF0000
	bmp._bit_masks._alpha = Bytes78;  // 0xFF000000

	bmp._pixel_data = flip_scanlines(raw_pixels(), 4, bmp._width);  // raw pixel data from png fliped
	bmp._image_size = bmp._pixel_data.size();
	bmp._file_size = 122 + bmp._image_size; 

	_file_size = bmp._file_size;

	return bmp;
}
```

#### Result

Now all we need to do is read an image file, cast it to our desired format, and save it.

```C++
  ImageReader image_reader("test.png");

	auto image = image_reader.image();
	image->print_info();

	auto bmp = image->to_bmp();
	bmp.print_info();
	bmp.save("new");
  
```
  
### End Note
This project was to learn about reading binary data from files and more a proof of concept than a finished product. This code only works for RGBA PNGs as other PNG types will certainly result in an error. Because there are a lot of different types between the file formats more work would need to be done to cleanly deal with those types. I was mainly interested in binary data, compression, and filtering. There is room for optimization when unfiltering or passing pixel data.
