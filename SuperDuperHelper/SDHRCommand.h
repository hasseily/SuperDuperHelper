#pragma once
#include <vector>
#include <string>
#include <stdint.h>
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>

static char cxSDHR_hi = 0xC0;		// SDHR High byte of 0xC0B.
static char cxSDHR_ctrl = 0xB0;	// SDHR Low byte of 0xC0B. command
static char cxSDHR_data = 0xB1;	// SDHR Low byte of 0xC01. data

enum class SDHRControls
{
	DISABLE = 0,
	ENABLE,
	PROCESS,
	RESET
};

/**
 * @brief SDHR Command structures
*/
enum class SDHR_CMD {
	NONE = 0,
	UPLOAD_DATA = 1,
	DEFINE_IMAGE_ASSET = 2,
	DEFINE_IMAGE_ASSET_FILENAME = 3,
	DEFINE_TILESET = 4,
	DEFINE_TILESET_IMMEDIATE = 5,
	DEFINE_WINDOW = 6,
	UPDATE_WINDOW_SET_BOTH = 7,
	UPDATE_WINDOW_SINGLE_TILESET = 8,
	UPDATE_WINDOW_SHIFT_TILES = 9,
	UPDATE_WINDOW_SET_WINDOW_POSITION = 10,
	UPDATE_WINDOW_ADJUST_WINDOW_VIEW = 11,
	UPDATE_WINDOW_SET_BITMASKS = 12,
	UPDATE_WINDOW_ENABLE = 13,
	READY = 14,
	UPLOAD_DATA_FILENAME = 15,
	UPDATE_WINDOW_SET_UPLOAD = 16,
};

class SDHRCommand;	// forward declaration

/**
 * @brief SDHRCommandBatcher
 * Writes the complete command batch to SHM along with a SDHR_CMD_READY flag
 * Call GameLink::SDHR_process() to have AppleWin process them
*/
class SDHRCommandBatcher
{
public:
	SDHRCommandBatcher(std::string server_ip, int server_port);
	~SDHRCommandBatcher();

	// Stream of subcommands to add to the command
	// They'll be processed in FIFO.
	void AddCommand(SDHRCommand* command);

	void SDHR_On();
	void SDHR_Off();
	void SDHR_Reset();
	// Publishes the queued commands.
	void SDHR_Process();

	bool isConnected = false;
private:
	std::vector<SDHRCommand*> v_cmds;
	SOCKET client_socket = NULL;
	sockaddr_in server_addr = { 0 };
};

/**
 * @brief SDHR Command Structs
 * Use these command structs to pass to the SDHRCommand functions
 */

#pragma pack(push)
#pragma pack(1)

struct UploadDataCmd {
	uint8_t dest_addr_med;
	uint8_t dest_addr_high;
	uint8_t source_addr_med;
	uint8_t num_256b_pages;
};

struct UploadDataFilenameCmd {
	uint8_t dest_addr_med;
	uint8_t dest_addr_high;
	uint8_t filename_length;
	const char* filename;  // don't include the trailing null either in the data or counted in the filename_length
};

struct DefineImageAssetCmd {
	uint8_t asset_index;
	uint8_t upload_addr_med;
	uint8_t upload_addr_high;
	uint16_t upload_page_count;
};

struct DefineImageAssetFilenameCmd {
	uint8_t asset_index;
	uint8_t filename_length;
	const char* filename;  // don't include the trailing null either in the data or counted in the filename_length
};

struct DefineTilesetCmd {
	uint8_t tileset_index;
	uint8_t num_entries;
	uint8_t xdim;
	uint8_t ydim;
	uint8_t asset_index;
	uint8_t data_med;
	uint8_t data_high;
};

struct DefineTilesetImmediateCmd {
	uint8_t tileset_index;
	uint8_t num_entries;
	uint8_t xdim;
	uint8_t ydim;
	uint8_t asset_index;
	uint8_t* data;  // data is 4-byte records, 16-bit x and y offsets (scaled by x/ydim), from the given asset
};

struct DefineWindowCmd {
	int8_t window_index;
	bool black_or_wrap;			// false: viewport is black outside of tile range, true: viewport wraps
	uint64_t screen_xcount;		// width in pixels of visible screen area of window
	uint64_t screen_ycount;
	int64_t screen_xbegin;		// pixel xy coordinate where window begins
	int64_t screen_ybegin;
	int64_t  tile_xbegin;		// pixel xy coordinate on backing tile array where aperture begins
	int64_t  tile_ybegin;
	uint64_t tile_xdim;			// xy dimension, in pixels, of tiles in the window.
	uint64_t tile_ydim;
	uint64_t tile_xcount;		// xy dimension, in tiles, of the tile array
	uint64_t tile_ycount;
};

struct UpdateWindowSetBothCmd {
	int8_t window_index;
	int64_t tile_xbegin;
	int64_t tile_ybegin;
	uint64_t tile_xcount;
	uint64_t tile_ycount;
	uint8_t* data;  // data is 2-byte records per tile, tileset and index
};

struct UpdateWindowSetUploadCmd {
	int8_t window_index;
	int64_t tile_xbegin;
	int64_t tile_ybegin;
	uint64_t tile_xcount;
	uint64_t tile_ycount;
	uint8_t upload_addr_med;
	uint8_t upload_addr_high;
};

struct UpdateWindowSingleTilesetCmd {
	int8_t window_index;
	int64_t tile_xbegin;
	int64_t tile_ybegin;
	uint64_t tile_xcount;
	uint64_t tile_ycount;
	uint8_t tileset_index;
	uint8_t* data;  // data is 1-byte record per tile, index on the given tileset
};

struct UpdateWindowShiftTilesCmd {
	int8_t window_index;
	int8_t x_dir; // +1 shifts tiles right by 1, negative shifts tiles left by 1, zero no change
	int8_t y_dir; // +1 shifts tiles down by 1, negative shifts tiles up by 1, zero no change
};

struct UpdateWindowSetWindowPositionCmd {
	int8_t window_index;
	int64_t screen_xbegin;
	int64_t screen_ybegin;
};

struct UpdateWindowAdjustWindowViewCmd {
	int8_t window_index;
	int64_t tile_xbegin;
	int64_t tile_ybegin;
};

struct UpdateWindowEnableCmd {
	int8_t window_index;
	bool enabled;
};

#pragma pack(pop)

/**
 * @brief SDHRCommand
 * Superclass of all SDHR commands
 * Each SDHR command will be constructed with all necessary data
 * It makes a copy of the data so you can reuse your buffers when creating commands
*/
class SDHRCommand
{
public:
	SDHR_CMD id = SDHR_CMD::NONE;
	std::vector<uint8_t> v_data;
protected:
	void InsertSizeHeader();
};

class SDHRCommand_UploadData : public SDHRCommand
{
public:
	SDHRCommand_UploadData(UploadDataCmd* cmd);
};

class SDHRCommand_UploadDataFilename : public SDHRCommand
{
public:
	SDHRCommand_UploadDataFilename(UploadDataFilenameCmd* cmd);
};

class SDHRCommand_DefineImageAsset : public SDHRCommand
{
public:
	SDHRCommand_DefineImageAsset(DefineImageAssetCmd* cmd);
};

class SDHRCommand_DefineImageAssetFilename : public SDHRCommand
{
public:
	SDHRCommand_DefineImageAssetFilename(DefineImageAssetFilenameCmd* cmd);
};

class SDHRCommand_DefineTileset : public SDHRCommand
{
public:
	SDHRCommand_DefineTileset(DefineTilesetCmd* cmd);
};

class SDHRCommand_DefineTilesetImmediate : public SDHRCommand
{
public:
	SDHRCommand_DefineTilesetImmediate(DefineTilesetImmediateCmd *cmd);
};

class SDHRCommand_DefineWindow : public SDHRCommand
{
public:
	SDHRCommand_DefineWindow(DefineWindowCmd* cmd);
};

class SDHRCommand_UpdateWindowSetBoth : public SDHRCommand
{
public:
	SDHRCommand_UpdateWindowSetBoth(UpdateWindowSetBothCmd* cmd);
};

class SDHRCommand_UpdateWindowSetUpload : public SDHRCommand
{
public:
	SDHRCommand_UpdateWindowSetUpload(UpdateWindowSetUploadCmd* cmd);
};

class SDHRCommand_UpdateWindowSingleTileset : public SDHRCommand
{
public:
	SDHRCommand_UpdateWindowSingleTileset(UpdateWindowSingleTilesetCmd* cmd);
};

class SDHRCommand_UpdateWindowShiftTiles : public SDHRCommand
{
public:
	SDHRCommand_UpdateWindowShiftTiles(UpdateWindowShiftTilesCmd* cmd);
};

class SDHRCommand_UpdateWindowSetWindowPosition : public SDHRCommand
{
public:
	SDHRCommand_UpdateWindowSetWindowPosition(UpdateWindowSetWindowPositionCmd* cmd);
};

class SDHRCommand_UpdateWindowAdjustWindowView : public SDHRCommand
{
public:
	SDHRCommand_UpdateWindowAdjustWindowView(UpdateWindowAdjustWindowViewCmd* cmd);
};

class SDHRCommand_UpdateWindowEnable : public SDHRCommand
{
public:
	SDHRCommand_UpdateWindowEnable(UpdateWindowEnableCmd* cmd);
};
