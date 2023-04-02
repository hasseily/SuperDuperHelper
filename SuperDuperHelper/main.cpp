// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"
#include <stdio.h>
#include <memory>
#include <SDL.h>
#include "font8x8.h"
#include "brittania_tiles.h"
#include <fstream>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include "ImageHelper.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "ini.h"

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
    auto myFont = io.Fonts->AddFontFromFileTTF("fonts/Roboto-Medium.ttf", 16.0f);
    io.Fonts->AddFontFromFileTTF("fonts/Cousine-Regular.ttf", 15.0f);
	io.Fonts->AddFontFromFileTTF("fonts/Karla-Regular.ttf", 15.0f);
	io.Fonts->AddFontFromFileTTF("fonts/ProggyClean.ttf", 15.0f);
	io.Fonts->AddFontFromFileTTF("fonts/ProggyTiny.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Load INI Config
    mINI::INIFile file("sdh_config.ini");
    mINI::INIStructure ini;
    file.read(ini);

    // Load Textures
	int my_image_width = 0;
	int my_image_height = 0;
	GLuint my_image_texture = 0;
	GLuint gamelink_video_texture = 0;

    // Our state
    bool show_demo_window = false;
    bool show_commands_window = false;
	bool show_tileset_window = false;
	bool show_gamelink_video_window = true;
    bool is_gamelink_focused = false;
	ImGuiFileDialog instance_a;
	ImGuiFileDialog dialog_data;
	ImGuiFileDialog dialog_image0;
	ImGuiFileDialog dialog_image1;

    std::string asset_name = ini["Assets"]["Dialog1"];  // TODO: Remove
    {
		bool ret = ImageHelper::LoadTextureFromFile(asset_name.c_str(), &my_image_texture, &my_image_width, &my_image_height);
    }

    std::string data_filename = ini["Data"]["Data_filename"];
	int data_dest_addr_med = 0;
	int data_dest_addr_high = 0;
	std::string image0_filename = ini["Image"]["Image0_filename"];
    int image0_asset_index = 0;
	std::string image1_filename = ini["Image"]["Image1_filename"];
    int image1_asset_index = 1;
	int tileset0_index = 0;
    int tileset0_asset_index = 0;
	int tileset0_num_entries = 256;
	int tileset0_xdim = 16;
    int tileset0_ydim = 16;
	int tileset1_index = 1;
	int tileset1_asset_index = 0;
	int tileset1_num_entries = 256;
	int tileset1_xdim = 16;
	int tileset1_ydim = 16;
    int window0_index = 0;
    bool window0_black_or_wrap = false;
	int window0_screen_xcount = 640;
	int window0_screen_ycount = 360;
	int window0_screen_xbegin = 0;
	int window0_screen_ybegin = 0;
	int window0_tile_xbegin = 0;
	int window0_tile_ybegin = 0;
	int window0_tile_xdim = 16;
	int window0_tile_ydim = 16;
	int window0_tile_xcount = 256;
	int window0_tile_ycount = 256;
    int window1_index = 1;
	bool window1_black_or_wrap = false;
	int window1_screen_xcount = 640;
	int window1_screen_ycount = 360;
	int window1_screen_xbegin = 160;
	int window1_screen_ybegin = 160;
	int window1_tile_xbegin = 0;
	int window1_tile_ybegin = 0;
	int window1_tile_xdim = 16;
	int window1_tile_ydim = 16;
	int window1_tile_xcount = 1;
	int window1_tile_ycount = 1;
    try
    {
		data_dest_addr_med = std::stoi(ini["Data"]["Data_dest_addr_med"]);
		data_dest_addr_high = std::stoi(ini["Data"]["Data_dest_addr_high"]);
        image0_asset_index = std::stoi(ini["Image"]["Image0_asset_index"]);
		image1_asset_index = std::stoi(ini["Image"]["Image1_asset_index"]);
        tileset0_index = std::stoi(ini["Tileset"]["Tileset0_index"]);
        tileset0_asset_index = std::stoi(ini["Tileset"]["Tileset0_asset_index"]);
        tileset0_num_entries = std::stoi(ini["Tileset"]["Tileset0_num_entries"]);
        tileset0_xdim = std::stoi(ini["Tileset"]["Tileset0_xdim"]);
        tileset0_ydim = std::stoi(ini["Tileset"]["Tileset0_ydim"]);
		tileset1_index = std::stoi(ini["Tileset"]["Tileset1_index"]);
		tileset1_asset_index = std::stoi(ini["Tileset"]["Tileset1_asset_index"]);
		tileset1_num_entries = std::stoi(ini["Tileset"]["Tileset1_num_entries"]);
		tileset1_xdim = std::stoi(ini["Tileset"]["Tileset1_xdim"]);
		tileset1_ydim = std::stoi(ini["Tileset"]["Tileset1_ydim"]);
        window0_index = std::stoi(ini["Window"]["Window0_index"]);
		window0_black_or_wrap = std::stoi(ini["Window"]["Window0_black_or_wrap"]);
		window0_screen_xcount = std::stoi(ini["Window"]["Window0_screen_xcount"]);
		window0_screen_ycount = std::stoi(ini["Window"]["Window0_screen_ycount"]);
		window0_screen_xbegin = std::stoi(ini["Window"]["Window0_screen_xbegin"]);
		window0_screen_ybegin = std::stoi(ini["Window"]["Window0_screen_ybegin"]);
		window0_tile_xbegin = std::stoi(ini["Window"]["Window0_tile_xbegin"]);
		window0_tile_ybegin = std::stoi(ini["Window"]["Window0_tile_ybegin"]);
		window0_tile_xdim = std::stoi(ini["Window"]["Window0_tile_xdim"]);
		window0_tile_ydim = std::stoi(ini["Window"]["Window0_tile_ydim"]);
		window0_tile_xcount = std::stoi(ini["Window"]["Window0_tile_xcount"]);
		window0_tile_ycount = std::stoi(ini["Window"]["Window0_tile_ycount"]);
		window1_index = std::stoi(ini["Window"]["Window1_index"]);
		window1_black_or_wrap = std::stoi(ini["Window"]["Window1_black_or_wrap"]);
		window1_screen_xcount = std::stoi(ini["Window"]["Window1_screen_xcount"]);
		window1_screen_ycount = std::stoi(ini["Window"]["Window1_screen_ycount"]);
		window1_screen_xbegin = std::stoi(ini["Window"]["Window1_screen_xbegin"]);
		window1_screen_ybegin = std::stoi(ini["Window"]["Window1_screen_ybegin"]);
		window1_tile_xbegin = std::stoi(ini["Window"]["Window1_tile_xbegin"]);
		window1_tile_ybegin = std::stoi(ini["Window"]["Window1_tile_ybegin"]);
		window1_tile_xdim = std::stoi(ini["Window"]["Window1_tile_xdim"]);
		window1_tile_ydim = std::stoi(ini["Window"]["Window1_tile_ydim"]);
		window1_tile_xcount = std::stoi(ini["Window"]["Window1_tile_xcount"]);
		window1_tile_ycount = std::stoi(ini["Window"]["Window1_tile_ycount"]);
    }
    catch (const std::exception& e)
    {
    	
    }




    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // GameLink State
    bool activate_gamelink = false;
	bool activate_sdhr = false;

    int64_t tile_posx = 560;  // coords of iolo's hut
    int64_t tile_posy = 832;

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
		ImGui::PushFont(myFont);

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("GameLink Configuration");

            ImGui::SeparatorText("SuperDuper High Resolution Testing");               // Display some text (you can use a format strings too)
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
                {
                    GameLink::SDHR_on();
                    show_commands_window = true;
                }
                else
                {
                    GameLink::SDHR_off();
					show_commands_window = false;
                }
            }
			ImGui::SeparatorText("SDHD Commands");
   //         ImGui::InputText("Asset", &asset_name);
			//ImGui::SameLine();
			//if (ImGui::Button("Select File"))
			//{
			//	instance_a.OpenDialog("ChooseFileDlgKey", "Choose File", ".png", "./Assets");
			//}

            if (ImGui::Button("Define Structs"))
            {
                //// hacky code used to create data file for britannia map
                //std::ofstream f("Assets/britannia.dat", std::ios::out | std::ios::binary | std::ios::trunc);
                //for (auto i = 0; i < sizeof(britannia_tiles); ++i) {
                //    f.put(0);
                //    f.put(brit_lookup()[britannia_tiles[i]]);
                //}
                //f.close();
                auto batcher = SDHRCommandBatcher();

                std::string asset_name = "D:/repos/SuperDuperHelper/SuperDuperHelper/Assets/Tiles_Ultima5.png";
                DefineImageAssetFilenameCmd asset_cmd;
                asset_cmd.asset_index = 0;
                asset_cmd.filename_length = asset_name.length();
                asset_cmd.filename = asset_name.c_str();
                auto assetc = SDHRCommand_DefineImageAssetFilename(&asset_cmd);
                batcher.AddCommand(&assetc);

                std::string tilefile = "D:/repos/SuperDuperHelper/SuperDuperHelper/Assets/britannia.dat";
                UploadDataFilenameCmd upload_tiles;
                upload_tiles.dest_addr_med = 0;
                upload_tiles.dest_addr_high = 0;
                upload_tiles.filename_length = tilefile.length();
                upload_tiles.filename = tilefile.c_str();
                auto upload_tiles_cmd = SDHRCommand_UploadDataFilename(&upload_tiles);
                batcher.AddCommand(&upload_tiles_cmd);

                std::vector<uint16_t> set1_addresses;
                std::vector<uint16_t> set2_addresses;
                for (auto i = 0; i < 256; ++i) {
                    set1_addresses.push_back(i % 32); // x coordinate of tile from PNG
                    set2_addresses.push_back(i % 32);
                    set1_addresses.push_back(i / 32); // y coordinate of tile from PNG
                    set2_addresses.push_back(8 + (i / 32));
                }

                DefineTilesetImmediateCmd set1;
                set1.asset_index = 0;
                set1.tileset_index = 0;
                set1.num_entries = 0; // 0 means 256
                set1.xdim = 16;
                set1.ydim = 16;
                set1.data = (uint8_t*)set1_addresses.data();
                auto set1_cmd = SDHRCommand_DefineTilesetImmediate(&set1);
                batcher.AddCommand(&set1_cmd);

                DefineTilesetImmediateCmd set2;
                set2.asset_index = 0;
                set2.tileset_index = 1;
                set2.num_entries = 0; // 0 means 256
                set2.xdim = 16;
                set2.ydim = 16;
                set2.data = (uint8_t*)set2_addresses.data();
                auto set2_cmd = SDHRCommand_DefineTilesetImmediate(&set2);
                batcher.AddCommand(&set2_cmd);

                DefineWindowCmd w;
                w.window_index = 0;
                w.black_or_wrap = false;
                w.screen_xcount = 336;
                w.screen_ycount = 336;
                w.screen_xbegin = 0;
                w.screen_ybegin = 0;
                w.tile_xbegin = tile_posx;
                w.tile_ybegin = tile_posy;
                w.tile_xdim = set1.xdim;
                w.tile_ydim = set1.ydim;
                w.tile_xcount = 256;
                w.tile_ycount = 256;
                auto w_cmd = SDHRCommand_DefineWindow(&w);
                batcher.AddCommand(&w_cmd);

                DefineWindowCmd w2;
                w2.window_index = 1;
                w2.black_or_wrap = false;
                w2.screen_xcount = 16;
                w2.screen_ycount = 16;
                w2.screen_xbegin = 160;
                w2.screen_ybegin = 160;
                w2.tile_xbegin = 0;
                w2.tile_ybegin = 0;
                w2.tile_xdim = set2.xdim;
                w2.tile_ydim = set2.ydim;
                w2.tile_xcount = 1;
                w2.tile_ycount = 1;
                auto w2_cmd = SDHRCommand_DefineWindow(&w2);
                batcher.AddCommand(&w2_cmd);

                UpdateWindowSetUploadCmd set_tiles;
                set_tiles.window_index = 0;
                set_tiles.tile_xbegin = 0;
                set_tiles.tile_ybegin = 0;
                set_tiles.tile_xcount = w.tile_xcount;
                set_tiles.tile_ycount = w.tile_ycount;
                set_tiles.upload_addr_med = 0;
                set_tiles.upload_addr_high = 0;
                auto set_tiles_cmd = SDHRCommand_UpdateWindowSetUpload(&set_tiles);
                batcher.AddCommand(&set_tiles_cmd);

                std::array<uint8_t, 2> avatar_tile = { 1, 28 };
                UpdateWindowSetBothCmd set_tiles2;
                set_tiles2.window_index = 1;
                set_tiles2.tile_xbegin = 0;
                set_tiles2.tile_ybegin = 0;
                set_tiles2.tile_xcount = 1;
                set_tiles2.tile_ycount = 1;
                set_tiles2.data = avatar_tile.data();
                auto set_tiles2_cmd = SDHRCommand_UpdateWindowSetBoth(&set_tiles2);
                batcher.AddCommand(&set_tiles2_cmd);

                UpdateWindowEnableCmd w_enable;
                w_enable.window_index = 0;
                w_enable.enabled = true;
                auto w_enable_cmd = SDHRCommand_UpdateWindowEnable(&w_enable);
                batcher.AddCommand(&w_enable_cmd);

                UpdateWindowEnableCmd w_enable2;
                w_enable2.window_index = 1;
                w_enable2.enabled = true;
                auto w_enable2_cmd = SDHRCommand_UpdateWindowEnable(&w_enable2);
                batcher.AddCommand(&w_enable2_cmd);

                batcher.Publish();
            }

            UpdateWindowAdjustWindowViewCmd scWP;
            scWP.window_index = 0;
            scWP.tile_xbegin = tile_posx;
            scWP.tile_ybegin = tile_posy;
			//ImGui::SeparatorText("North");
			//static int tile_pos_abs_h = tile_posx;
   //         if (ImGui::SliderInt("Move North", &tile_pos_abs_h, 0, 255))
   //         {
   //             struct UpdateWindowAdjustWindowViewCmd {
   //                 int8_t window_index;
   //                 int64_t tile_xbegin;
   //                 int64_t tile_ybegin;
   //             };
   //             scWP.screen_xbegin = sprite_pos_abs_h;
			//	auto batcher = SDHRCommandBatcher();
			//	auto c1 = SDHRCommand_UpdateWindowSetWindowPosition(&scWP);
			//	batcher.AddCommand(&c1);
			//	batcher.Publish();
   //         }
			//static int sprite_pos_abs_v = sprite_posy;
			//if (ImGui::SliderInt("Move Sprite Vertical", &sprite_pos_abs_v, 0, 360))
			//{
			//	scWP.screen_ybegin = sprite_pos_abs_v;
			//	auto batcher = SDHRCommandBatcher();
			//	auto c1 = SDHRCommand_UpdateWindowSetWindowPosition(&scWP);
			//	batcher.AddCommand(&c1);
			//	batcher.Publish();
			//}

            if (ImGui::Button("North"))
            {
                for (auto i = 0; i < 8; ++i) {
                    auto batcher = SDHRCommandBatcher();
                    tile_posy -= 2;
                    scWP.tile_ybegin = tile_posy;
                    auto c1 = SDHRCommand_UpdateWindowAdjustWindowView(&scWP);
                    batcher.AddCommand(&c1);
                    batcher.Publish();
                }
            }
            if (ImGui::Button("South"))
            {
                for (auto i = 0; i < 8; ++i) {
                    auto batcher = SDHRCommandBatcher();
                    tile_posy += 2;
                    scWP.tile_ybegin = tile_posy;
                    auto c1 = SDHRCommand_UpdateWindowAdjustWindowView(&scWP);
                    batcher.AddCommand(&c1);
                    batcher.Publish();
                }
            }
            if (ImGui::Button("East"))
            {
                for (auto i = 0; i < 8; ++i) {
                    auto batcher = SDHRCommandBatcher();
                    tile_posx += 2;
                    scWP.tile_xbegin = tile_posx;
                    auto c1 = SDHRCommand_UpdateWindowAdjustWindowView(&scWP);
                    batcher.AddCommand(&c1);
                    batcher.Publish();
                }
            }
            if (ImGui::Button("West"))
            {
                for (auto i = 0; i < 8; ++i) {
                    auto batcher = SDHRCommandBatcher();
                    tile_posx -= 2;
                    scWP.tile_xbegin = tile_posx;
                    auto c1 = SDHRCommand_UpdateWindowAdjustWindowView(&scWP);
                    batcher.AddCommand(&c1);
                    batcher.Publish();
                }
            }

			if (ImGui::Button("Reset"))
				GameLink::SDHR_reset();

			if (!activate_gamelink)
				ImGui::EndDisabled();

            ImGui::NewLine();

			ImGui::SeparatorText("Other");

			if (instance_a.Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(200,200), ImVec2(2000,2000)))
			{
				// action if OK
				if (instance_a.IsOk())
				{
					asset_name = instance_a.GetFilePathName();
					std::string filePath = instance_a.GetCurrentPath();
					bool ret = ImageHelper::LoadTextureFromFile(asset_name.c_str(), &my_image_texture, &my_image_width, &my_image_height);
                    ini["Assets"]["Dialog1"] = asset_name;
                    file.write(ini);
                    show_tileset_window = true;
				}

				// close
				instance_a.Close();
			}

			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state


            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show the commands window
        if (show_commands_window)
        {
			ImVec2 vsize = ImVec2(300.f, 200.f);
            ImGui::SetNextWindowSize(vsize, ImGuiCond_FirstUseEver);
            ImGui::Begin("Commands Window", &show_commands_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			if (ImGui::CollapsingHeader("Data Upload"))
			{
                ImGui::Text(data_filename.c_str());
				ImGui::SameLine();
                if (ImGui::Button("Select File"))
                {
                    dialog_data.OpenDialog("ChooseAssetDlgKey", "Select File", ".*", "./Assets", -1, nullptr,
                        ImGuiFileDialogFlags_NoDialog |
                        ImGuiFileDialogFlags_DisableCreateDirectoryButton |
                        ImGuiFileDialogFlags_ReadOnlyFileNameField);
                }
                ImGui::SliderInt("Med Byte", &data_dest_addr_med, 0, 255);
				ImGui::SliderInt("High Byte", &data_dest_addr_high, 0, 255);
				if (dialog_data.Display("ChooseAssetDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(0, 0), ImVec2(0, 250)))
				{
					// action if OK
					if (dialog_data.IsOk())
					{
                        data_filename = dialog_data.GetFilePathName();
					}
					// close
                    dialog_data.Close();
				}
				if (ImGui::Button("Process Data Upload"))
				{
					ini["Data"]["Data_dest_addr_med"] = data_dest_addr_med;
					ini["Data"]["Data_dest_addr_high"] = data_dest_addr_high;
					ini["Data"]["Data_filename"] = data_filename;
					file.write(ini);
					auto batcher = SDHRCommandBatcher();
                    UploadDataFilenameCmd _udc;
                    _udc.dest_addr_med = (uint8_t)data_dest_addr_med;
					_udc.dest_addr_high = (uint8_t)data_dest_addr_high;
                    _udc.filename_length = (uint8_t)data_filename.length();
                    _udc.filename = data_filename.c_str();
					auto _cmd = SDHRCommand_UploadDataFilename(&_udc);
					batcher.AddCommand(&_cmd);
					batcher.Publish();
				}
			}
			if (ImGui::CollapsingHeader("Image Asset 0"))
			{
				ImGui::Text(image0_filename.c_str());
				ImGui::SameLine();
				if (ImGui::Button("Select File"))
				{
					dialog_data.OpenDialog("ChooseImage0DlgKey", "Select File", ".*", "./Assets", -1, nullptr,
						ImGuiFileDialogFlags_NoDialog |
						ImGuiFileDialogFlags_DisableCreateDirectoryButton |
						ImGuiFileDialogFlags_ReadOnlyFileNameField);
				}
				if (dialog_data.Display("ChooseImage0DlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(0, 0), ImVec2(0, 250)))
				{
					// action if OK
					if (dialog_data.IsOk())
					{
						image0_filename = dialog_data.GetFilePathName();
					}
					// close
					dialog_data.Close();
				}
				if (ImGui::Button("Process Image 0"))
				{
					ini["Image"]["Image0_asset_index"] = image0_asset_index;
					ini["Image"]["Image0_filename"] = image0_filename;
					file.write(ini);
					auto batcher = SDHRCommandBatcher();
					DefineImageAssetFilenameCmd _udc;
					_udc.asset_index = (uint8_t)image0_asset_index;
					_udc.filename_length = (uint8_t)image0_filename.length();
					_udc.filename = image0_filename.c_str();
					auto _cmd = SDHRCommand_DefineImageAssetFilename(&_udc);
					batcher.AddCommand(&_cmd);
					batcher.Publish();
				}
			}
			if (ImGui::CollapsingHeader("Image Asset 1"))

			{
				ImGui::Text(image1_filename.c_str());
				ImGui::SameLine();
				if (ImGui::Button("Select File"))
				{
					dialog_data.OpenDialog("ChooseImage1DlgKey", "Select File", ".*", "./Assets", -1, nullptr,
						ImGuiFileDialogFlags_NoDialog |
						ImGuiFileDialogFlags_DisableCreateDirectoryButton |
						ImGuiFileDialogFlags_ReadOnlyFileNameField);
				}
				if (dialog_data.Display("ChooseImage1DlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(0, 0), ImVec2(0, 250)))
				{
					// action if OK
					if (dialog_data.IsOk())
					{
						image1_filename = dialog_data.GetFilePathName();
					}
					// close
					dialog_data.Close();
				}
				if (ImGui::Button("Process Image 1"))
				{
					ini["Image"]["Image1_asset_index"] = image1_asset_index;
					ini["Image"]["Image1_filename"] = image1_filename;
					file.write(ini);
					auto batcher = SDHRCommandBatcher();
					DefineImageAssetFilenameCmd _udc;
					_udc.asset_index = (uint8_t)image0_asset_index;
					_udc.filename_length = (uint8_t)image1_filename.length();
					_udc.filename = image1_filename.c_str();
					auto _cmd = SDHRCommand_DefineImageAssetFilename(&_udc);
					batcher.AddCommand(&_cmd);
					batcher.Publish();
				}
			}
			if (ImGui::CollapsingHeader("Tileset 0"))
			{
                if (tileset0_num_entries == 0)
                    tileset0_num_entries = 256;
				ImGui::SliderInt("Asset Index", &tileset0_index, 0, 1);
                int isq = std::round(sqrt(tileset0_num_entries));
				if (ImGui::SliderInt("Number of Entries", &tileset0_num_entries, 1, 256))
				{
					// Check it's a square
					isq = std::round(sqrt(tileset0_num_entries));
					if (isq * isq != tileset0_num_entries)
						tileset0_num_entries = isq * isq;
				}
                ImGui::SliderInt("X Dimension", &tileset0_xdim, 1, 256);
				ImGui::SliderInt("Y Dimension", &tileset0_ydim, 1, 256);
				if (ImGui::Button("Process Tileset 0"))
				{
					ini["Tileset"]["Tileset0_index"] = tileset0_index;
					ini["Tileset"]["Tileset0_asset_index"] = tileset0_asset_index;
					ini["Tileset"]["Tileset0_num_entries"] = tileset0_num_entries;
					ini["Tileset"]["Tileset0_xdim"] = tileset0_xdim;
					ini["Tileset"]["Tileset0_ydim"] = tileset0_ydim;
					file.write(ini);
					auto batcher = SDHRCommandBatcher();
                    DefineTilesetImmediateCmd _udc;
					_udc.tileset_index = (uint8_t)tileset0_index;
					_udc.num_entries = (uint8_t)tileset0_num_entries;   // 256 becomes 0
					_udc.xdim = (uint8_t)tileset0_xdim;   // 256 becomes 0
					_udc.ydim = (uint8_t)tileset0_ydim;   // 256 becomes 0
					_udc.asset_index = tileset0_asset_index;

                    // Create the data. We assume here that the tileset is square
					std::vector<uint16_t> set_addresses;
					for (auto i = 0; i < 256; ++i) {
						set_addresses.push_back(i % isq); // x coordinate of tile from PNG
						set_addresses.push_back(i / isq); // y coordinate of tile from PNG
					}
					_udc.data = (uint8_t*)set_addresses.data();
					auto _cmd = SDHRCommand_DefineTilesetImmediate(&_udc);
					batcher.AddCommand(&_cmd);
					batcher.Publish();
				}
			}
			if (ImGui::CollapsingHeader("Tileset 1"))
			{
				if (tileset1_num_entries == 0)
					tileset1_num_entries = 256;
				ImGui::SliderInt("Asset Index", &tileset1_index, 0, 1);
				int isq = std::round(sqrt(tileset1_num_entries));
                if (ImGui::SliderInt("Number of Entries", &tileset1_num_entries, 1, 256))
                {
                    // Check it's a square
					isq = std::round(sqrt(tileset1_num_entries));
                    if (isq * isq != tileset1_num_entries)
                        tileset1_num_entries = isq * isq;
                }
				ImGui::SliderInt("X Dimension", &tileset1_xdim, 1, 256);
				ImGui::SliderInt("Y Dimension", &tileset1_ydim, 1, 256);
				if (ImGui::Button("Process Tileset 1"))
				{
					ini["Tileset"]["Tileset0_index"] = tileset1_index;
					ini["Tileset"]["Tileset0_asset_index"] = tileset1_asset_index;
					ini["Tileset"]["Tileset0_num_entries"] = tileset1_num_entries;
					ini["Tileset"]["Tileset0_xdim"] = tileset1_xdim;
					ini["Tileset"]["Tileset0_ydim"] = tileset1_ydim;
					file.write(ini);
					auto batcher = SDHRCommandBatcher();
					DefineTilesetImmediateCmd _udc;
					_udc.tileset_index = (uint8_t)tileset1_index;
					_udc.num_entries = (uint8_t)tileset1_num_entries;   // 256 becomes 0
					_udc.xdim = (uint8_t)tileset1_xdim;   // 256 becomes 0
					_udc.ydim = (uint8_t)tileset1_ydim;   // 256 becomes 0
					_udc.asset_index = tileset1_asset_index;

					// Create the data. We assume here that the tileset is square
					std::vector<uint16_t> set_addresses;
					for (auto i = 0; i < 256; ++i) {
						set_addresses.push_back(i % isq); // x coordinate of tile from PNG
						set_addresses.push_back(i / isq); // y coordinate of tile from PNG
					}
					_udc.data = (uint8_t*)set_addresses.data();
					auto _cmd = SDHRCommand_DefineTilesetImmediate(&_udc);
					batcher.AddCommand(&_cmd);
					batcher.Publish();
				}
			}
			if (ImGui::CollapsingHeader("Window 0"))
			{
                // TODO: Enable/Disable window with a button
                ImGui::SeparatorText("Define Window");
				ImGui::Checkbox("Wrap", &window0_black_or_wrap);
                ImGui::PushItemWidth(80.f);
				ImGui::Text("Screen Pixels:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &window0_screen_xcount); ImGui::SameLine(240); ImGui::InputInt(" px", &window0_screen_ycount);
                ImGui::Text("Screen Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &window0_screen_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px", &window0_screen_ybegin);
				ImGui::Text("Tile Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &window0_tile_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px", &window0_tile_ybegin);
				ImGui::Text("Tile Dimensions:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &window0_tile_xdim); ImGui::SameLine(240); ImGui::InputInt(" px", &window0_tile_ydim);
				ImGui::Text("Tile Count:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &window0_tile_xcount); ImGui::SameLine(240); ImGui::InputInt(" tiles", &window0_tile_ycount);
				ImGui::PopItemWidth();
				if (ImGui::Button("Define Window 0"))
				{
					ini["Window"]["Window0_index"] = window0_index;
					ini["Window"]["Window0_black_or_wrap"] = window0_black_or_wrap;
					ini["Window"]["Window0_screen_xcount"] = window0_screen_xcount;
					ini["Window"]["Window0_screen_ycount"] = window0_screen_ycount;
					ini["Window"]["Window0_screen_xbegin"] = window0_screen_xbegin;
					ini["Window"]["Window0_screen_ybegin"] = window0_screen_ybegin;
					ini["Window"]["Window0_tile_xbegin"] = window0_tile_xbegin;
					ini["Window"]["Window0_tile_ybegin"] = window0_tile_ybegin;
					ini["Window"]["Window0_tile_xdim"] = window0_tile_xdim;
					ini["Window"]["Window0_tile_ydim"] = window0_tile_ydim;
					ini["Window"]["Window0_tile_xcount"] = window0_tile_xcount;
					ini["Window"]["Window0_tile_ycount"] = window0_tile_ycount;
					file.write(ini);
					auto batcher = SDHRCommandBatcher();
                    DefineWindowCmd _udc;
					_udc.window_index = window0_index;
					_udc.black_or_wrap = window0_black_or_wrap;
					_udc.screen_xcount = window0_screen_xcount;
					_udc.screen_ycount = window0_screen_ycount;
					_udc.screen_xbegin = window0_screen_xbegin;
					_udc.screen_ybegin = window0_screen_ybegin;
					_udc.tile_xbegin = window0_tile_xbegin;
					_udc.tile_ybegin = window0_tile_ybegin;
					_udc.tile_xdim = window0_tile_xdim;
					_udc.tile_ydim = window0_tile_ydim;
					_udc.tile_xcount = window0_tile_xcount;
					_udc.tile_ycount = window0_tile_ycount;
					auto _cmd = SDHRCommand_DefineWindow(&_udc);
					batcher.AddCommand(&_cmd);
					// auto-enable the window
					UpdateWindowEnableCmd w_enable;
					w_enable.window_index = 0;
					w_enable.enabled = true;
					auto w_enable_cmd = SDHRCommand_UpdateWindowEnable(&w_enable);
					batcher.AddCommand(&w_enable_cmd);
					batcher.Publish();
				}
			}
			if (ImGui::CollapsingHeader("Window 1"))
			{
				// TODO: Enable/Disable window with a button
								// TODO: Enable/Disable window with a button
				ImGui::SeparatorText("Define Window");
				ImGui::Checkbox("Wrap", &window1_black_or_wrap);
				ImGui::PushItemWidth(80.f);
				ImGui::Text("Screen Pixels:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &window1_screen_xcount); ImGui::SameLine(240); ImGui::InputInt(" px", &window1_screen_ycount);
				ImGui::Text("Screen Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &window1_screen_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px", &window1_screen_ybegin);
				ImGui::Text("Tile Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &window1_tile_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px", &window1_tile_ybegin);
				ImGui::Text("Tile Dimensions:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &window1_tile_xdim); ImGui::SameLine(240); ImGui::InputInt(" px", &window1_tile_ydim);
				ImGui::Text("Tile Count:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &window1_tile_xcount); ImGui::SameLine(240); ImGui::InputInt(" tiles", &window1_tile_ycount);
				ImGui::PopItemWidth();
				if (ImGui::Button("Define Window 0"))
				{
					ini["Window"]["Window1_index"] = window1_index;
					ini["Window"]["Window1_black_or_wrap"] = window1_black_or_wrap;
					ini["Window"]["Window1_screen_xcount"] = window1_screen_xcount;
					ini["Window"]["Window1_screen_ycount"] = window1_screen_ycount;
					ini["Window"]["Window1_screen_xbegin"] = window1_screen_xbegin;
					ini["Window"]["Window1_screen_ybegin"] = window1_screen_ybegin;
					ini["Window"]["Window1_tile_xbegin"] = window1_tile_xbegin;
					ini["Window"]["Window1_tile_ybegin"] = window1_tile_ybegin;
					ini["Window"]["Window1_tile_xdim"] = window1_tile_xdim;
					ini["Window"]["Window1_tile_ydim"] = window1_tile_ydim;
					ini["Window"]["Window1_tile_xcount"] = window1_tile_xcount;
					ini["Window"]["Window1_tile_ycount"] = window1_tile_ycount;
					file.write(ini);
					auto batcher = SDHRCommandBatcher();
					DefineWindowCmd _udc;
					_udc.window_index = window1_index;
					_udc.black_or_wrap = window1_black_or_wrap;
					_udc.screen_xcount = window1_screen_xcount;
					_udc.screen_ycount = window1_screen_ycount;
					_udc.screen_xbegin = window1_screen_xbegin;
					_udc.screen_ybegin = window1_screen_ybegin;
					_udc.tile_xbegin = window1_tile_xbegin;
					_udc.tile_ybegin = window1_tile_ybegin;
					_udc.tile_xdim = window1_tile_xdim;
					_udc.tile_ydim = window1_tile_ydim;
					_udc.tile_xcount = window1_tile_xcount;
					_udc.tile_ycount = window1_tile_ycount;
					auto _cmd = SDHRCommand_DefineWindow(&_udc);
					batcher.AddCommand(&_cmd);
					// auto-enable the window
					UpdateWindowEnableCmd w_enable;
					w_enable.window_index = 1;
					w_enable.enabled = true;
					auto w_enable_cmd = SDHRCommand_UpdateWindowEnable(&w_enable);
					batcher.AddCommand(&w_enable_cmd);
					batcher.Publish();
				}
			}
			ImGui::SeparatorText("Window Commands - Applied on a window index");
			static int _vWindowIndex = 0;
			ImGui::PushItemWidth(80.f);
			ImGui::InputInt("Window Index", &_vWindowIndex);
			ImGui::PopItemWidth();
			if (ImGui::CollapsingHeader("Update Window: Enable"))
			{

				int _bState = 0;
				if (ImGui::Button("Enable Window"))
				{
					_bState = 2;
				}
				ImGui::SameLine();
				if (ImGui::Button("Disable Window"))
				{
					_bState = 1;

				}
				if (_bState > 0)
				{
					auto batcher = SDHRCommandBatcher();
					UpdateWindowEnableCmd w_enable;
					w_enable.window_index = _vWindowIndex;
					w_enable.enabled = _bState - 1;
					auto w_enable_cmd = SDHRCommand_UpdateWindowEnable(&w_enable);
					batcher.AddCommand(&w_enable_cmd);
					batcher.Publish();
				}
			}
			if (ImGui::CollapsingHeader("Update Window: Set Upload"))
			{
				static int _uwsu_tile_xbegin;
				static int _uwsu_tile_ybegin;
				static int _uwsu_tile_xcount;
				static int _uwsu_tile_ycount;
				static int _uwsu_addr_med;
				static int _uwsu_addr_high;
				ImGui::PushItemWidth(80.f);
				ImGui::Text("Tile Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &_uwsu_tile_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px", &window0_tile_ybegin);
				ImGui::Text("Tile Count:");  ImGui::SameLine(130);
				ImGui::InputInt(" ", &_uwsu_tile_xcount); ImGui::SameLine(240); ImGui::InputInt(" tiles", &_uwsu_tile_ycount);
				ImGui::PopItemWidth();
				ImGui::SliderInt("Med Byte", &_uwsu_addr_med, 0, 255);
				ImGui::SliderInt("High Byte", &_uwsu_addr_high, 0, 255);
				if (ImGui::Button("Update"))
				{
					auto batcher = SDHRCommandBatcher();
					UpdateWindowSetUploadCmd _wcmd;
					_wcmd.window_index = _vWindowIndex;
					_wcmd.tile_xbegin = _uwsu_tile_xbegin;
					_wcmd.tile_ybegin = _uwsu_tile_ybegin;
					_wcmd.upload_addr_med = _uwsu_addr_med;
					_wcmd.upload_addr_high = _uwsu_addr_high;
					auto w_enable_cmd = SDHRCommand_UpdateWindowSetUpload(&_wcmd);
					batcher.AddCommand(&w_enable_cmd);
					batcher.Publish();
				}
			}
			if (ImGui::CollapsingHeader("Update Window: Single Tileset"))
			{

			}
			if (ImGui::CollapsingHeader("Update Window: Set Both"))
			{

			}
			if (ImGui::CollapsingHeader("Update Window: Shift Tiles"))
			{

			}
			if (ImGui::CollapsingHeader("Update Window: Set Window Position"))
			{

			}
			if (ImGui::CollapsingHeader("Update Window: Adjust Window View"))
			{

			}
            ImGui::End();
        }

        // 4. Show texture in a window
        if (show_tileset_window)
		{
            ImVec2 vpos = ImVec2(300.f, 100.f);
            ImGui::SetNextWindowPos(vpos, ImGuiCond_FirstUseEver);
			ImGui::Begin("Loaded PNG Asset", &show_tileset_window);
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

		ImGui::PopFont();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        if (show_gamelink_video_window && activate_gamelink)
        {
            glDeleteTextures(1, &gamelink_video_texture);
        }
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
