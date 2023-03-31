#include "SDHRCommand.h"
#include <stdint.h>


/* End SHDR Command Structures */

void SDHRCommandBatcher::Publish()
{
	// TODO: Shouldn't have to recreate a vector
	uint64_t vecsize = 0;
	for (auto& cmd : v_cmds)
	{
		vecsize += cmd->v_data.size();
	}
	std::vector<uint8_t> v_fulldata;
	v_fulldata.reserve(vecsize);
	for (auto& cmd : v_cmds)
	{
		uint16_t cmd_size = cmd->v_data.size() - 1;
		uint8_t* p_cmdsize = (uint8_t*)&cmd_size;
		v_fulldata.insert(v_fulldata.end(), p_cmdsize, p_cmdsize + 2);
		v_fulldata.insert(v_fulldata.end(), cmd->v_data.begin(), cmd->v_data.end());
	}
	GameLink::SDHR_write(v_fulldata);
}

void SDHRCommandBatcher::AddCommand(SDHRCommand* command)
{
	v_cmds.push_back(command);
}

SDHRCommand_UpdateWindowEnable::SDHRCommand_UpdateWindowEnable(UpdateWindowEnableCmd* cmd)
{
	id = SDHR_CMD::UPDATE_WINDOW_ENABLE;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UpdateWindowEnableCmd); i++) { v_data.push_back(p[i]); };
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
}

SDHRCommand_DefineWindow::SDHRCommand_DefineWindow(DefineWindowCmd* cmd)
{
	id = SDHR_CMD::DEFINE_WINDOW;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(DefineWindowCmd); i++) { v_data.push_back(p[i]); };
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
	for (size_t i = 0; i < (size_t)cmd->tile_xcount * cmd->tile_ycount; i++) { v_data.push_back(p[i]); };
}

SDHRCommand_UpdateWindowSetWindowPosition::SDHRCommand_UpdateWindowSetWindowPosition(UpdateWindowSetWindowPositionCmd* cmd) {
	id = SDHR_CMD::UPDATE_WINDOW_SET_WINDOW_POSITION;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UpdateWindowSetWindowPositionCmd); i++) { v_data.push_back(p[i]); };
}

SDHRCommand_UploadData::SDHRCommand_UploadData(UploadDataCmd* cmd)
{
	id = SDHR_CMD::UPLOAD_DATA;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UploadDataCmd); i++) { v_data.push_back(p[i]); };
}

SDHRCommand_DefineImageAsset::SDHRCommand_DefineImageAsset(DefineImageAssetCmd* cmd)
{
	id = SDHR_CMD::DEFINE_IMAGE_ASSET;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(DefineImageAssetCmd); i++) { v_data.push_back(p[i]); };
}

SDHRCommand_DefineImageAssetFilename::SDHRCommand_DefineImageAssetFilename(DefineImageAssetFilenameCmd* cmd)
{
	id = SDHR_CMD::DEFINE_IMAGE_ASSET_FILENAME;
	v_data.push_back((uint8_t)id);
	v_data.push_back(cmd->asset_index);
	v_data.push_back(cmd->filename_length);
	// push the filename string (no trailing null)
	for (size_t i = 0; i < cmd->filename_length; i++) { v_data.push_back(cmd->filename[i]); };
}


SDHRCommand_DefineTileset::SDHRCommand_DefineTileset(DefineTilesetCmd* cmd)
{
	id = SDHR_CMD::DEFINE_TILESET;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(DefineTilesetCmd); i++) { v_data.push_back(p[i]); };
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
}

SDHRCommand_UpdateWindowShiftTiles::SDHRCommand_UpdateWindowShiftTiles(UpdateWindowShiftTilesCmd* cmd)
{
	id = SDHR_CMD::UPDATE_WINDOW_SHIFT_TILES;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UpdateWindowShiftTilesCmd); i++) { v_data.push_back(p[i]); };
}

SDHRCommand_UpdateWindowAdjustWindowView::SDHRCommand_UpdateWindowAdjustWindowView(UpdateWindowAdjustWindowViewCmd* cmd)
{
	id = SDHR_CMD::UPDATE_WINDOW_ADJUST_WINDOW_VIEW;
	v_data.push_back((uint8_t)id);
	uint8_t* p = (uint8_t*)cmd;
	for (size_t i = 0; i < sizeof(UpdateWindowAdjustWindowViewCmd); i++) { v_data.push_back(p[i]); };
}
