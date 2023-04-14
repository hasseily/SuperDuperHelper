// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#define WIN32_LEAN_AND_MEAN
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
#include <filesystem>
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
#include "ImGuiFileDialog/stb/stb_image.h"
#pragma comment(lib, "ws2_32.lib")

std::map<int, bool> keyboard; // Saves the state(true=pressed; false=released) of each SDL_Key.

// We're moving data one page (0x0100) at a time, and we're allowed to use the Apple 2's memory
// from 0x0200 to 0xBFFF.
#define APPLE2_MEM_LOW	 0x02	//	Lower bound (med byte) of Apple 2 memory allowed to use
#define APPLE2_MEM_HIGH	 0xBF	//	Upper bound (med byte) of Apple 2 memory allowed to use

static uint8_t UploadDataFilename(UploadDataFilenameCmd* cmd, const uint8_t source_addr_med,
	SDHRCommandBatcher* b)
{
	if (cmd->filename_length == 0)
		return 0;
	if (source_addr_med < APPLE2_MEM_LOW)
	{
		std::cerr << "Apple 2 memory source address is too low!" << std::endl;
		std::cerr << std::hex << source_addr_med << std::endl;
		return 0;
	}
	if (source_addr_med >= APPLE2_MEM_HIGH)
	{
		std::cerr << "Apple 2 memory source address is too high!" << std::endl;
		return 0;
	}
	std::string filename(cmd->filename, cmd->filename + cmd->filename_length);
	std::ifstream f(filename.c_str(), std::ios::binary | std::ios::ate);

	UploadDataCmd upCmd;
	upCmd.source_addr_med = source_addr_med;
	upCmd.upload_addr_med = 0;
	upCmd.upload_addr_high = 0;
	upCmd.num_256b_pages = 1;	// upload 1 256b page at a time

	uint16_t memaddr = upCmd.source_addr_med << 8;

	f.seekg(0, std::ios::beg);
	char fval;
	uint64_t bytes_read = 0;
	uint8_t num_256b_pages = 0;
	b->packet.pad = 0;
	while (!f.eof())
	{
		if (bytes_read % 256 == 0)
		{
			if (num_256b_pages > 0)
			{
				// send each page one at a time
				upCmd.source_addr_med = source_addr_med;
				upCmd.upload_addr_med = (uint8_t)(bytes_read >> 8);
				upCmd.upload_addr_high = (uint8_t)(bytes_read >> 16);
				memaddr = upCmd.source_addr_med << 8;
				auto up_cmd = SDHRCommand_UploadData(&upCmd);
				b->AddCommand(&up_cmd);
				b->SDHR_Process();
			}
			++num_256b_pages;
		}
		// Send the file through the network one byte at a time,
		// as if it were the Apple 2 card bus
		f.read(&fval, sizeof(char));
		b->packet.addr = memaddr++;
		b->packet.data = fval;
		send(b->client_socket, (char*)&b->packet, 4, 0);
		++bytes_read;
	}
	// Fill the rest of the page with 0s
	while (bytes_read % 256 > 0)
	{
		BusPacket bp;
		b->packet.addr = memaddr++;
		bp.data = 0;
		send(b->client_socket, (char*)&b->packet, 4, 0);
		++bytes_read;
	}
	// Send the last page
	upCmd.source_addr_med = source_addr_med;
	upCmd.upload_addr_med = (uint8_t)(bytes_read >> 8);
	upCmd.upload_addr_high = (uint8_t)(bytes_read >> 16);
	memaddr = upCmd.source_addr_med << 8;
	auto up_cmd = SDHRCommand_UploadData(&upCmd);
	b->AddCommand(&up_cmd);
	b->SDHR_Process();
	return num_256b_pages;
}

static void UploadImageFilename(DefineImageAssetFilenameCmd* cmd, const uint8_t source_addr_med,
	const std::string server_ip, const int server_port)
{
	if (cmd->filename_length == 0)
		return;
	if (source_addr_med < APPLE2_MEM_LOW)
	{
		std::cerr << "Apple 2 memory source address is too low!" << std::endl;
		std::cerr << std::hex << source_addr_med << std::endl;
		return;
	}
	if (source_addr_med >= APPLE2_MEM_HIGH)
	{
		std::cerr << "Apple 2 memory source address is too high!" << std::endl;
		return;
	}
	// WARNING: The filename upload stuff doesn't exist on the server.
// Load the file in memory, and send Apple 2 memory write packets.
// It's up to the caller to process the UploadDataCmd
	UploadDataFilenameCmd upfcmd;
	upfcmd.filename = cmd->filename;
	upfcmd.filename_length = cmd->filename_length;
	upfcmd.upload_addr_med = 0;
	upfcmd.upload_addr_high = 0;
	auto b = SDHRCommandBatcher(server_ip, server_port);

	uint8_t num_256b_pages = UploadDataFilename(&upfcmd, source_addr_med, &b);
	if (num_256b_pages == 0)
		return;

	// And now tell the SDHR graphics processor that the file uploaded
	// at 0, with page count num_256b_pages is and image with a specific asset index 
	DefineImageAssetCmd asset_cmd;
	asset_cmd.asset_index = cmd->asset_index;
	asset_cmd.upload_addr_med = 0;
	asset_cmd.upload_addr_high = 0;
	asset_cmd.upload_page_count = num_256b_pages;
	auto assetc = SDHRCommand_DefineImageAsset(&asset_cmd);
	b.AddCommand(&assetc);
	b.SDHR_Process();
}


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

    // Our state
    bool show_demo_window = false;
    bool show_commands_window = false;
	bool show_tileset_window = false;
	ImGuiFileDialog instance_a;
	ImGuiFileDialog dialog_data;
	ImGuiFileDialog dialog_image0;
	ImGuiFileDialog dialog_image1;

	// Socket Server
	static std::string server_ip = ini["Server"]["IP"];
	if (server_ip.length() == 0)
		server_ip = "127.0.0.1";
	static int server_port = 8080;
	try
	{
		server_port = std::stoi(ini["Server"]["Port"]);
	}
	catch (const std::exception& e)
	{

	}

    std::string asset_name = ini["Assets"]["Dialog1"];  // TODO: Remove
    {
		ImageHelper::LoadTextureFromFile(asset_name.c_str(), &my_image_texture, &my_image_width, &my_image_height);
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

            ImGui::Begin("SDHD Configuration");
			ImGui::SeparatorText("Server Connection");
			if (ImGui::InputText("IP##server", &server_ip) || ImGui::InputInt("Port##server", &server_port))
			{
				ini["Server"]["IP"] = server_ip;
				ini["Server"]["Port"] = server_port;
				file.write(ini);
			}

            ImGui::SeparatorText("SuperDuper High Resolution Testing");

            if (ImGui::Checkbox("Enable SuperDuperHiRes (SDHR)", &activate_sdhr))
            {
				auto batcher = SDHRCommandBatcher(server_ip, server_port);
                if (activate_sdhr)
                {
                    batcher.SDHR_On();
                    show_commands_window = true;
                }
                else
                {
					batcher.SDHR_Off();
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

				// Load an image using the UploadImageFilename() helper
				std::filesystem::path asset_path = "Assets/Tiles_Ultima5.png";
				asset_name = std::filesystem::absolute(asset_path).string();
				DefineImageAssetFilenameCmd imgasset_cmd;
				imgasset_cmd.asset_index = 0;
				imgasset_cmd.filename_length = asset_name.length();
				imgasset_cmd.filename = asset_name.c_str();
				UploadImageFilename(&imgasset_cmd, 0x20, server_ip, server_port);

				auto batcher = SDHRCommandBatcher(server_ip, server_port);

				// Load an asset by just uploading the data
				// This one loads at 0x00
				std::filesystem::path tilepath = "Assets/britannia.dat";
                std::string tilefile = std::filesystem::absolute(tilepath).string();
                UploadDataFilenameCmd upload_tilesf;
                upload_tilesf.upload_addr_med = 0;
                upload_tilesf.upload_addr_high = 0;
                upload_tilesf.filename_length = tilefile.length();
                upload_tilesf.filename = tilefile.c_str();
				UploadDataFilename(&upload_tilesf, APPLE2_MEM_LOW, &batcher);

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

                batcher.SDHR_Process();
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
			//	auto batcher = SDHRCommandBatcher(server_ip, server_port);
			//	auto c1 = SDHRCommand_UpdateWindowSetWindowPosition(&scWP);
			//	batcher.AddCommand(&c1);
			//	batcher.SDHR_process();
   //         }
			//static int sprite_pos_abs_v = sprite_posy;
			//if (ImGui::SliderInt("Move Sprite Vertical", &sprite_pos_abs_v, 0, 360))
			//{
			//	scWP.screen_ybegin = sprite_pos_abs_v;
			//	auto batcher = SDHRCommandBatcher(server_ip, server_port);
			//	auto c1 = SDHRCommand_UpdateWindowSetWindowPosition(&scWP);
			//	batcher.AddCommand(&c1);
			//	batcher.SDHR_process();
			//}

            if (ImGui::Button("North"))
            {
                for (auto i = 0; i < 8; ++i) {
                    auto batcher = SDHRCommandBatcher(server_ip, server_port);
                    tile_posy -= 2;
                    scWP.tile_ybegin = tile_posy;
                    auto c1 = SDHRCommand_UpdateWindowAdjustWindowView(&scWP);
                    batcher.AddCommand(&c1);
                    batcher.SDHR_Process();
                }
            }
            if (ImGui::Button("South"))
            {
                for (auto i = 0; i < 8; ++i) {
                    auto batcher = SDHRCommandBatcher(server_ip, server_port);
                    tile_posy += 2;
                    scWP.tile_ybegin = tile_posy;
                    auto c1 = SDHRCommand_UpdateWindowAdjustWindowView(&scWP);
                    batcher.AddCommand(&c1);
                    batcher.SDHR_Process();
                }
            }
            if (ImGui::Button("East"))
            {
                for (auto i = 0; i < 8; ++i) {
                    auto batcher = SDHRCommandBatcher(server_ip, server_port);
                    tile_posx += 2;
                    scWP.tile_xbegin = tile_posx;
                    auto c1 = SDHRCommand_UpdateWindowAdjustWindowView(&scWP);
                    batcher.AddCommand(&c1);
                    batcher.SDHR_Process();
                }
            }
            if (ImGui::Button("West"))
            {
                for (auto i = 0; i < 8; ++i) {
                    auto batcher = SDHRCommandBatcher(server_ip, server_port);
                    tile_posx -= 2;
                    scWP.tile_xbegin = tile_posx;
                    auto c1 = SDHRCommand_UpdateWindowAdjustWindowView(&scWP);
                    batcher.AddCommand(&c1);
                    batcher.SDHR_Process();
                }
            }

			if (ImGui::Button("Reset"))
			{
				auto batcher = SDHRCommandBatcher(server_ip, server_port);
				batcher.SDHR_Reset();
			}

            ImGui::NewLine();

			ImGui::SeparatorText("Other");

			if (instance_a.Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(200,200), ImVec2(2000,2000)))
			{
				// action if OK
				if (instance_a.IsOk())
				{
					asset_name = instance_a.GetFilePathName();
					std::string filePath = instance_a.GetCurrentPath();
					ImageHelper::LoadTextureFromFile(asset_name.c_str(), &my_image_texture, &my_image_width, &my_image_height);
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
                if (ImGui::Button("Select File###"))
                {
                    dialog_data.OpenDialog("ChooseAssetDlgKey", "Select File", ".*", "./Assets", -1, nullptr,
                        ImGuiFileDialogFlags_NoDialog |
                        ImGuiFileDialogFlags_DisableCreateDirectoryButton |
                        ImGuiFileDialogFlags_ReadOnlyFileNameField);
                }
                ImGui::SliderInt("Med Byte###", &data_dest_addr_med, 0, 255);
				ImGui::SliderInt("High Byte###", &data_dest_addr_high, 0, 255);
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
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
                    UploadDataFilenameCmd _udc;
                    _udc.upload_addr_med = (uint8_t)data_dest_addr_med;
					_udc.upload_addr_high = (uint8_t)data_dest_addr_high;
                    _udc.filename_length = (uint8_t)data_filename.length();
                    _udc.filename = data_filename.c_str();
					UploadDataFilename(&_udc, APPLE2_MEM_LOW, &batcher);
				}
			}
			if (ImGui::CollapsingHeader("Image Asset 0"))
			{
				ImGui::Text(image0_filename.c_str());
				ImGui::SameLine();
				if (ImGui::Button("Select File###"))
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
					DefineImageAssetFilenameCmd img0asset_cmd;
					img0asset_cmd.asset_index = image0_asset_index;
					img0asset_cmd.filename_length = image0_filename.length();
					img0asset_cmd.filename = image0_filename.c_str();
					UploadImageFilename(&img0asset_cmd, 0x20, server_ip, server_port);
				}
			}
			if (ImGui::CollapsingHeader("Image Asset 1"))

			{
				ImGui::Text(image1_filename.c_str());
				ImGui::SameLine();
				if (ImGui::Button("Select File###"))
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
					DefineImageAssetFilenameCmd img1asset_cmd;
					img1asset_cmd.asset_index = image1_asset_index;
					img1asset_cmd.filename_length = image1_filename.length();
					img1asset_cmd.filename = image1_filename.c_str();
					UploadImageFilename(&img1asset_cmd, 0x20, server_ip, server_port);
				}
			}
			if (ImGui::CollapsingHeader("Tileset 0"))
			{
                if (tileset0_num_entries == 0)
                    tileset0_num_entries = 256;
				ImGui::SliderInt("Asset Index##t0", &tileset0_index, 0, 1);
                int isq = std::round(sqrt(tileset0_num_entries));
				if (ImGui::SliderInt("Number of Entries##t0", &tileset0_num_entries, 1, 256))
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
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
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
					batcher.SDHR_Process();
				}
			}
			if (ImGui::CollapsingHeader("Tileset 1"))
			{
				if (tileset1_num_entries == 0)
					tileset1_num_entries = 256;
				ImGui::SliderInt("Asset Index##t1", &tileset1_index, 0, 1);
				int isq = std::round(sqrt(tileset1_num_entries));
                if (ImGui::SliderInt("Number of Entries##t1", &tileset1_num_entries, 1, 256))
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
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
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
					batcher.SDHR_Process();
				}
			}
			if (ImGui::CollapsingHeader("Window 0"))
			{
                // TODO: Enable/Disable window with a button
                ImGui::SeparatorText("Define Window");
				ImGui::Checkbox("Wrap##w0", &window0_black_or_wrap);
                ImGui::PushItemWidth(80.f);
				ImGui::Text("Screen Pixels:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##0w0", &window0_screen_xcount); ImGui::SameLine(240); ImGui::InputInt(" px##0w0", &window0_screen_ycount);
                ImGui::Text("Screen Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##1w0", &window0_screen_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px##1w0", &window0_screen_ybegin);
				ImGui::Text("Tile Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##2w0", &window0_tile_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px##2w0", &window0_tile_ybegin);
				ImGui::Text("Tile Dimensions:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##3w0", &window0_tile_xdim); ImGui::SameLine(240); ImGui::InputInt(" px##3w0", &window0_tile_ydim);
				ImGui::Text("Tile Count:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##4w0", &window0_tile_xcount); ImGui::SameLine(240); ImGui::InputInt(" tiles##4w0", &window0_tile_ycount);
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
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
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
					batcher.SDHR_Process();
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
				ImGui::InputInt(" ##0w1", &window1_screen_xcount); ImGui::SameLine(240); ImGui::InputInt(" px##0w1", &window1_screen_ycount);
				ImGui::Text("Screen Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##1w1", &window1_screen_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px##1w1", &window1_screen_ybegin);
				ImGui::Text("Tile Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##2w1", &window1_tile_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px##2w1", &window1_tile_ybegin);
				ImGui::Text("Tile Dimensions:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##3w1", &window1_tile_xdim); ImGui::SameLine(240); ImGui::InputInt(" px##3w1", &window1_tile_ydim);
				ImGui::Text("Tile Count:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##4w1", &window1_tile_xcount); ImGui::SameLine(240); ImGui::InputInt(" tiles##4w1", &window1_tile_ycount);
				ImGui::PopItemWidth();
				if (ImGui::Button("Define Window 1"))
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
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
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
					batcher.SDHR_Process();
				}
			}
			ImGui::SeparatorText("Window Commands - Applied on a window index");
			static int _vWindowIndex = 0;
			ImGui::PushItemWidth(80.f);
			ImGui::InputInt("Window Index##wi", &_vWindowIndex);
			ImGui::PopItemWidth();
			if (ImGui::CollapsingHeader("Update Window: Enable"))
			{

				int _bState = 0;
				if (ImGui::Button("Enable Window##wi0"))
				{
					_bState = 2;
				}
				ImGui::SameLine();
				if (ImGui::Button("Disable Window##wi1"))
				{
					_bState = 1;

				}
				if (_bState > 0)
				{
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
					UpdateWindowEnableCmd w_enable;
					w_enable.window_index = _vWindowIndex;
					w_enable.enabled = _bState - 1;
					auto w_enable_cmd = SDHRCommand_UpdateWindowEnable(&w_enable);
					batcher.AddCommand(&w_enable_cmd);
					batcher.SDHR_Process();
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
				ImGui::InputInt(" ##0uwsu", &_uwsu_tile_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px##0uwsu", &_uwsu_tile_ybegin);
				ImGui::Text("Tile Count:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##1uwsu", &_uwsu_tile_xcount); ImGui::SameLine(240); ImGui::InputInt(" tiles##1uwsu", &_uwsu_tile_ycount);
				ImGui::PopItemWidth();
				ImGui::SliderInt("Med Byte##uwsu", &_uwsu_addr_med, 0, 255);
				ImGui::SliderInt("High Byte##uwsu", &_uwsu_addr_high, 0, 255);
				if (ImGui::Button("Update##uwsu"))
				{
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
					UpdateWindowSetUploadCmd _wcmd;
					_wcmd.window_index = _vWindowIndex;
					_wcmd.tile_xbegin = _uwsu_tile_xbegin;
					_wcmd.tile_ybegin = _uwsu_tile_ybegin;
					_wcmd.tile_xcount = _uwsu_tile_xcount;
					_wcmd.tile_ycount = _uwsu_tile_ycount;
					_wcmd.upload_addr_med = _uwsu_addr_med;
					_wcmd.upload_addr_high = _uwsu_addr_high;
					auto w_enable_cmd = SDHRCommand_UpdateWindowSetUpload(&_wcmd);
					batcher.AddCommand(&w_enable_cmd);
					batcher.SDHR_Process();
				}
			}
			if (ImGui::CollapsingHeader("Update Window: Single Tileset"))
			{
				static int _uwst_tile_xbegin;
				static int _uwst_tile_ybegin;
				static int _uwst_tile_xcount;
				static int _uwst_tile_ycount;
				static int _uwst_tileset_index;
				static int _uwst_data[4] = { 0, 0, 0, 0 };
				ImGui::PushItemWidth(80.f);
				ImGui::SliderInt("Tileset Index##uwst", &_uwst_tileset_index, 0, 1);
				ImGui::Text("Tile Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##0uwst", &_uwst_tile_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px##0uwst", &_uwst_tile_ybegin);
				ImGui::Text("Tile Count:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##1uwst", &_uwst_tile_xcount); ImGui::SameLine(240); ImGui::InputInt(" tiles##1uwst", &_uwst_tile_ycount);
				ImGui::PopItemWidth();
				ImGui::Text("Data is 1-byte record per tile: an index on the given tileset");
				ImGui::Text("Max of 4 tiles can be used here.");
				ImGui::InputInt4("Data##uwst", _uwst_data);
				if (ImGui::Button("Update##uwst"))
				{
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
					UpdateWindowSingleTilesetCmd _wcmd;
					_wcmd.window_index = _vWindowIndex;
					_wcmd.tile_xbegin = _uwst_tile_xbegin;
					_wcmd.tile_ybegin = _uwst_tile_ybegin;
					_wcmd.tile_xcount = _uwst_tile_xcount;
					_wcmd.tile_ycount = _uwst_tile_ycount;
					_wcmd.tileset_index = _uwst_tileset_index;
					uint8_t _uwst_data2[4];
					for (size_t i = 0; i < std::max(4, _uwst_tile_xcount * _uwst_tile_ycount); i++)
					{
						_uwst_data2[i] = (uint8_t)_uwst_data[i];
					}
					_wcmd.data = _uwst_data2;
					auto w_enable_cmd = SDHRCommand_UpdateWindowSingleTileset(&_wcmd);
					batcher.AddCommand(&w_enable_cmd);
					batcher.SDHR_Process();
				}
			}
			if (ImGui::CollapsingHeader("Update Window: Set Both"))
			{
				static int _uwsb_tile_xbegin;
				static int _uwsb_tile_ybegin;
				static int _uwsb_tile_xcount;
				static int _uwsb_tile_ycount;
				static int _uwsb_tileset[4] = { 0, 0, 0, 0 };
				static int _uwsb_data[4] = { 0, 0, 0, 0 };
				ImGui::PushItemWidth(80.f);
				ImGui::Text("Tile Begin:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##0uwsb", &_uwsb_tile_xbegin); ImGui::SameLine(240); ImGui::InputInt(" px##0uwsb", &_uwsb_tile_ybegin);
				ImGui::Text("Tile Count:");  ImGui::SameLine(130);
				ImGui::InputInt(" ##1uwsb", &_uwsb_tile_xcount); ImGui::SameLine(240); ImGui::InputInt(" tiles##1uwsb", &_uwsb_tile_ycount);
				ImGui::PopItemWidth();
				ImGui::Text("Input tileset and index for each tile.");
				ImGui::Text("Max of 4 tiles can be used here.");
				ImGui::InputInt4("Tilesets##uwsb", _uwsb_tileset);
				ImGui::InputInt4("Index##uwsb", _uwsb_data);
				if (ImGui::Button("Update##uwsb"))
				{
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
					UpdateWindowSetBothCmd _wcmd;
					_wcmd.window_index = _vWindowIndex;
					_wcmd.tile_xbegin = _uwsb_tile_xbegin;
					_wcmd.tile_ybegin = _uwsb_tile_ybegin;
					_wcmd.tile_xcount = _uwsb_tile_xcount;
					_wcmd.tile_ycount = _uwsb_tile_ycount;
					uint8_t _uwsb_data2[8];
					for (size_t i = 0; i < std::max(4, _uwsb_tile_xcount * _uwsb_tile_ycount); i++)
					{
						_uwsb_data2[i * 2] = (uint8_t)_uwsb_tileset[i];
						_uwsb_data2[(i * 2) + 1] = (uint8_t)_uwsb_data[i];
					}
					_wcmd.data = _uwsb_data2;
					auto w_updateb_cmd = SDHRCommand_UpdateWindowSetBoth(&_wcmd);
					batcher.AddCommand(&w_updateb_cmd);
					batcher.SDHR_Process();
				}
			}
			if (ImGui::CollapsingHeader("Update Window: Shift Tiles"))
			{
				static int _uwshift_x = 0;
				static int _uwshift_y = 0;
				ImGui::SliderInt("Shift X##uwshift", &_uwshift_x, -127, 127);
				ImGui::SliderInt("Shift Y##uwshift", &_uwshift_y, -127, 127);
				if (ImGui::Button("Shift Tiles##uwshift"))
				{
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
					UpdateWindowShiftTilesCmd _wcmd;
					_wcmd.window_index = _vWindowIndex;
					_wcmd.x_dir = _uwshift_x;
					_wcmd.y_dir = _uwshift_y;
					auto w_updateb_cmd = SDHRCommand_UpdateWindowShiftTiles(&_wcmd);
					batcher.AddCommand(&w_updateb_cmd);
					batcher.SDHR_Process();
				}
			}
			if (ImGui::CollapsingHeader("Update Window: Set Window Position"))
			{
				static int _uwsetwin_x = 0;
				static int _uwsetwin_y = 0;
				ImGui::Text("Set the window position by defining the top left screen xy pixel");
				ImGui::PushItemWidth(140.f);
				ImGui::InputInt("PosX##uwsetwin", &_uwsetwin_x); ImGui::SameLine(220); ImGui::InputInt("PosY##uwsetwin", &_uwsetwin_y);
				ImGui::PopItemWidth();
				if (ImGui::Button("Set Window Position##uwsetwin"))
				{
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
					UpdateWindowSetWindowPositionCmd _wcmd;
					_wcmd.window_index = _vWindowIndex;
					_wcmd.screen_xbegin = _uwsetwin_x;
					_wcmd.screen_ybegin = _uwsetwin_y;
					auto w_updateb_cmd = SDHRCommand_UpdateWindowSetWindowPosition(&_wcmd);
					batcher.AddCommand(&w_updateb_cmd);
					batcher.SDHR_Process();
				}
			}
			if (ImGui::CollapsingHeader("Update Window: Adjust Window View"))
			{
				static int _uwadjview_x = 0;
				static int _uwadjview_y = 0;
				ImGui::Text("Adjust the window view by defining the top left tile xy");
				ImGui::PushItemWidth(80.f);
				ImGui::InputInt("TileX##uwadjview", &_uwadjview_x); ImGui::SameLine(150); ImGui::InputInt("TileY##uwadjview", &_uwadjview_y);
				ImGui::PopItemWidth();
				if (ImGui::Button("Adjust Window View##uwadjview"))
				{
					auto batcher = SDHRCommandBatcher(server_ip, server_port);
					UpdateWindowAdjustWindowViewCmd _wcmd;
					_wcmd.window_index = _vWindowIndex;
					_wcmd.tile_xbegin = _uwadjview_x;
					_wcmd.tile_ybegin = _uwadjview_y;
					auto w_updateb_cmd = SDHRCommand_UpdateWindowAdjustWindowView(&_wcmd);
					batcher.AddCommand(&w_updateb_cmd);
					batcher.SDHR_Process();
				}
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

		ImGui::PopFont();

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
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
