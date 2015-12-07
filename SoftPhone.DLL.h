
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../bob/debug.h"
#include "../bob/types.h"
#include "../bob/FastDelegate/FastDelegate.h"
typedef fastdelegate::FastDelegate2<u08*,u32,u32> DelBlockPush;
typedef fastdelegate::FastDelegate2<u08*,u32,u32> DelBlockPull;

struct softphone_config {
	DelBlockPush       push;
	DelBlockPull       pull;
	bool               available;
};

#if defined _WINDLL // For export as dll
#ifdef _WINDLL
#define DLL_CALL __declspec(dllexport)
#endif
extern "C" void DLL_CALL softphone_config_log( print_ft, print_ft );
extern "C" void DLL_CALL softphone_force_vpn_ip( const char* ip );
extern "C" int DLL_CALL softphone_init( struct softphone_config *csip );
extern "C" int DLL_CALL softphone_listen();
extern "C" int DLL_CALL softphone_connect_account( const char *account );
extern "C" int DLL_CALL softphone_codecs( const char *codecs );
//extern "C" int DLL_CALL softphone_connect( char *domain, char *username, char *password );
extern "C" int DLL_CALL softphone_destroy();
#undef DLL_CALL
#else // For Loading as dll

namespace SoftPhoneDLL {
#define LOAD_DLL_FUNCTION( function, name ) static function = (decltype(fn)) GetProcAddress(hDLL, name)
	HMODULE hDLL;
	
	static void config_vpn(const char *ip) {
		LOAD_DLL_FUNCTION(int (*fn)(const char*), "softphone_config_vpn");
		if( fn ) fn(ip);
	}
	static void config_codecs(const char *codecs) {
		LOAD_DLL_FUNCTION(int (*fn)(const char*), "softphone_codecs");
		if( fn ) fn(codecs);
	}
	static void config(print_ft dbg, print_ft err) {
		LOAD_DLL_FUNCTION(void (*fn)(print_ft, print_ft), "softphone_config_log");
		if( fn ) fn(dbg, err);
	}

	void init(struct softphone_config *csip, const char *ip, const char *codecs ) {
		LOAD_DLL_FUNCTION(int (*fn)(struct softphone_config*), "softphone_init");
		if( !hDLL  ) warnln("Using softphone without DLL loaded.");
		if( fn     ) fn( csip );
		if( ip     ) config_vpn( ip );
		if( codecs ) config_codecs( codecs );
	}
	void listen() {
		LOAD_DLL_FUNCTION(int (*fn)(), "softphone_listen");
		if( fn ) fn();
	}
	void connect(const char *account) {
		LOAD_DLL_FUNCTION(int (*fn)(const char*), "softphone_connect");
		if( fn && account ) fn( account );
	}
	void destroy() {
		LOAD_DLL_FUNCTION(int (*fn)(), "softphone_destroy");
		if( fn ) fn();
	}
	
	struct DLL_Loader {
		DLL_Loader() {
			// Load DLL
			hDLL = LoadLibraryExW( L"SoftPhone.dll", NULL, LOAD_IGNORE_CODE_AUTHZ_LEVEL );
			switch(DWORD err = hDLL ? NO_ERROR : GetLastError()) {
				case NO_ERROR: break;
				default: warnln("Failed to load SoftPhone.dll");
			}
			// Set printing functions for the library
			config(dprintf, eprintf);
		};
		~DLL_Loader() { FreeLibrary(hDLL); };
	} load;
#undef LOAD_DLL_FUNCTION
};
#endif