// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <memory>
#include <SDL.h>
#include "font8x8.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include "ImageHelper.h"

// This can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif
#include <map>

#include "SDHRCommand.h"

std::map<int, bool> keyboard; // Saves the state(true=pressed; false=released) of each SDL_Key.

// Main code
int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("SuperDuper Helper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    io.Fonts->AddFontFromFileTTF("fonts/DroidSans.ttf", 16.0f);
    io.Fonts->AddFontFromFileTTF("fonts/Roboto-Medium.ttf", 16.0f);
    io.Fonts->AddFontFromFileTTF("fonts/Cousine-Regular.ttf", 15.0f);
	io.Fonts->AddFontFromFileTTF("fonts/Karla-Regular.ttf", 15.0f);
	io.Fonts->AddFontFromFileTTF("fonts/ProggyClean.ttf", 15.0f);
	io.Fonts->AddFontFromFileTTF("fonts/ProggyTiny.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);


    // Load Textures
	int my_image_width = 0;
	int my_image_height = 0;
	GLuint my_image_texture = 0;
	bool ret = ImageHelper::LoadTextureFromFile("Assets/Tiles_Ultima5.png", &my_image_texture, &my_image_width, &my_image_height);
	IM_ASSERT(ret);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
	bool show_tileset_window = true;
	bool show_gamelink_video_window = true;
    bool is_gamelink_focused = false;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // GameLink State
    bool activate_gamelink = false;
	bool activate_sdhr = false;

    uint16_t sprite_posx = 0;
    uint16_t sprite_posy = 0;

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = NULL;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (is_gamelink_focused)
            {
#pragma warning(push)
#pragma warning( disable : 26812 )    // unscoped enum
				switch (event.type)
				{
				case SDL_KEYDOWN:
					keyboard[event.key.keysym.sym] = true;
                    if (GameLink::IsActive())
                        GameLink::SendKeystroke((UINT)SDL_GetScancodeFromKey(event.key.keysym.sym), true);
					break;
				case SDL_KEYUP:
					keyboard[event.key.keysym.sym] = false;
                    if (GameLink::IsActive())
					    GameLink::SendKeystroke((UINT)SDL_GetScancodeFromKey(event.key.keysym.sym), false);
					break;
				}
#pragma warning(pop)
            }
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        // Sample on how to deal with keyboard events
        /*
        if (keyboard.contains(SDLK_RETURN))
        {
            if (keyboard.at(SDLK_RETURN) == true)
                printf("Return has been pressed.\n");
            else
                printf("Return has been released.\n");
        }
        */

        keyboard.clear();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("GameLink Configuration");

            ImGui::Text("Configure GameLink here");               // Display some text (you can use a format strings too)
            if (ImGui::Checkbox("GameLink Active", &activate_gamelink))
            {
				if (!GameLink::IsActive() && activate_gamelink)
					activate_gamelink = GameLink::Init();
                else if (GameLink::IsActive() && !activate_gamelink)
					GameLink::Destroy();
                activate_gamelink = GameLink::IsActive();
            }

			if (!activate_gamelink)
				ImGui::BeginDisabled();

            if (ImGui::Checkbox("Enable SuperDuperHiRes (SDHR)", &activate_sdhr))
            {
                if (activate_sdhr)
                    GameLink::SDHR_on();
                else
                    GameLink::SDHR_off();
            }

            if (ImGui::Button("Define Structs"))
            {
                auto batcher = SDHRCommandBatcher();

                DefineImageAssetFilenameCmd sc0;
                sc0.asset_index = 0;
                const std::string fname = "D:\\Repos\\SuperDuperHelper\\SuperDuperHelper\\Assets\\Tiles_Ultima5.png";
                sc0.filename = (uint8_t*)fname.c_str();
                sc0.filename_length = fname.length();

                auto c_0 = SDHRCommand_DefineImageAssetFilename(&sc0);
				batcher.AddCommand(&c_0);


                uint8_t tiles[] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, };
                DefineTilesetImmediateCmd sc1;
                sc1.asset_index = 0;
                sc1.tileset_index = 0;
                sc1.num_entries = 2;
                sc1.xdim = 8;
                sc1.ydim = 8;
                sc1.data = tiles;

                auto c_1 = SDHRCommand_DefineTilesetImmediate(&sc1);
                batcher.AddCommand(&c_1);

                DefineWindowCmd sc2;
                sc2.window_index = 0;
                sc2.screen_xcount = 640;
                sc2.screen_ycount = 360;
                sc2.screen_xbegin = 0;
                sc2.screen_ybegin = 0;
                sc2.tile_xbegin = 0;
                sc2.tile_ybegin = 0;
                sc2.tile_xdim = sc1.xdim;
                sc2.tile_ydim = sc1.ydim;
                sc2.tile_xcount = 80;
                sc2.tile_ycount = 45;
                auto c_3 = SDHRCommand_DefineWindow(&sc2);
                batcher.AddCommand(&c_3);

                DefineWindowCmd sc3 = sc2;
                sc3.window_index = 1;
                sc3.screen_xcount = 32;
                sc3.screen_ycount = 32;
                sc3.tile_xcount = 4;
                sc3.tile_ycount = 4;
                auto c_3_1 = SDHRCommand_DefineWindow(&sc3);
                batcher.AddCommand(&c_3_1);

                // Set the tile index for all the tiles
                auto matrix_tiles = std::make_unique<uint8_t[]>((uint64_t)sc2.tile_xcount * sc2.tile_ycount);
                auto mtsize = (uint64_t)sc2.tile_xcount * sc2.tile_ycount * sizeof(*matrix_tiles.get());
                uint8_t tile_i = 0;
                for (auto i = 0; i < mtsize; ++i) {
                    matrix_tiles[i] = tile_i;
                    ++tile_i;
                }

                UpdateWindowSetBothCmd sc4;
                sc4.window_index = 0;
                sc4.tile_xbegin = 0;
                sc4.tile_ybegin = 0;
                sc4.tile_xcount = sc2.tile_xcount;
                sc4.tile_ycount = sc2.tile_ycount;
                sc4.data = matrix_tiles.get();
                auto c_4 = SDHRCommand_UpdateWindowSetBoth(&sc4);
				batcher.AddCommand(&c_4);

                auto matrix_tiles2 = std::make_unique<uint8_t[]>((uint64_t)sc3.tile_xcount * sc3.tile_ycount);
                auto mtsize2 = (uint64_t)sc3.tile_xcount * sc3.tile_ycount * sizeof(*matrix_tiles2.get());
                memset(matrix_tiles2.get(), 1, mtsize2);

				UpdateWindowSetBothCmd sc4_2;
                sc4_2.window_index = 1;
                sc4_2.tile_xbegin = 0;
                sc4_2.tile_ybegin = 0;
                sc4_2.tile_xcount = sc3.tile_xcount;
                sc4_2.tile_ycount = sc3.tile_ycount;
                sc4_2.data = matrix_tiles2.get();
				auto c_4_2 = SDHRCommand_UpdateWindowSetBoth(&sc4_2);
                batcher.AddCommand(&c_4_2);

                UpdateWindowEnableCmd sc5;
                sc5.window_index = 0;
                sc5.enabled = 1;
                auto c_5 = SDHRCommand_UpdateWindowEnable(&sc5);
				batcher.AddCommand(&c_5);

				UpdateWindowEnableCmd sc5_2;
				sc5_2.window_index = 1;
				sc5_2.enabled = 1;
                auto c_5_2 = SDHRCommand_UpdateWindowEnable(&sc5_2);
                batcher.AddCommand(&c_5_2);

                batcher.Publish();
                GameLink::SDHR_process();
            }

            UpdateWindowSetWindowPositionCmd scWP;
            scWP.window_index = 1;
            scWP.screen_xbegin = sprite_posx;
            scWP.screen_ybegin = sprite_posy;
			ImGui::SeparatorText("Move Sprite");
			static int sprite_pos_abs_h = sprite_posx;
            if (ImGui::SliderInt("Move Sprite Horizontal", &sprite_pos_abs_h, 0, 640))
            {
                scWP.screen_xbegin = sprite_pos_abs_h;
				auto batcher = SDHRCommandBatcher();
				auto c1 = SDHRCommand_UpdateWindowSetWindowPosition(&scWP);
				batcher.AddCommand(&c1);
				batcher.Publish();
				GameLink::SDHR_process();
            }
			static int sprite_pos_abs_v = sprite_posy;
			if (ImGui::SliderInt("Move Sprite Vertical", &sprite_pos_abs_v, 0, 360))
			{
				scWP.screen_ybegin = sprite_pos_abs_v;
				auto batcher = SDHRCommandBatcher();
				auto c1 = SDHRCommand_UpdateWindowSetWindowPosition(&scWP);
				batcher.AddCommand(&c1);
				batcher.Publish();
				GameLink::SDHR_process();
			}

            if (ImGui::Button("Move Sprite Down"))
            {
                auto batcher = SDHRCommandBatcher();
                sprite_posy += 1;
				scWP.screen_ybegin = sprite_posy;
				auto c1 = SDHRCommand_UpdateWindowSetWindowPosition(&scWP);
                batcher.AddCommand(&c1);
                batcher.Publish();
                GameLink::SDHR_process();
            }
            if (ImGui::Button("Move Sprite UP"))
            {
                auto batcher = SDHRCommandBatcher();
                sprite_posy -= 1;
				scWP.screen_ybegin = sprite_posy;
				auto c1 = SDHRCommand_UpdateWindowSetWindowPosition(&scWP);
                batcher.AddCommand(&c1);
                batcher.Publish();
                GameLink::SDHR_process();
            }
            if (ImGui::Button("Move Sprite Right"))
            {
                auto batcher = SDHRCommandBatcher();
                sprite_posx += 1;
				scWP.screen_xbegin = sprite_posx;
				auto c1 = SDHRCommand_UpdateWindowSetWindowPosition(&scWP);
                batcher.AddCommand(&c1);
                batcher.Publish();
                GameLink::SDHR_process();
            }
            if (ImGui::Button("Move Sprite Left"))
            {
                auto batcher = SDHRCommandBatcher();
                sprite_posx -= 1;
				scWP.screen_xbegin = sprite_posx;
				auto c1 = SDHRCommand_UpdateWindowSetWindowPosition(&scWP);
                batcher.AddCommand(&c1);
                batcher.Publish();
                GameLink::SDHR_process();
            }
            //ImGui::SameLine();
    //        if (!GameLink::SDHR_IsReadyToProcess())
    //        {
    //            ImGui::BeginDisabled();
    //            ImGui::Button("Process");
    //            ImGui::EndDisabled();
    //        }
    //        else
    //        {
				//if (ImGui::Button("Process"))
				//	GameLink::SDHR_process();
    //        }

			if (ImGui::Button("Reset"))
				GameLink::SDHR_reset();

			if (!activate_gamelink)
				ImGui::EndDisabled();

            ImGui::NewLine();

			ImGui::SeparatorText("Other");

			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state


            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // 4. Show texture in a window
        if (show_tileset_window)
		{
            ImVec2 vpos = ImVec2(300.f, 100.f);
            ImGui::SetNextWindowPos(vpos, ImGuiCond_FirstUseEver);
			ImGui::Begin("Ultima V (Amiga) Tileset", &show_tileset_window);
			ImGui::Text("pointer = %p", my_image_texture);
			ImGui::Text("size = %d x %d", my_image_width, my_image_height);
			ImGui::Image((void*)(intptr_t)my_image_texture, ImVec2(my_image_width, my_image_height));
			ImGui::End();
		}

		// 4. Show gamelink in a window

        if (show_gamelink_video_window && activate_gamelink)
        {
            // Load video
            auto fbI = GameLink::GetFrameBufferInfo();
            GLuint gamelink_video_texture = 0;
            bool ret = ImageHelper::LoadTextureFromMemory(fbI.frameBuffer, &gamelink_video_texture, fbI.width, fbI.height, true);
            ImVec2 vpos = ImVec2(300.f, 300.f);
            ImGui::SetNextWindowPos(vpos, ImGuiCond_FirstUseEver);
            ImGui::Begin("AppleWin Video", &show_gamelink_video_window);
            ImGui::Text("size = %d x %d", fbI.width, fbI.height);
            ImGui::Image((void*)(intptr_t)gamelink_video_texture, ImVec2(fbI.width, fbI.height), ImVec2(0, 1), ImVec2(1, 0));
            is_gamelink_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
            ImGui::End();
        }
        else
            is_gamelink_focused = false;

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    if (GameLink::IsActive())
        GameLink::Destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
