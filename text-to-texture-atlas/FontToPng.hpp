#pragma once
#include "Font.hpp"
#include "Font.hpp"

namespace text_to_texture_atlas::character_to_png
{
	// Note: Must be 4 channel.
	void character_to_png(
		const std::string& file_path_n_name,
		const std::vector<unsigned char>& raw_bitmap_buffer,
		int width,
		int height
	);

}