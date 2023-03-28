#include "SDHRCommand.h"
#include <stdint.h>

#pragma pack(push)
#pragma pack(1)	
struct UploadDataCmd {
	uint8_t dest_addr_med;
	uint8_t dest_addr_high;
	uint8_t source_addr_med;
	uint8_t num_256b_pages;
};

struct DefineTilesetCmd {
	uint8_t tileset_index;
	uint8_t depth;
	uint8_t num_entries;
	uint8_t xdim;
	uint8_t ydim;
	uint8_t data_low;
	uint8_t data_med;
	uint8_t data_high;
};

struct DefineTilesetImmediateCmd {
	uint8_t tileset_index;
	uint8_t depth;
	uint8_t num_entries;
	uint8_t xdim;
	uint8_t ydim;
	uint8_t data[];
};

struct DefinePaletteCmd {
	uint8_t palette_index;
	uint8_t data_low;
	uint8_t data_med;
	uint8_t data_high;
};

struct DefinePaletteImmediateCmd {
	uint8_t palette_index;
	uint8_t depth;
	uint8_t data[];
};

struct DefineWindowCmd {
	int8_t window_index;
	uint16_t screen_xcount;
	uint16_t screen_ycount;
	int16_t screen_xbegin;
	int16_t screen_ybegin;
	uint16_t tile_xbegin;
	uint16_t tile_ybegin;
	uint16_t tile_xdim;
	uint16_t tile_ydim;
	uint16_t tile_xcount;
	uint16_t tile_ycount;
};

struct UpdateWindowSetAllCmd {
	int8_t window_index;
	uint16_t tile_xbegin;
	uint16_t tile_ybegin;
	uint16_t tile_xcount;
	uint16_t tile_ycount;
	uint8_t data[];
};

struct UpdateWindowSingleTilesetCmd {
	int8_t window_index;
	uint16_t tile_xbegin;
	uint16_t tile_ybegin;
	uint16_t tile_xcount;
	uint16_t tile_ycount;
	uint8_t tileset_index;
	uint8_t data[];
};

struct UpdateWindowSinglePaletteCmd {
	int8_t window_index;
	uint16_t tile_xbegin;
	uint16_t tile_ybegin;
	uint16_t tile_xcount;
	uint16_t tile_ycount;
	uint8_t palette_index;
	uint8_t data[];
};

struct UpdateWindowSingleBothCmd {
	int8_t window_index;
	uint16_t tile_xbegin;
	uint16_t tile_ybegin;
	uint16_t tile_xcount;
	uint16_t tile_ycount;
	uint8_t tileset_index;
	uint8_t palette_index;
	uint8_t data[];
};

struct UpdateWindowSetBitmasksCmd {
	int8_t window_index;
	uint16_t tile_xcount;
	uint16_t tile_ycount;
	uint8_t data[];
};

struct UpdateWindowShiftTilesCmd {
	int8_t window_index;
	int8_t x_dir; // +1 shifts tiles right by 1, negative shifts tiles left by 1, zero no change
	int8_t y_dir; // +1 shifts tiles down by 1, negative shifts tiles up by 1, zero no change
};

struct UpdateWindowSetWindowPositionCmd {
	int8_t window_index;
	int16_t screen_xbegin;
	int16_t screen_ybegin;
};

struct UpdateWindowAdjustWindowViewCommand {
	int8_t window_index;
	uint16_t tile_xbegin;
	uint16_t tile_ybegin;
};

struct UpdateWindowEnableCmd {
	int8_t window_index;
	bool enabled;
};

#pragma pack(pop)

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
		v_fulldata.insert(v_fulldata.end(), cmd->v_data.begin(), cmd->v_data.end());
	}
	GameLink::SDHR_write(v_fulldata);
}

void SDHRCommandBatcher::AddCommand(SDHRCommand* command)
{
	v_cmds.push_back(command);
}

void SDHRCommand::InsertSizeHeader()
{
	uint16_t vSize = v_data.size() - 1;
	uint8_t* p;
	p = (uint8_t*)&vSize;
	v_data.insert(v_data.begin(), p[1]);
	v_data.insert(v_data.begin(), p[0]);
}

SDHRCommand_UpdateWindowEnable::SDHRCommand_UpdateWindowEnable(uint8_t index, bool enabled) : SDHRCommand()
{
	id = SDHR_CMD::UPDATE_WINDOW_ENABLE;
	v_data.push_back((uint8_t)id);
	v_data.push_back(index);
	v_data.push_back((uint8_t)enabled);

	InsertSizeHeader();
}

SDHRCommand_DefineTilesetImmediate::SDHRCommand_DefineTilesetImmediate(uint8_t index, uint8_t depth,
	uint8_t num_entries, uint8_t xdim, uint8_t ydim, uint8_t* data, uint32_t datalen)
{
	id = SDHR_CMD::DEFINE_TILESET_IMMEDIATE;
	v_data.push_back((uint8_t)id);
	v_data.push_back(index);
	v_data.push_back(depth);
	v_data.push_back(num_entries);
	v_data.push_back(xdim);
	v_data.push_back(ydim);
	for (auto i = 0; i < datalen; i++) { v_data.push_back(data[i]); };

	InsertSizeHeader();
}

SDHRCommand_DefinePaletteImmediate::SDHRCommand_DefinePaletteImmediate(uint8_t index, uint8_t depth, uint8_t* data, uint16_t datalen)
{
	id = SDHR_CMD::DEFINE_PALETTE_IMMEDIATE;
	v_data.push_back((uint8_t)id);
	v_data.push_back(index);
	v_data.push_back(depth);
	// The data for the palette should be the size of 2 to the power of (depth + 1)
	// because each color is 2 bytes. But the spec states that if some of the data is missing,
	// it is still valid and the remainder will be 0
	for (auto i = 0; i < datalen; i++) { v_data.push_back(data[i]); };
	InsertSizeHeader();
}

SDHRCommand_DefineWindow::SDHRCommand_DefineWindow(uint8_t index, 
	uint16_t screen_xcount, uint16_t screen_ycount, 
	uint16_t screen_xbegin, uint16_t screen_ybegin, 
	uint16_t tile_xbegin, uint16_t tile_ybegin, 
	uint16_t tile_xdim, uint16_t tile_ydim, 
	uint16_t tile_xcount, uint16_t tile_ycount)
{
	id = SDHR_CMD::DEFINE_WINDOW;
	// all the below are 2 byte, to push to a vector of bytes
	v_data.push_back((uint8_t)id);
	v_data.push_back(index);
	uint8_t* p;
	p = (uint8_t*)&screen_xcount;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&screen_ycount;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&screen_xbegin;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&screen_ybegin;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&tile_xbegin;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&tile_ybegin;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&tile_xdim;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&tile_ydim;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&tile_xcount;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&tile_ycount;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);

	InsertSizeHeader();
}

SDHRCommand_UpdateWindowSingleBoth::SDHRCommand_UpdateWindowSingleBoth(uint8_t index, 
	uint16_t tile_xbegin, uint16_t tile_ybegin, 
	uint16_t tile_xcount, uint16_t tile_ycount, 
	uint8_t tileset_index, uint8_t palette_index, 
	uint8_t* data, uint16_t datalen)
{
	id = SDHR_CMD::UPDATE_WINDOW_SINGLE_BOTH;
	// all the below are 2 byte, to push to a vector of bytes
	v_data.push_back((uint8_t)id);
	v_data.push_back(index);
	uint8_t* p;
	p = (uint8_t*)&tile_xbegin;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&tile_ybegin;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&tile_xcount;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&tile_ycount;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	
	v_data.push_back(tileset_index);
	v_data.push_back(palette_index);
	for (auto i = 0; i < datalen; i++) { v_data.push_back(data[i]); };

	InsertSizeHeader();
}

SDHRCommand_SetWindowPosition::SDHRCommand_SetWindowPosition(uint8_t index,
	uint16_t xbegin, uint16_t ybegin) {
	id = SDHR_CMD::UPDATE_WINDOW_SET_WINDOW_POSITION;
	v_data.push_back((uint8_t)id);
	v_data.push_back(index);
	uint8_t* p;
	p = (uint8_t*)&xbegin;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);
	p = (uint8_t*)&ybegin;
	v_data.push_back(p[0]);
	v_data.push_back(p[1]);

	InsertSizeHeader();
}