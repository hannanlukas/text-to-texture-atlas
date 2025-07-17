#include "FontToPng.hpp"

#pragma warning(disable: 4996)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void text_to_texture_atlas::character_to_png::character_to_png
(
	const std::string& file_path_n_name,
	const std::vector<unsigned char>& raw_bitmap_buffer,
	const int width,
	const int height
)
{
	stbi_write_png(file_path_n_name.c_str(), width, height, 4, raw_bitmap_buffer.data(), width * 4);
}
