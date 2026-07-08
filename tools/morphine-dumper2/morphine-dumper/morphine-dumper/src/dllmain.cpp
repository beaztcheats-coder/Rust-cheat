#include "core/core.hpp"

#include <windows.h>
#include <shlobj.h>
#include <cstdio>
#include <cstring>

#pragma comment( lib, "shell32.lib" )

static HMODULE g_self_handle = nullptr;

static BOOL WINAPI console_ctrl( DWORD ctrl )
{
	if ( ctrl == CTRL_CLOSE_EVENT || ctrl == CTRL_C_EVENT || ctrl == CTRL_BREAK_EVENT )
	{
		FreeConsole( );
		return TRUE;
	}
	return FALSE;
}

static DWORD WINAPI thread_main( LPVOID )
{
	AllocConsole( );
	SetConsoleTitleA( "morphine-dumper" );
	SetConsoleCtrlHandler( console_ctrl, TRUE );
	SetConsoleOutputCP( CP_UTF8 );

	FILE* dummy = nullptr;
	freopen_s( &dummy, "CONOUT$", "w", stdout );
	freopen_s( &dummy, "CONOUT$", "w", stderr );
	std::setvbuf( stdout, nullptr, _IONBF, 0 );

	char out_dir[ MAX_PATH ]{ };
	if ( FAILED( SHGetFolderPathA( nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, out_dir ) ) )
	{
		if ( !GetEnvironmentVariableA( "TEMP", out_dir, MAX_PATH ) )
			std::strcpy( out_dir, "C:\\temp" );
	}

	char output_path[ MAX_PATH ]{ };
	std::snprintf( output_path, MAX_PATH, "%s\\morphine-dumper_output.h", out_dir );

	std::printf( "[morphine-dumper] starting (output: %s)\n", output_path );

	{
		c_core core;
		if ( !core.run( output_path, true ) )
			std::printf( "[morphine-dumper] run failed\n" );
		else
			std::printf( "[morphine-dumper] done output written\n" );
	}

	std::printf( "[morphine-dumper] dump finished. press END to unload.\n" );
	std::fflush( stdout );

	constexpr int unload_hotkey_id = 1;
	RegisterHotKey( nullptr, unload_hotkey_id, MOD_NOREPEAT, VK_END );

	bool unload_requested = false;
	while ( true )
	{
		if ( GetAsyncKeyState( VK_END ) & 0x8000 )
		{
			unload_requested = true;
			break;
		}

		MSG message{ };
		while ( PeekMessageA( &message, nullptr, 0, 0, PM_REMOVE ) )
		{
			if ( message.message == WM_HOTKEY &&
				message.wParam == unload_hotkey_id )
			{
				unload_requested = true;
				break;
			}
		}
		if ( unload_requested )
			break;

		Sleep( 50 );
	}

	UnregisterHotKey( nullptr, unload_hotkey_id );
	std::printf( "[morphine-dumper] END detected, unloading DLL.\n" );
	SetConsoleCtrlHandler( console_ctrl, FALSE );
	std::fflush( stderr );
	std::fflush( stdout );
	FreeConsole( );
	FreeLibraryAndExitThread( g_self_handle, 0 );
}

BOOL APIENTRY DllMain( HMODULE module, DWORD reason, LPVOID )
{
	if ( reason == DLL_PROCESS_ATTACH )
	{
		g_self_handle = module;
		DisableThreadLibraryCalls( module );
		auto thread = CreateThread( nullptr, 0, thread_main, nullptr, 0, nullptr );
		if ( thread )
			CloseHandle( thread );
	}
	return TRUE;
}
