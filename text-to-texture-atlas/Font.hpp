#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <freetype/freetype.h>
#include FT_FREETYPE_H

namespace text_to_texture_atlas
{
	/**
	 * @class Font
	 * @brief Manages the loading, rendering, and packing of font characters into a single texture atlas.
	 *
	 * @details This class provides a complete pipeline for converting a standard font file (like TTF)
	 *          into a GPU-ready texture atlas. It uses the FreeType library to load a font, render
	 *          a specified range of characters (by default, all printable ASCII), and then packs
	 *          the resulting bitmaps into a single, efficient texture.
	 *
	 *          The class offers two primary methods for font sizing:
	 *          1.  **Point-based (`Font_Pt`)**: Traditional sizing for document-style rendering,
	 *              controlled by point size and DPI.
	 *          2.  **Pixel-based (`Font_Px`)**: Precise sizing for pixel-perfect UI and game
	 *              development, controlled by exact pixel dimensions.
	 *
	 *          Once created, the `Font` object holds the main texture atlas and a map of character
	 *          metrics. This data is sufficient for rendering any string of text using a graphics
	 *          API like OpenGL or DirectX.
	 *
	 * @note The entire process is handled during construction. Object creation can be time-consuming,
	 *       so it's best to create `Font` objects during a loading phase.
	 *
	 * @warning This class is not thread-safe. Each thread that needs to create a font atlas should
	 *          have its own `Font` object.
	 *
	 * @usage
	 * ### Basic Usage
	 * 1.  **Create a Font object** using one of the static factory methods (`Font_Pt` or `Font_Px`).
	 * 2.  **Check for success** by using the object in a boolean context (e.g., `if (my_font)`).
	 * 3.  **Get the atlas** using `get_main_atlas()` and upload it's buffer to the GPU.
	 * 4.  **Render text** by iterating through a string, using `get_character()` to find the
	 *     metrics and texture coordinates for each character, and drawing a quad for each one.
	 * 5.  **(Optional) Free memory** using `free_character_buffers()` and `free_atlas_buffer()`
	 *     after the texture is on the GPU.
	 *
	 * @example
	 * ```cpp
	 * // 1. Create a 32-pixel high Arial font
	 * auto arial_font = text_to_texture_atlas::Font::Font_Px("arial.ttf", 32, 0);
	 *
	 * // 2. Check if the font loaded successfully
	 * if (!arial_font) {
	 *     std::cerr << "Error: Could not load font 'arial.ttf'." << std::endl;
	 *     return -1;
	 * }
	 *
	 * // 3. Get the atlas and upload to GPU (hypothetical function)
	 * auto& atlas = arial_font.get_main_atlas();
	 * GLuint texture_id = upload_texture_to_gpu(atlas.atlas_buffer.data(), atlas.width, atlas.height);
	 *
	 * // 4. Render the string "Hello!"
	 * std::string text_to_render = "Hello!";
	 * float cursor_x = 0.0f;
	 *
	 * for (char c : text_to_render) {
	 *     auto& ch = arial_font.get_character(c);
	 *
	 *     // Calculate quad vertices using ch.width_, ch.height_, ch.x_bearing_, ch.y_bearing_
	 *     // Use ch.tex_coords_* for UV mapping from the atlas texture
	 *     draw_quad(cursor_x, ch, texture_id);
	 *
	 *     // Advance cursor for the next character
	 *     cursor_x += (ch.advance_x_ >> 6); // advance_x is in 1/64 pixels
	 * }
	 *
	 * // 5. Free CPU-side memory now that the texture is on the GPU
	 * arial_font.free_character_buffers();
	 * arial_font.free_atlas_buffer();
	 * ```
	 */
	class Font
	{
		#pragma region internal
		/**
		 * @struct character
		 * @brief Holds all rendering metrics and positioning data for a single character.
		 *
		 * @details This struct contains everything needed to render a character from the font atlas.
		 *          It includes the character's dimensions, layout metrics (like advance and bearing),
		 *          and the specific texture coordinates required to sample its bitmap from the main
		 *          atlas texture. An instance of this struct is created for each character loaded
		 *          by the Font class.
		 */
		struct character
		{
			/**
			 * @struct vector2f
			 * @brief A simple 2D vector using floating-point coordinates.
			 */
			struct vector2f
			{
				/// @brief The coordinate on the x-axis.
				float x{};
				/// @brief The coordinate on the y-axis.
				float y{};

				/**
				 * @brief Prints the x and y coordinates to the console. For debugging.
				 */
				void output() const
				{
					std::cout << "X : " << x << " | " << "Y : " << y << "\n";
				}
			};

			/**
			 * @struct vector2
			 * @brief A simple 2D vector using unsigned integer coordinates.
			 */
			struct vector2
			{
				/// @brief The coordinate on the x-axis.
				unsigned int x{};
				/// @brief The coordinate on the y-axis.
				unsigned int y{};

				/**
				 * @brief Normalizes the vector's coordinates to a range of [0.0, 1.0].
				 * @param max_width The maximum width of the space to normalize against (e.g., atlas width).
				 * @param max_height The maximum height of the space to normalize against (e.g., atlas height).
				 * @return A `vector2f` with normalized x and y coordinates.
				 */
				[[nodiscard]] vector2f get_normalized(const unsigned int max_width, const unsigned int max_height) const
				{
					return vector2f{ .x = static_cast<float>(x) / static_cast<float>(max_width), .y = static_cast<
						float>(y) / static_cast<float>(max_height)
					};
				}

				/**
				 * @brief Prints the x and y coordinates to the console. For debugging.
				 */
				inline void output() const
				{
					std::cout << "X : " << x << " | " << "Y : " << y << "\n";
				}
			};

			//--- Glyph Metrics ---//

			/// @brief The width of the character's bitmap in pixels.
			unsigned int width_{};
			/// @brief The height of the character's bitmap in pixels.
			unsigned int height_{};

			/// @brief The horizontal distance from the cursor's origin to the left edge of the bitmap.
			int x_bearing_{};
			/// @brief The vertical distance from the cursor's origin to the top edge of the bitmap.
			int y_bearing_{};

			/// @brief The horizontal distance to advance the cursor for the next character. (in 1/64 pixels)
			int advance_x_{};
			/// @brief The vertical distance to advance the cursor (for vertical layouts). (in 1/64 pixels)
			int advance_y_{};

			//--- Atlas Positioning ---//

			/// @brief The top-left pixel coordinate of the character within the main atlas texture.
			vector2 top_left{};
			/// @brief The top-right pixel coordinate of the character within the main atlas texture.
			vector2 top_right{};
			/// @brief The bottom-left pixel coordinate of the character within the main atlas texture.
			vector2 bottom_left{};
			/// @brief The bottom-right pixel coordinate of the character within the main atlas texture.
			vector2 bottom_right{};

			//--- Normalized Texture Coordinates (0.0 to 1.0) ---//

			/// @brief The top-left UV texture coordinate for sampling from the atlas.
			vector2f tex_coords_top_left{};
			/// @brief The top-right UV texture coordinate for sampling from the atlas.
			vector2f tex_coords_top_right{};
			/// @brief The bottom-left UV texture coordinate for sampling from the atlas.
			vector2f tex_coords_bottom_left{};
			/// @brief The bottom-right UV texture coordinate for sampling from the atlas.
			vector2f tex_coords_bottom_right{};

			//--- Raw Data (Temporary) ---//

			/**
			 * @brief The raw 4-channel (RGBA) bitmap data for this character.
			 * @note This buffer is temporary and is used to build the main atlas. It can be cleared
			 *       by calling `Font::free_character_buffers()` to save memory after the atlas is created.
			 */
			std::vector<unsigned char> raw_bitmap_buffer{};

			//--- Debug Methods ---//

			/**
			 * @brief Prints the character's pixel positions within the atlas to the console.
			 */
			inline void output_buffer_positions() const
			{
				std::cout << "Top Left     : ";     top_left.output();
				std::cout << "Top Right    : ";     top_right.output();
				std::cout << "Bottom Left  : ";     bottom_left.output();
				std::cout << "Bottom Right : ";     bottom_right.output();
			}

			/**
			 * @brief Prints the character's normalized texture coordinates to the console.
			 */
			inline void output_texture_coordinates() const
			{
				std::cout << "Tex Top Left     : ";     tex_coords_top_left.output();
				std::cout << "Tex Top Right    : ";     tex_coords_top_right.output();
				std::cout << "Tex Bottom Left  : ";     tex_coords_bottom_left.output();
				std::cout << "Tex Bottom Right : ";     tex_coords_bottom_right.output();
			}

			/**
			 * @brief Prints a visual representation of the character's raw bitmap to the console.
			 * @warning This function will produce no output if the buffer has been cleared by
			 *          `Font::free_character_buffers()`.
			 */
			void output_raw() const;
		};
		/**
		 * @struct atlas
		 * @brief Represents the generated texture atlas containing all character bitmaps.
		 *
		 * @details This struct holds the final, consolidated texture image created by the Font class.
		 *          It contains the raw pixel data for all rendered characters packed into a single
		 *          buffer, along with the dimensions of the complete atlas texture. This data is
		 *          ready to be uploaded directly to a GPU for rendering text.
		 */
		struct atlas
		{
			/**
			 * @brief The raw pixel data for the entire texture atlas.
			 * @details This buffer contains a tightly packed, 4-channel (RGBA) bitmap. The alpha
			 *          channel represents the glyph's shape and antialiasing. The data can be
			 *          passed directly to graphics APIs like OpenGL's `glTexImage2D`.
			 */
			std::vector<unsigned char> atlas_buffer{};

			/// @brief The total width of the atlas texture in pixels.
			unsigned int width{};
			/// @brief The total height of the atlas texture in pixels.
			unsigned int height{};
		};
		#pragma endregion

		// Freetype objects
		FT_Library library_{};	// Freetype library.
		FT_Face face_{};		// Font face object.
		FT_Error ft_error_{};	// Last freetype error code.
		bool error_{};			// For capturing any errors during construction.

		// Character and atlas storage
		std::unordered_map<char, character> character_map_{};	// Holds each character and it's relative character data.
		atlas main_atlas_{};									// Holds the main atlas for the specified font.

		// Font configuration
		std::string windows_fonts_paths_{ "C:/Windows/Fonts/" };	// Windows font paths.
		std::string selected_font_;									// The font chosen by the user.

		// Font Sizing
		signed long char_pt_size_{ 64 * 64 };		// The font size in pt.
		unsigned int char_width_dpi_{ 600 };		// The font DPI width.
		unsigned int char_height_dpi_{ 600 };		// The font DPI height.
		unsigned int char_width_px_{ 0 };			// The font width in pixels.
		unsigned int char_height_px_{ 600 };		// The font height in pixels.

		// Character Processing Range
		int char_range_min{ 32 };		// First printable character.
		int char_range_max{ 126 };		// Last Printable character.

		// Font and Atlas initialization
		bool init_library();						// initializes the `library_` and returns false if unsuccessful.
		bool init_face();							// initializes  the 'face_' and returns false if unsuccessful.
		bool init_char_size();						// initializes the character pt sizes, returns false if unsuccessful.
		bool init_pixel_size();						// initializes the character px sizes, returns false if unsuccessful.
		bool init_character_map();					// initializes the character_map_, returns false if unsuccessful.
		bool init_main_atlas_buffer();				// initializes the atlas buffer, ensuring the buffer is large enough to account for all characters.
		bool convert_bitmap_to_four_channel_buffer	// Converts the raw bitmap buffer into a four channel buffer.
			(std::vector<unsigned char>& dst_vector,
				unsigned int bitmap_width,
				unsigned int bitmap_height) const;

		// Getters
		size_t get_total_buffer_size() const;				// Calculates the total buffer size needed for the atlas.
		unsigned int get_max_character_width() const;		// Calculates the maximum width of a character from all characters in the map.
		unsigned int get_max_character_height() const;		// Calculates the maximum height of a character from all characters in the map.

		// Constructors.
		explicit Font(								// Generates a font and atlas using the pt size.
			std::string font_name,
			signed long char_pt_size = 64 * 64,
			unsigned int char_width_dpi = 600,
			unsigned int char_height_dpi = 600
		);
		
		explicit Font(								// Generates a font and atlas using pixel size.
			std::string font_name,
			unsigned int char_height = 0,
			unsigned int char_width = 0
		);

	public:

		// Factory Constructors
		/**
		 * @brief Creates a font atlas using point-based sizing and DPI configuration.
		 *
		 * @details This factory method generates a complete font texture atlas by loading the specified
		 * font file, rendering all printable ASCII characters (32-126), and packing them into
		 * a single texture buffer. The font is sized using traditional point measurements with
		 * configurable DPI for high-quality text rendering.
		 *
		 * @param font_name The font filename including extension (e.g., "arial.ttf", "times.ttf").
		 *                  Must be a valid font file located in the Windows fonts directory
		 *                  (C:/Windows/Fonts/). Only the filename is required, not the full path.
		 *
		 * @param char_pt_size Font size in points, multiplied by 64 for precision (default: 64*64 = 64pt).
		 *                     Higher values produce crisper text at larger sizes. The value is passed
		 *                     directly to FreeType's FT_Set_Char_Size function.
		 *
		 * @param char_width_dpi Horizontal resolution in dots per inch (default: 600).
		 *                       Higher DPI values produce sharper character edges but increase
		 *                       memory usage and processing time.
		 *
		 * @param char_height_dpi Vertical resolution in dots per inch (default: 600).
		 *                        Should typically match char_width_dpi for proportional rendering.
		 *
		 * @return Font object containing the complete texture atlas and character metrics.
		 *         Use the bool conversion operator to check if construction was successful.
		 *
		 * @throws None - Uses error flag instead. Check success with: if (font) { ... }
		 *
		 * @note The atlas includes all printable ASCII characters (space through tilde).
		 *       Characters are automatically spaced and positioned within the atlas buffer.
		 *       The resulting atlas can be used directly for OpenGL/DirectX texture rendering.
		 *
		 * @warning Font file must exist in C:/Windows/Fonts/ directory. Invalid font names
		 *          will result in construction failure (detectable via bool conversion).
		 *
		 * @example
		 * ```cpp
		 * // Create a 48pt Arial font at 600 DPI
		 * auto arial_font = text_to_texture_atlas::Font::Font_Pt("arial.ttf", 48 * 64, 600, 600);
		 * if (arial_font) {
		 *     // Successfully loaded - access character data
		 *     auto& char_a = arial_font.get_character('A');
		 *     auto& atlas = arial_font.get_main_atlas();
		 *
		 *     // Use atlas.atlas_buffer.data() for OpenGL texture upload
		 *     // Use char_a.tex_coords_* for UV coordinates
		 *     std::cout << "Atlas size: " << atlas.width << "x" << atlas.height << std::endl;
		 * } else {
		 *     std::cout << "Failed to load font!" << std::endl;
		 * }
		 *
		 * // Create high-quality font for large text
		 * auto large_font = text_to_texture_atlas::Font::Font_Pt("times.ttf", 72 * 64, 1200, 1200);
		 *
		 * // Create standard font with defaults (64pt at 600 DPI)
		 * auto default_font = text_to_texture_atlas::Font::Font_Pt("calibri.ttf");
		 * ```
		 *
		 * @see Font_Px() for pixel-based sizing alternative
		 * @see get_character() to access individual character data
		 * @see get_main_atlas() to access the complete texture atlas
		 */
		static Font Font_Pt(
			const std::string& font_name,
			signed long char_pt_size = 64 * 64,
			unsigned int char_width_dpi = 600,
			unsigned int char_height_dpi = 600
		);
		/**
		 * @brief Creates a font atlas using pixel-based sizing for precise control over character dimensions.
		 *
		 * @details This factory method generates a complete font texture atlas by loading the specified
		 * font file, rendering all printable ASCII characters (32-126), and packing them into
		 * a single texture buffer. The font is sized using direct pixel measurements, providing
		 * exact control over character dimensions without DPI considerations.
		 *
		 * @param font_name The font filename including extension (e.g., "arial.ttf", "consolas.ttf").
		 *                  Must be a valid font file located in the Windows fonts directory
		 *                  (C:/Windows/Fonts/). Only the filename is required, not the full path.
		 *
		 * @param char_height Character height in pixels (default: 0 = auto-sized).
		 *                    When set to 0, FreeType automatically determines the height based on
		 *                    the font's internal metrics. Positive values force exact pixel height.
		 *
		 * @param char_width Character width in pixels (default: 0 = auto-sized).
		 *                   When set to 0, FreeType automatically determines the width based on
		 *                   the font's internal metrics and maintains proper proportions.
		 *
		 * @return Font object containing the complete texture atlas and character metrics.
		 *         Use the bool conversion operator to check if construction was successful.
		 *
		 * @throws None - Uses error flag instead. Check success with: if (font) { ... }
		 *
		 * @note This method uses FreeType's FT_Set_Pixel_Sizes function internally, providing
		 *       direct pixel-level control without DPI scaling. Ideal for pixel-perfect rendering
		 *       in games and applications where exact sizing is crucial.
		 *
		 * @note The atlas includes all printable ASCII characters (space through tilde).
		 *       Characters are automatically spaced and positioned within the atlas buffer.
		 *       The resulting atlas can be used directly for OpenGL/DirectX texture rendering.
		 *
		 * @warning Font file must exist in C:/Windows/Fonts/ directory. Invalid font names
		 *          will result in construction failure (detectable via bool conversion).
		 *
		 * @warning Setting both char_width and char_height to 0 may result in very small or
		 *          unusable character sizes depending on the font's internal metrics.
		 *
		 * @example
		 * ```cpp
		 * // Create a 32-pixel high font with automatic width
		 * auto pixel_font = text_to_texture_atlas::Font::Font_Px("arial.ttf", 32, 0);
		 * if (pixel_font) {
		 *     auto& atlas = pixel_font.get_main_atlas();
		 *     std::cout << "Created " << atlas.width << "x" << atlas.height << " atlas" << std::endl;
		 * }
		 *
		 * // Create a square monospace-style font (32x32 pixels)
		 * auto mono_font = text_to_texture_atlas::Font::Font_Px("consolas.ttf", 32, 32);
		 *
		 * // Create font with automatic sizing (uses font's default metrics)
		 * auto auto_font = text_to_texture_atlas::Font::Font_Px("times.ttf", 0, 0);
		 *
		 * // Create high-resolution pixel font for detailed rendering
		 * auto hires_font = text_to_texture_atlas::Font::Font_Px("calibri.ttf", 128, 0);
		 * if (hires_font) {
		 *     // Access individual character data
		 *     auto& char_A = hires_font.get_character('A');
		 *     std::cout << "Character 'A' size: " << char_A.width_ << "x" << char_A.height_ << std::endl;
		 *
		 *     // Get texture coordinates for rendering
		 *     auto coords = char_A.tex_coords_top_left;
		 *     std::cout << "UV coords: " << coords.x << ", " << coords.y << std::endl;
		 * }
		 * ```
		 *
		 * @see Font_Pt() for point-based sizing with DPI configuration
		 * @see get_character() to access individual character data
		 * @see get_main_atlas() to access the complete texture atlas
		 */
		static Font Font_Px(
			const std::string& font_name,
			unsigned int char_height = 0,
			unsigned int char_width = 0
		);

		// Operators
		/**
		 * @brief Checks if the font was successfully constructed and is ready for use.
		 *
		 * @details for checking if the font successfully initialized.
		 *
		 * @return true if font construction succeeded and atlas is available, false otherwise.
		 *
		 * @note This operator allows Font objects to be used in boolean contexts.
		 *       Returns false if any initialization step failed (library, face, character map, or atlas).
		 *
		 * @example
		 * ```cpp
		 * auto font = Font::Font_Pt("arial.ttf");
		 * if (font) {
		 *     // Font loaded successfully - safe to use
		 *     auto& atlas = font.get_main_atlas();
		 * } else {
		 *     // Font failed to load - handle error
		 *     std::cout << "Font loading failed!" << std::endl;
		 * }
		 * ```
		 */
		explicit operator bool() const
		{
			return !error_;
		}

		// Cleanup
		/**
		 * @brief Releases memory used by individual character bitmaps after the main atlas is created.
		 *
		 * @details This function iterates through all loaded characters and clears their
		 *          `raw_bitmap_buffer`. This is a memory optimization that should be called
		 *          once the main texture atlas has been generated and the individual character
		 *          bitmaps are no longer needed for processing or debugging.
		 *
		 * @note This is an optional memory-saving step. The main texture atlas, character metrics,
		 *       and texture coordinates remain unaffected and fully usable for rendering.
		 *
		 * @warning After calling this function, the raw bitmap data for each character is permanently
		 *          deleted. Any subsequent operations that require individual character bitmaps
		 *          (e.g., `character::output_raw()`, re-generating the atlas) will fail or produce
		 *          empty results.
		 *
		 * @example
		 * ```cpp
		 * // Create the font and its atlas
		 * auto font = text_to_texture_atlas::Font::Font_Px("arial.ttf", 64, 0);
		 * if (font) {
		 *     // Get the main atlas for rendering
		 *     auto& atlas = font.get_main_atlas();
		 *
		 *     // (Optional) Upload the atlas texture to the GPU here
		 *     // upload_texture(atlas.atlas_buffer.data(), atlas.width, atlas.height);
		 *
		 *     // The individual character buffers are now redundant. Free them to save memory.
		 *     font.free_character_buffers();
		 *
		 *     // Now, the Font object uses significantly less RAM, but the atlas is still intact.
		 *     // Rendering text using the atlas and character metrics will still work perfectly.
		 * }
		 * ```
		 *
		 * @see free_atlas_buffer() to release the main atlas memory.
		 */
		void free_character_buffers();
		/**
		 * @brief Releases memory used by the main texture atlas buffer.
		 *
		 * @details This function clears the `atlas_buffer` within the `main_atlas_` struct.
		 *          This is a significant memory optimization that should be called after the
		 *          atlas texture has been uploaded to the GPU or is otherwise no longer needed
		 *          in system memory.
		 *
		 * @note This is an optional memory-saving step. Character metrics and texture coordinates
		 *       remain unaffected, but the raw pixel data for the atlas will be gone.
		 *
		 * @warning After calling this function, the main atlas texture data is permanently deleted.
		 *          Any subsequent operations that require the atlas buffer (e.g., re-uploading
		 *          the texture, saving it to a file) will fail or use an empty buffer.
		 *
		 * @example
		 * ```cpp
		 * // Create the font and its atlas
		 * auto font = text_to_texture_atlas::Font::Font_Px("arial.ttf", 64, 0);
		 * if (font) {
		 *     auto& atlas = font.get_main_atlas();
		 *
		 *     // Upload the atlas texture to the GPU
		 *     upload_texture_to_gpu(atlas.atlas_buffer.data(), atlas.width, atlas.height);
		 *
		 *     // The atlas buffer is now redundant in system memory. Free it.
		 *     font.free_atlas_buffer();
		 *
		 *     // The Font object now uses minimal RAM.
		 *     // Rendering text using the GPU texture and stored character metrics will still work.
		 * }
		 * ```
		 *
		 * @see free_character_buffers() to release memory from individual character bitmaps.
		 */
		void free_atlas_buffer();

		// Getters
		/**
		 * @brief Retrieves the data for a specific character from the font atlas.
		 *
		 * @details This function provides access to a `character` struct containing all
		 *          rendering and metric information for the requested character, such as its
		 *          size, bearing, advance, and texture coordinates within the main atlas.
		 *
		 * @param character_ The ASCII character to retrieve (e.g., 'A', 'b', '?').
		 *
		 * @return A reference to the `character` struct. This allows for both reading and
		 *         modification of the character's data, though modification is not advised.
		 *
		 * @note The character must be within the range loaded during font creation
		 *       (typically ASCII 32-126).
		 *
		 * @warning This function uses the `[]` operator of `std::unordered_map`. If the
		 *          requested character does not exist in the map (e.g., it's outside the
		 *          loaded range), this will insert a new, default-constructed `character`
		 *          object into the map and return a reference to it. This new object will
		 *          contain empty or zeroed-out data, which may lead to unexpected rendering
		 *          artifacts if not handled correctly.
		 *
		 * @example
		 * ```cpp
		 * auto font = text_to_texture_atlas::Font::Font_Px("arial.ttf", 32, 0);
		 * if (font) {
		 *     // Get the data for the character 'A'
		 *     auto& char_A = font.get_character('A');
		 *
		 *     // Use its texture coordinates and metrics to render it
		 *     float x_pos = 0.0f;
		 *     float y_pos = 0.0f;
		 *
		 *     // Example: Calculate vertex positions for a quad
		 *     float x = x_pos + char_A.x_bearing_;
		 *     float y = y_pos - (char_A.height_ - char_A.y_bearing_);
		 *     float w = char_A.width_;
		 *     float h = char_A.height_;
		 *
		 *     // Use char_A.tex_coords_* for UV mapping
		 *
		 *     // Advance the cursor for the next character
		 *     x_pos += (char_A.advance_x_ >> 6); // Advance is in 1/64 pixels
		 * }
		 * ```
		 *
		 * @see get_main_atlas() to access the complete texture atlas.
		 * @see Font::character for details on the returned struct.
		 */
		character& get_character(char character_);
		/**
		 * @brief Retrieves the main texture atlas containing all rendered characters.
		 *
		 * @details This function provides access to the `atlas` struct, which holds the
		 *          complete, generated texture image. The atlas contains all characters
		 *          from the specified range, packed together into a single bitmap.
		 *
		 * @return A reference to the `atlas` struct. This contains the raw pixel data
		 *         in `atlas_buffer` as well as the `width` and `height` of the texture.
		 *
		 * @note The `atlas_buffer` contains 4-channel (RGBA) pixel data, ready to be
		 *       uploaded to a GPU for rendering. The alpha channel represents the
		 *       character's shape and anti-aliasing.
		 *
		 * @warning The returned reference provides direct access to the internal atlas
		 *          data. Modifying the buffer or its dimensions may lead to undefined
		 *          behavior.
		 *
		 * @example
		 * ```cpp
		 * auto font = text_to_texture_atlas::Font::Font_Px("arial.ttf", 32, 0);
		 * if (font) {
		 *     // Get the main atlas for rendering
		 *     auto& main_atlas = font.get_main_atlas();
		 *
		 *     // Upload the atlas texture to the GPU
		 *     // (This is a hypothetical GPU texture upload function)
		 *     upload_texture_to_gpu(
		 *         main_atlas.atlas_buffer.data(),
		 *         main_atlas.width,
		 *         main_atlas.height
		 *     );
		 *
		 *     std::cout << "Atlas texture uploaded to GPU. Dimensions: "
		 *               << main_atlas.width << "x" << main_atlas.height << std::endl;
		 * }
		 * ```
		 *
		 * @see get_character() to retrieve the texture coordinates for a character within this atlas.
		 * @see free_atlas_buffer() to release the memory used by the atlas buffer after it's no longer needed.
		 * @see Font::atlas for details on the returned struct.
		 */
		atlas& get_main_atlas();
		inline int get_char_range_min() const { return char_range_min; }	// returns the character processing range minimum.
		inline int get_char_range_max() const { return char_range_max; }	// returns the character processing range maximum.

	};


}
