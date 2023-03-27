#include "SDHRCommand.h"
#include <stdint.h>

/**
 * @brief SHDR Command structures
*/
enum ShdrCmd_e {
	SDHR_CMD_UPLOAD_DATA = 1,
	SDHR_CMD_DEFINE_TILESET = 2,
	SDHR_CMD_DEFINE_TILESET_IMMEDIATE = 3,
	SDHR_CMD_DEFINE_PALETTE = 4,
	SDHR_CMD_DEFINE_PALETTE_IMMEDIATE = 5,
	SDHR_CMD_DEFINE_WINDOW = 6,
	SDHR_CMD_UPDATE_WINDOW_SET_ALL = 7,
	SDHR_CMD_UPDATE_WINDOW_SINGLE_TILESET = 8,
	SDHR_CMD_UPDATE_WINDOW_SINGLE_PALETTE = 9,
	SDHR_CMD_UPDATE_WINDOW_SINGLE_BOTH = 10,
	SDHR_CMD_UPDATE_WINDOW_SHIFT_TILES = 11,
	SDHR_CMD_UPDATE_WINDOW_SET_WINDOW_POSITION = 12,
	SDHR_CMD_UPDATE_WINDOW_ADJUST_WINDOW_VIEW = 13,
	SDHR_CMD_UPDATE_WINDOW_SET_BITMASKS = 14,
	SDHR_CMD_UPDATE_WINDOW_ENABLE = 15,
	SDHR_CMD_READY = 16,
};
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

