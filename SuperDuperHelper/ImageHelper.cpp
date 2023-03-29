#include "ImageHelper.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ImageHelper
{
	// Simple helper function to load an image into a OpenGL texture with common settings
	bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
	{
		// Load from file
		int image_width = 0;
		int image_height = 0;
		unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
		if (image_data == NULL)
			return false;

		LoadTextureFromMemory(image_data, out_texture, image_width, image_height);

		*out_width = image_width;
		*out_height = image_height;

		stbi_image_free(image_data);

		return true;
	}

	bool LoadTextureFromMemory(const unsigned char* image_data, GLuint* out_texture, const int image_width, const int image_height)
	{
		// Create a OpenGL texture identifier
		GLuint image_texture;
		glGenTextures(1, &image_texture);
		glBindTexture(GL_TEXTURE_2D, image_texture);

		// Setup filtering parameters for display
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

		// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

		*out_texture = image_texture;

		return true;
	}

	void convertRGB888toRGB555(const uint8_t* rgb888_buffer, int width, int height, uint16_t* rgb555_buffer) {
		size_t num_pixels = (size_t)width * height;

		for (size_t i = 0; i < num_pixels; ++i) {
			uint8_t r = rgb888_buffer[i * 3];
			uint8_t g = rgb888_buffer[i * 3 + 1];
			uint8_t b = rgb888_buffer[i * 3 + 2];

			uint16_t r_555 = r >> 3;
			uint16_t g_555 = g >> 3;
			uint16_t b_555 = b >> 3;

			rgb555_buffer[i] = (r_555 << 10) | (g_555 << 5) | b_555;
		}
	}
}