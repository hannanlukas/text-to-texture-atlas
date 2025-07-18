#include "Font.hpp"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <utility>

#include "texture-operations/texture_operations.h"

#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>

//#define DEBUGGING

#pragma region character::output_raw
void text_to_texture_atlas::Font::character::output_raw() const
{
	for (unsigned int y = 0; y < height_; y++)
	{
		for (unsigned int x = 0; x < width_; x++)
		{
			const auto flat{ (y * width_ + x) * 4 };
			const auto r = raw_bitmap_buffer[flat + 0];
			const auto g = raw_bitmap_buffer[flat + 1];
			const auto b = raw_bitmap_buffer[flat + 2];
			const auto a = raw_bitmap_buffer[flat + 3];

			if (r == 255) { std::cout << "r"; }
			//else { std::cout << " "; }
			
			if (g == 255) { std::cout << "g"; }
			//else { std::cout << " "; }
			if (b == 255) { std::cout << "b"; }
			if (a != 0)
			{
				std::cout << static_cast<int>(a);
			}
			else
			{
				std::cout << " ";
			}

			//else { std::cout << " "; }
			
		}
		std::cout << "\n";
	}
}
#pragma endregion

#pragma region init_library_
bool text_to_texture_atlas::Font::init_library()
{
	ft_error_ = FT_Init_FreeType(&library_);
	if (ft_error_)
	{
		return false;
	}
	return true;
}
#pragma endregion

#pragma region init_face_
bool text_to_texture_atlas::Font::init_face()
{
	std::string font = windows_fonts_paths_ + selected_font_;
	ft_error_ = FT_New_Face(library_, font.c_str(), 0, &face_);
	if (ft_error_)
	{
		return false;
	}
	return true;
}
#pragma endregion

#pragma region init_char_size
bool text_to_texture_atlas::Font::init_char_size()
{
	ft_error_ = FT_Set_Char_Size
	(
		face_,
		0,
		char_pt_size_,
		char_width_dpi_,
		char_height_dpi_
	);

	if (ft_error_)
	{
		return false;
	}
	return true;
}
#pragma endregion

#pragma region init_pixel_size
bool text_to_texture_atlas::Font::init_pixel_size()
{
	ft_error_ = FT_Set_Pixel_Sizes
	(
		face_,
		char_width_px_,
		char_height_px_
	);
	if (ft_error_)
	{
		return false;
	}
	return true;
}
#pragma endregion

#pragma region init_character_map
bool text_to_texture_atlas::Font::init_character_map()
{

	FT_UInt glyph_index{};

	character_map_.reserve(char_range_max - char_range_min + 1);

	for (int i = char_range_min; i <= char_range_max; i++)
	{
		//init_char_size();

		glyph_index = FT_Get_Char_Index(face_, i);
		ft_error_ = FT_Load_Glyph(face_, glyph_index, FT_LOAD_DEFAULT);
		if (ft_error_)
		{
			std::cout << "error loading glyph!\n";
			continue; 
		}

		ft_error_ = FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_NORMAL);
		if (ft_error_)
		{
			std::cout << "error rendering glyph!\n";
			continue;
		}

		auto& bitmap{ face_->glyph->bitmap };
		if (!bitmap.buffer && !isspace(static_cast<char>(i)))
		{
			continue;
		}
		size_t flat_size{static_cast<size_t>(bitmap.rows * bitmap.width * 4)};

		character_map_[static_cast<char>(i)] = character{};
		character& current_character = character_map_[static_cast<char>(i)];

		current_character.height_ = bitmap.rows;
		current_character.width_ = bitmap.width;
		current_character.x_bearing_ = face_->glyph->bitmap_left;
		current_character.y_bearing_ = face_->glyph->bitmap_top;
		current_character.advance_x_ = face_->glyph->advance.x;
		current_character.advance_y_ = face_->glyph->advance.y;

		current_character.raw_bitmap_buffer = std::vector<unsigned char>(flat_size);

		if (isspace(static_cast<char>(i)))
		{
			continue;
		}

		if (!convert_bitmap_to_four_channel_buffer(current_character.raw_bitmap_buffer, bitmap.width, bitmap.rows))
		{
			std::cout << "error converting bitmap to vector\n";
			return false;
		}

	}
	return true;
}
#pragma endregion

#pragma region convert_bitmap_to_vector
bool text_to_texture_atlas::Font::convert_bitmap_to_four_channel_buffer
(
	std::vector<unsigned char>& dst_vector,
	unsigned int bitmap_width,
	unsigned int bitmap_height
) const
{
	auto& bitmap{ face_->glyph->bitmap };
	if (!bitmap.buffer) { return false; }
	
	for (unsigned int y = 0; y < bitmap_height; y++)
	{
		for (unsigned int x = 0; x < bitmap_width; x++)
		{
			auto flat{ (y * bitmap_width + x) };
			auto current{ bitmap.buffer[flat] };
			auto dst_flat{ flat * 4 };
			if (current == '\0')
			{
				dst_vector[dst_flat] = 0;
				dst_vector[dst_flat + 1] = 0;
				dst_vector[dst_flat + 2] = 0;
				dst_vector[dst_flat + 3] = 0;
				continue;
			}
			dst_vector[dst_flat] = 0;
			dst_vector[dst_flat + 1] = 0;
			dst_vector[dst_flat + 2] = 0;
			dst_vector[dst_flat + 3] = current;
		}
	}

	return true;
	
}
#pragma endregion

#pragma region get_total_buffer_size
size_t text_to_texture_atlas::Font::get_total_buffer_size() const
{
	size_t total_size{};
	for (const auto& val : character_map_ | std::views::values)
	{
		total_size += val.raw_bitmap_buffer.size();
	}
	return total_size;
}
#pragma endregion

#pragma region get_max_character_width
unsigned int text_to_texture_atlas::Font::get_max_character_width() const
{
	unsigned int max_size{};
	for (const auto& val : character_map_ | std::views::values)
	{
		max_size = std::max(val.width_, max_size);
	}
	return max_size;

}
#pragma endregion

#pragma region get_max_character_height
unsigned int text_to_texture_atlas::Font::get_max_character_height() const
{
	unsigned int max_size{};
	for (const auto& val : character_map_ | std::views::values)
	{
		max_size = std::max(val.height_, max_size);
	}
	return max_size;

}
#pragma endregion

#pragma region constructors
text_to_texture_atlas::Font::Font
(
	std::string font_name,
	const signed long char_pt_size,
	const unsigned int char_width_dpi,
	const unsigned int char_height_dpi
)
	: selected_font_(std::move(font_name)),
	char_pt_size_(char_pt_size),
	char_width_dpi_(char_width_dpi),
	char_height_dpi_(char_height_dpi)
{
	const auto logger = spdlog::stdout_color_mt("console");

	if (!error_)
	{
		if (!init_library())
		{
			SPDLOG_LOGGER_ERROR(logger, "Error initializing freetype library");
			error_ = true;
		}
	}
	if (!error_)
	{
		if (!init_face())
		{
			SPDLOG_LOGGER_ERROR(logger, "Error initializing freetype face");
			error_ = true;
		}
	}
	if (!error_)
	{
		if (!init_char_size())
		{
			SPDLOG_LOGGER_ERROR(logger, "Error initializing freetype character size");
			error_ = true;
		}
	}
	if (!error_)
	{

		if (!init_character_map())
		{
			SPDLOG_LOGGER_ERROR(logger, "Error initializing character map");
			error_ = true;
		}
	}
	if (!error_)
	{
		if (!init_main_atlas_buffer())
		{
			SPDLOG_LOGGER_ERROR(logger, "Error initializing atlas buffer");
			error_ = true;
		}
	}

}

text_to_texture_atlas::Font::Font(
	std::string font_name,
	const unsigned int char_height,
	const unsigned int char_width
)
	: selected_font_(std::move(font_name)),
		char_width_px_(char_width),
		char_height_px_(char_height)
{
	const auto logger = spdlog::stdout_color_mt("console");
	if (!error_)
	{
		if (!init_library())
		{
			SPDLOG_LOGGER_ERROR(logger, "Error initializing freetype library");
			error_ = true;
		}
	}
	if (!error_)
	{
		if (!init_face())
		{
			SPDLOG_LOGGER_ERROR(logger, "Error initializing freetype face");
			error_ = true;
		}
	}
	if (!error_)
	{
		if (!init_pixel_size())
		{
			SPDLOG_LOGGER_ERROR(logger, "Error initializing freetype pixel size");
			error_ = true;
		}
	}
	if (!error_)
	{

		if (!init_character_map())
		{
			SPDLOG_LOGGER_ERROR(logger, "Error initializing character map");
			error_ = true;
		}
	}
	if (!error_)
	{
		if (!init_main_atlas_buffer())
		{
			SPDLOG_LOGGER_ERROR(logger, "Error initializing atlas buffer");
			error_ = true;
		}
	}
}

text_to_texture_atlas::Font text_to_texture_atlas::Font::Font_Pt
(const std::string& font_name, signed long char_pt_size, unsigned int char_width_dpi, unsigned int char_height_dpi)
{
	return Font{ font_name, char_pt_size, char_width_dpi, char_height_dpi };
}

text_to_texture_atlas::Font text_to_texture_atlas::Font::Font_Px
(const std::string& font_name, unsigned int char_height, unsigned int char_width)
{
	return Font{ font_name, char_height, char_width };
}
#pragma endregion

#pragma region free_character_buffers
void text_to_texture_atlas::Font::free_character_buffers()
{
	for (auto& val : character_map_ | std::views::values)
	{
		val.raw_bitmap_buffer.clear();
	}
}
#pragma endregion

#pragma region free_atlas_buffer
void text_to_texture_atlas::Font::free_atlas_buffer()
{
	main_atlas_.atlas_buffer.clear();
}
#pragma endregion

#pragma region get_character
text_to_texture_atlas::Font::character& text_to_texture_atlas::Font::get_character
(
	char character_
)
{
	return character_map_[character_];
}
#pragma endregion

#pragma region get_main_atlas
text_to_texture_atlas::Font::atlas& text_to_texture_atlas::Font::get_main_atlas()
{
	return main_atlas_;
}
#pragma endregion

#pragma region init_main_atlas_buffer
bool text_to_texture_atlas::Font::init_main_atlas_buffer()
{

	unsigned int max_character_height{ get_max_character_height() };
	unsigned int max_character_width{ get_max_character_width() };
	int character_range{ char_range_max - char_range_min };
	unsigned int characters_per_w_and_h{static_cast<unsigned int>(std::ceil(sqrt(character_range)))};
	unsigned int width_spacing = 5;
	unsigned int height_spacing = 5;
	double total_width_spacing = width_spacing * characters_per_w_and_h;
	double total_height_spacing = height_spacing * characters_per_w_and_h;

	unsigned int initial_width_spacing = 5; // Where it should start. so it's not starting at (0,0)
	unsigned int initial_height_spacing = 5;

	unsigned int total_buffer_width = static_cast<unsigned int>(characters_per_w_and_h * max_character_width + total_width_spacing + (initial_width_spacing * 2));
	unsigned int total_buffer_height = static_cast<unsigned int>(characters_per_w_and_h * max_character_height + total_height_spacing + (initial_height_spacing * 2));
	size_t total_buffer_size = static_cast<size_t>(total_buffer_width * total_buffer_height);

	unsigned int x_position{initial_width_spacing};
	unsigned int y_position{initial_height_spacing};

	int character_channels{ 4 };
	int atlas_buffer_channels{ 4 };
	int character_stride = sizeof(unsigned char) * 4;
	int atlas_buffer_stride = sizeof(unsigned char) * 4;

	unsigned int increment_x_size{ max_character_width + width_spacing };
	unsigned int increment_y_size{ max_character_height + height_spacing };


	main_atlas_.atlas_buffer = std::vector<unsigned char>(total_buffer_size * character_channels);
	main_atlas_.width = total_buffer_width;
	main_atlas_.height = total_buffer_height;


	int index{};
	for (auto& a : character_map_)
	{
		if (std::isspace(a.first))
		{
			continue;
		}


		if (x_position + a.second.width_ > total_buffer_width)
		{
			x_position = initial_width_spacing;
			y_position += increment_y_size;
		}

		

		auto er = texture_operations::blit_texture
		(
			static_cast<int>(x_position),
			static_cast<int>(y_position),
			a.second.width_,
			a.second.height_,
			character_channels,
			static_cast<int>(total_buffer_width),
			static_cast<int>(total_buffer_height),
			atlas_buffer_channels,
			a.second.raw_bitmap_buffer.data(),
			main_atlas_.atlas_buffer.data(),
			character_stride,
			atlas_buffer_stride
		);

		if (er != texture_operations::SUCCESS)
		{
			unsigned char logbuffer[255];
			texture_operations::get_info_log(er, logbuffer, 255);
			std::cout << logbuffer << "\n";

			error_ = true;
			return false;
		}

		auto& current_character = a.second;

		// The X - Y position based on the buffer.
		// Was * 4 on the width (but GPT told me it was wrong)
		current_character.top_left = {.x = x_position, .y = y_position };
		current_character.top_right = { .x = x_position + (a.second.width_), .y = y_position };
		current_character.bottom_left = { .x = x_position, .y = y_position + a.second.height_ };
		current_character.bottom_right = { .x = x_position + (a.second.width_), .y = y_position + a.second.height_ };

		current_character.tex_coords_top_left = current_character.top_left.get_normalized(total_buffer_width, total_buffer_height);
		current_character.tex_coords_top_right = current_character.top_right.get_normalized(total_buffer_width, total_buffer_height);
		current_character.tex_coords_bottom_left = current_character.bottom_left.get_normalized(total_buffer_width, total_buffer_height);
		current_character.tex_coords_bottom_right = current_character.bottom_right.get_normalized(total_buffer_width, total_buffer_height);

		x_position += increment_x_size;
		index++;
	}


	return true;

}
#pragma endregion