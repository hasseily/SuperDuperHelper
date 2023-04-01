#pragma once
#include "GameLink.h"
#include <vector>

class SDHRCommand;	// forward declaration

/**
 * @brief SDHRCommandBatcher
 * Writes the complete command batch to SHM along with a SDHR_CMD_READY flag
 * Call GameLink::SDHR_process() to have AppleWin process them
*/
class SDHRCommandBatcher
{
public:

	// Publishes the queued commands.
	// Call GameLink::SDHR_process() to have AppleWin process them
	void Publish();

	// Stream of subcommands to add to the command
	// They'll be processed in FIFO.
	void AddCommand(SDHRCommand* command);

private:
	std::vector<SDHRCommand*> v_cmds;
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

struct UpdateWindowSetBothCmd {
	int8_t window_index;
	uint16_t tile_xbegin;
	uint16_t tile_ybegin;
	uint16_t tile_xcount;
	uint16_t tile_ycount;
	uint8_t* data;  // data is 2-byte records per tile, tileset and index
};

struct UpdateWindowSetUploadCmd {
	int8_t window_index;
	uint16_t tile_xbegin;
	uint16_t tile_ybegin;
	uint16_t tile_xcount;
	uint16_t tile_ycount;
	uint8_t upload_addr_med;
	uint8_t upload_addr_high;
};

struct UpdateWindowSingleTilesetCmd {
	int8_t window_index;
	uint16_t tile_xbegin;
	uint16_t tile_ybegin;
	uint16_t tile_xcount;
	uint16_t tile_ycount;
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
	int16_t screen_xbegin;
	int16_t screen_ybegin;
};

struct UpdateWindowAdjustWindowViewCmd {
	int8_t window_index;
	uint16_t tile_xbegin;
	uint16_t tile_ybegin;
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
