# Text to Texture Atlas

A C++20 library that converts font objects from FreeType to 4-channel RGBA texture atlases for use in OpenGL and other graphics APIs.

## Documentation

### *[Documentation](https://hannanlukas.github.io/text-to-texture-atlas/)*

## Description

This library takes TrueType font files and generates optimized texture atlases containing all printable ASCII characters. Each character is rendered with antialiasing and packed efficiently into a single RGBA texture that can be used directly with graphics APIs like OpenGL.

## Features

- **Font Loading**: Load TrueType fonts using FreeType library
- **Texture Atlas Generation**: Pack all characters into a single 4-channel (RGBA) texture
- **Flexible Sizing**: Support for both point-based and pixel-based font sizing
- **Character Metrics**: Provides advance metrics, positioning data, and texture coordinates
- **OpenGL Ready**: Direct compatibility with `glTexImage2D` and other GL functions
- **Memory Management**: Efficient memory usage with optional buffer cleanup
- **Error Logging**: Comprehensive error reporting using spdlog

## Dependencies

- **FreeType 2**: For loading and rendering font glyphs
  - Homepage: https://freetype.org/
  - License: FreeType License (BSD-style)
- **spdlog**: Fast C++ logging library
  - Homepage: https://github.com/gabime/spdlog
  - License: MIT License
- **C++20**: Modern C++ features and standard library

## Usage

### Basic Example

```cpp
#include "Font.hpp"

int main() {
    // Create a font using pixel sizing
    auto arial = text_to_texture_atlas::Font::Font_Px("arial.ttf", 100, 100);
    
    // Check if font loaded successfully
    if (!arial) {
        return -1;
    }
    
    // Get the main texture atlas
    auto& atlas = arial.get_main_atlas();
    auto& atlas_buffer = atlas.atlas_buffer;  // RGBA pixel data
    auto atlas_width = atlas.width;           // Atlas width in pixels
    auto atlas_height = atlas.height;         // Atlas height in pixels
    
    // Get individual character data
    auto char_a = arial.get_character('a');
    
    // Debug output
    char_a.output_raw();                      // Raw character representation
    char_a.output_buffer_positions();         // Atlas buffer positions
    char_a.output_texture_coordinates();      // Normalized texture coordinates
    
    // Clean up memory (optional)
    arial.free_character_buffers();
    arial.free_atlas_buffer();
    
    return 0;
}
```

### Font Creation Methods

```cpp
// Create font using point size and DPI
auto font_pt = text_to_texture_atlas::Font::Font_Pt("font.ttf", 12, 96, 96);

// Create font using pixel dimensions
auto font_px = text_to_texture_atlas::Font::Font_Px("font.ttf", 64, 64);
```

### Character Information

Each character provides:
- **Metrics**: Width, height, bearing, and advance values
- **Atlas Positioning**: Pixel coordinates within the texture atlas
- **Texture Coordinates**: Normalized UV coordinates (0.0-1.0) for sampling
- **Raw Bitmap**: Individual character bitmap data (temporary)

## API Reference

### Font Class

- `Font::Font_Pt(font_path, pt_size, width_dpi, height_dpi)` - Create font with point sizing
- `Font::Font_Px(font_path, height_px, width_px)` - Create font with pixel sizing
- `get_character(char)` - Get character data and metrics
- `get_main_atlas()` - Get the complete texture atlas
- `free_character_buffers()` - Free individual character buffers
- `free_atlas_buffer()` - Free the main atlas buffer

### Atlas Structure

- `atlas_buffer` - Vector of RGBA pixel data
- `width` - Atlas texture width
- `height` - Atlas texture height

### Character Structure

- Position data: `top_left`, `top_right`, `bottom_left`, `bottom_right`
- Texture coordinates: `tex_coords_*` variants
- Metrics: `width_`, `height_`, `bearing_x_`, `bearing_y_`, `advance_x_`, `advance_y_`

## Building

This project requires:
- C++20 compatible compiler
- FreeType library
- spdlog library

## Error Handling

The library uses spdlog for comprehensive error logging. All major operations are logged, and errors are reported through both the console logger and internal error flags.

## Memory Management

- Character buffers can be freed after atlas creation to save memory
- Atlas buffer should be kept alive while the texture is in use
- RAII principles ensure proper cleanup of FreeType resources

## Acknowledgments

- [FreeType](https://freetype.org/) - Font loading and glyph rendering
- [spdlog](https://github.com/gabime/spdlog) - Fast C++ logging library

## License

This project is currently unlicensed. Please contact the author for usage permissions.
