#pragma once
#include "GameLink.h"
#include <vector>

class SDHRCommand
{

public:
	// Writes the complete command to SHM along with a SDHR_CMD_READY flag
	// Call GameLink::SDHR_process() to have AppleWin process them
	void PublishCommand(std::vector<uint8_t>& data);

	// Stream of subcommands to add to the command
	// They'll be processed in FIFO.
	void AddSubCommand(SDHRCommand& subcommand);
private:
	std::vector<SDHRCommand&> v_subcmds;
};

