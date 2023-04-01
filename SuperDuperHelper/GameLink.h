#pragma once

#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

#define NOMCX
#define NOSERVICE
#define NOHELP
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <string>
#include <vector>

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

//------------------------------------------------------------------------------
// Namespace Declaration
//------------------------------------------------------------------------------

namespace GameLink
{

	//--------------------------------------------------------------------------
	// Global Declarations
	//--------------------------------------------------------------------------

	struct sFramebufferInfo
	{
		UINT16 width;
		UINT16 height;
		UINT8 imageFormat; // 0 = no frame; 1 = 32-bit 0xAARRGGBB
		UINT16 parX; // pixel aspect ratio
		UINT16 parY;
		UINT32 bufferLength;
		bool wantsMouse;
		UINT8* frameBuffer;
	};

	//--------------------------------------------------------------------------
	// Global Functions
	//--------------------------------------------------------------------------

	extern int Init();
	extern void Destroy();
	
	extern std::string GetEmulatedProgramName();
	extern int GetMemorySize();
	extern UINT8* GetMemoryBasePointer();
	extern UINT8 GetPeekAt(UINT position);
	extern bool IsActive();
	extern bool IsTrackingOnly();

	extern void SendCommand(std::string command);
	extern void Pause();
	extern void Reset();
	extern void Shutdown();
	extern void SDHR_on();
	extern void SDHR_off();
	extern void SDHR_reset();
	//extern void SDHR_write(uint8_t* buf, UINT16 buflength);
	extern void SDHR_write(const std::vector<uint8_t>& v_data);

	extern void SetSoundVolume(UINT8 main, UINT8 mockingboard);
	extern int GetSoundVolumeMain();
	extern int GetSoundVolumeMockingboard();

	extern void SendKeystroke(UINT scancode, bool isPressed);

	extern sFramebufferInfo GetFrameBufferInfo();
	extern inline UINT16 GetFrameSequence();

}; // namespace GameLink
