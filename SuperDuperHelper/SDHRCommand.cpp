#include "SDHRCommand.h"


SDHRCommandBatcher::SDHRCommandBatcher(std::string server_ip, int server_port)
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed: " << result << std::endl;
		isConnected = false;
		return;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
	memset(&(server_addr.sin_zero), '\0', 8);

	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_socket == INVALID_SOCKET) {
		std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
		WSACleanup();
		isConnected = false;
		return;
	}
	if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		std::cerr << "Error connecting to server: " << WSAGetLastError() << std::endl;
		closesocket(client_socket);
		WSACleanup();
		isConnected = false;
		return;
	}
	isConnected = true;

	packet.pad = 0;
}

SDHRCommandBatcher::~SDHRCommandBatcher()
{
	closesocket(client_socket);
	WSACleanup();
}

void SDHRCommandBatcher::SDHR_On()
{
	packet.addr = cxSDHR_ctrl;
	packet.data = (uint8_t)SDHRControls::ENABLE;
	send(client_socket, (char *)&packet, 4, 0);
}

void SDHRCommandBatcher::SDHR_Off()
{
	packet.addr = cxSDHR_ctrl;
	packet.data = (uint8_t)SDHRControls::DISABLE;
	send(client_socket, (char*)&packet, 4, 0);
}

void SDHRCommandBatcher::SDHR_Reset()
{
	packet.addr = cxSDHR_ctrl;
	packet.data = (uint8_t)SDHRControls::RESET;
	send(client_socket, (char*)&packet, 4, 0);
}

void SDHRCommandBatcher::SDHR_Process()
{
	// Always send 4 bytes: the address (0xC0B0 for ctrl or 0xC0B1 for data), the data byte, and a pad byte
	packet.addr = cxSDHR_data;
	for (auto& cmd : v_cmds)
	{
		// Send all the data
		for (auto& datab : cmd->v_data)
		{
			packet.data = datab;
			send(client_socket, (char*)&packet, 4, 0);
		}
	}
	// Now send the control command
	packet.addr = cxSDHR_ctrl;
	packet.data = (uint8_t)SDHRControls::PROCESS;
	send(client_socket, (char*)&packet, 4, 0);
}

void SDHRCommandBatcher::AddCommand(SDHRCommand* command)
{
	v_cmds.push_back(command);
}

void SDHRCommand::InsertSizeHeader()
{
	// OBSOLETE
// 	uint16_t vSize = v_data.size() - 1;
// 	uint8_t* p;
// 	p = (uint8_t*)&vSize;
// 	v_data.insert(v_data.begin(), p[1]);
// 	v_data.insert(v_data.begin(), p[0]);
}

SDHRCommand_UpdateWindowEnable::SDHRCommand_UpdateWindowEnable(UpdateWindowEnableCmd* cmd)
{
	id = SDHR_CMD::UPDATE_WINDOW_ENABLE;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UpdateWindowEnableCmd); i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}

SDHRCommand_DefineTilesetImmediate::SDHRCommand_DefineTilesetImmediate(DefineTilesetImmediateCmd* cmd)
{
	id = SDHR_CMD::DEFINE_TILESET_IMMEDIATE;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	// push all but the pointer to the data field
	for (size_t i = 0; i < (sizeof(DefineTilesetImmediateCmd) - sizeof(uint8_t*)); i++) { v_data.push_back(p[i]); };
	// push the data field
	p = cmd->data;
	size_t entries = (cmd->num_entries == 0) ? 256 : cmd->num_entries;
	for (size_t i = 0; i < (size_t)4 * entries; i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}

SDHRCommand_DefineWindow::SDHRCommand_DefineWindow(DefineWindowCmd* cmd)
{
	id = SDHR_CMD::DEFINE_WINDOW;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(DefineWindowCmd); i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}

SDHRCommand_UpdateWindowSetBoth::SDHRCommand_UpdateWindowSetBoth(UpdateWindowSetBothCmd* cmd)
{
	id = SDHR_CMD::UPDATE_WINDOW_SET_BOTH;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	// push all but the pointer to the data field
	for (size_t i = 0; i < (sizeof(UpdateWindowSetBothCmd) - sizeof(uint8_t*)); i++) { v_data.push_back(p[i]); };
	// push the data field
	p = cmd->data;
	for (size_t i = 0; i < (size_t)cmd->tile_xcount * cmd->tile_ycount * 2; i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}

SDHRCommand_UpdateWindowSetUpload::SDHRCommand_UpdateWindowSetUpload(UpdateWindowSetUploadCmd* cmd)
{
	id = SDHR_CMD::UPDATE_WINDOW_SET_UPLOAD;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UpdateWindowSetUploadCmd); i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}

SDHRCommand_UpdateWindowSetWindowPosition::SDHRCommand_UpdateWindowSetWindowPosition(UpdateWindowSetWindowPositionCmd* cmd) {
	id = SDHR_CMD::UPDATE_WINDOW_SET_WINDOW_POSITION;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UpdateWindowSetWindowPositionCmd); i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}

SDHRCommand_UploadData::SDHRCommand_UploadData(UploadDataCmd* cmd)
{
	id = SDHR_CMD::UPLOAD_DATA;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UploadDataCmd); i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}

SDHRCommand_UploadDataFilename::SDHRCommand_UploadDataFilename(UploadDataFilenameCmd* cmd)
{
	id = SDHR_CMD::UPLOAD_DATA_FILENAME;
	v_data.push_back((uint8_t)id);
	v_data.push_back(cmd->dest_addr_med);
	v_data.push_back(cmd->dest_addr_high);
	v_data.push_back(cmd->filename_length);
	// push the filename string (no trailing null)
	for (size_t i = 0; i < cmd->filename_length; i++) { v_data.push_back(cmd->filename[i]); };
	InsertSizeHeader();
}

SDHRCommand_DefineImageAsset::SDHRCommand_DefineImageAsset(DefineImageAssetCmd* cmd)
{
	id = SDHR_CMD::DEFINE_IMAGE_ASSET;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(DefineImageAssetCmd); i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}

SDHRCommand_DefineImageAssetFilename::SDHRCommand_DefineImageAssetFilename(DefineImageAssetFilenameCmd* cmd)
{
	id = SDHR_CMD::DEFINE_IMAGE_ASSET_FILENAME;
	v_data.push_back((uint8_t)id);
	v_data.push_back(cmd->asset_index);
	v_data.push_back(cmd->filename_length);
	// push the filename string (no trailing null)
	for (size_t i = 0; i < cmd->filename_length; i++) { v_data.push_back(cmd->filename[i]); };
	InsertSizeHeader();
}


SDHRCommand_DefineTileset::SDHRCommand_DefineTileset(DefineTilesetCmd* cmd)
{
	id = SDHR_CMD::DEFINE_TILESET;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(DefineTilesetCmd); i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}


SDHRCommand_UpdateWindowSingleTileset::SDHRCommand_UpdateWindowSingleTileset(UpdateWindowSingleTilesetCmd* cmd)
{
	id = SDHR_CMD::UPDATE_WINDOW_SINGLE_TILESET;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	// push all but the pointer to the data field
	for (size_t i = 0; i < (sizeof(UpdateWindowSingleTilesetCmd) - sizeof(uint8_t*)); i++) { v_data.push_back(p[i]); };
	// push the data field
	p = cmd->data;
	for (size_t i = 0; i < (size_t)cmd->tile_xcount * cmd->tile_ycount; i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}

SDHRCommand_UpdateWindowShiftTiles::SDHRCommand_UpdateWindowShiftTiles(UpdateWindowShiftTilesCmd* cmd)
{
	id = SDHR_CMD::UPDATE_WINDOW_SHIFT_TILES;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UpdateWindowShiftTilesCmd); i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}

SDHRCommand_UpdateWindowAdjustWindowView::SDHRCommand_UpdateWindowAdjustWindowView(UpdateWindowAdjustWindowViewCmd* cmd)
{
	id = SDHR_CMD::UPDATE_WINDOW_ADJUST_WINDOW_VIEW;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UpdateWindowAdjustWindowViewCmd); i++) { v_data.push_back(p[i]); };
	InsertSizeHeader();
}
