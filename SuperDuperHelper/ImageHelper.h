#pragma once
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

namespace ImageHelper
{
	extern bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height);
	bool LoadTextureFromMemory(const unsigned char* image_data, GLuint* out_texture, const int image_width, const int image_height);
};

