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
 * @brief SDHRCommand
 * Superclass of all SDHR commands
 * Each SDHR command will be constructed with all necessary data
 * It makes a copy of the data so you can reuse your buffers when creating commands
*/
class SDHRCommand
{
public:
	SDHR_CMD id;
	std::vector<uint8_t> v_data;
protected:
	void InsertSizeHeader();
};

class SDHRCommand_DefineTilesetImmediate : public SDHRCommand
{
public:
	SDHRCommand_DefineTilesetImmediate(uint8_t index, uint8_t depth, uint8_t num_entries, 
		uint8_t xdim, uint8_t ydim, uint8_t* data, uint32_t datalen);
};

class SDHRCommand_DefinePaletteImmediate : public SDHRCommand
{
public:
	SDHRCommand_DefinePaletteImmediate(uint8_t index, uint8_t depth, uint8_t* data, uint16_t datalen);
};

class SDHRCommand_DefineWindow : public SDHRCommand
{
public:
	SDHRCommand_DefineWindow(uint8_t index,
		uint16_t screen_xcount, uint16_t screen_ycount,
		uint16_t screen_xbegin, uint16_t screen_ybegin,
		uint16_t tile_xbegin, uint16_t tile_ybegin,
		uint16_t tile_xdim, uint16_t tile_ydim,
		uint16_t tile_xcount, uint16_t tile_ycount);
};

class SDHRCommand_UpdateWindowSingleBoth : public SDHRCommand
{
public:
	SDHRCommand_UpdateWindowSingleBoth(uint8_t index, 
		uint16_t tile_xbegin, uint16_t tile_ybegin,
		uint16_t tile_xcount, uint16_t tile_ycount,
		uint8_t tileset_index, uint8_t palette_index,
		uint8_t* data, uint16_t datalen);
};

class SDHRCommand_UpdateWindowEnable : public SDHRCommand
{
public:
	SDHRCommand_UpdateWindowEnable(uint8_t index, bool enabled);
	SDHRCommand_UpdateWindowEnable() : SDHRCommand_UpdateWindowEnable( 0, true) {}
};

class SDHRCommand_SetWindowPosition : public SDHRCommand
{
public:
	SDHRCommand_SetWindowPosition(uint8_t index,
		uint16_t xbegin, uint16_t ybegin);
};