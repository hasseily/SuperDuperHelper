#pragma once

#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

#define NOMCX
#define NOSERVICE
#define NOHELP
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

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
	extern void SDHR_process();
	extern void SDHR_write(BYTE b);

	extern void SetSoundVolume(UINT8 main, UINT8 mockingboard);
	extern int GetSoundVolumeMain();
	extern int GetSoundVolumeMockingboard();

	extern void SendKeystroke(UINT scancode, bool isPressed);

	extern sFramebufferInfo GetFrameBufferInfo();
	extern inline UINT16 GetFrameSequence();

}; // namespace GameLink
