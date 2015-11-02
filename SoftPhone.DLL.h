
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

#ifdef _WINDLL // For export as dll
#define DLL_CALL __declspec(dllexport)
extern "C" void DLL_CALL softphone_config_log( print_ft, print_ft );
extern "C" void DLL_CALL softphone_force_vpn_ip( const char* ip );
extern "C" int DLL_CALL softphone_init( struct softphone_config *csip );
extern "C" int DLL_CALL softphone_listen();
extern "C" int DLL_CALL softphone_connect_account( const char *account );
//extern "C" int DLL_CALL softphone_connect( char *domain, char *username, char *password );
extern "C" int DLL_CALL softphone_destroy();
#undef DLL_CALL
#elif _WINLIB

#else // For Loading as dll

#define LOAD_FUNCTION(var,name) var = (decltype(var)) GetProcAddress(hDLL, name);
class SoftPhone {
	HMODULE hDLL;
	void (*config)(print_ft, print_ft);
	void (*config_vpn)(const char *ip);
	int (*init)(struct softphone_config*);
	int (*listen)();
	int (*connect)(const char *account);
	int (*destroy)();
private:
	SoftPhone() {
		// Attempt to Load Library
		if( hDLL == NULL ) hDLL = LoadLibraryExW( L"SoftPhone.dll", NULL, 0);
		switch(DWORD err = hDLL ? NO_ERROR : GetLastError()) {
			case NO_ERROR: break;
			default: warnln("Failed to load SoftPhone.dll"); return;
		}
		// Load Functions
		LOAD_FUNCTION( config,     "softphone_config_log" );
		LOAD_FUNCTION( config_vpn, "softphone_force_vpn_ip" );
		LOAD_FUNCTION( init,       "softphone_init" );
		LOAD_FUNCTION( listen,     "softphone_listen" );
		LOAD_FUNCTION( connect,    "softphone_connect_account" );
		LOAD_FUNCTION( destroy,    "softphone_destroy" );
		// Verify Loaded
		if( config ) config(dprintf, eprintf);
		if( init && listen && connect && destroy );
		else warnln("Failed to load functions from SoftPhone.dll");
	}
	~SoftPhone() { FreeLibrary(hDLL); };
	static SoftPhone& GetInstance() {
		static SoftPhone main;
		return main;
	};
	void Init(struct softphone_config *csip, const char *addr ) {
		if( !hDLL ) warnln("Using softphone without DLL loaded.");
		if( init ) init(csip);
		if( addr && config_vpn ) config_vpn( addr );
	};
	void Listen() {
		if( listen ) listen();
	};
	void Connect(const char *account) {
		if( connect ) connect(account);
	};
	void Destroy() {
		if( destroy ) destroy();
	};

	friend struct SoftPhoneDLL;
};

struct SoftPhoneDLL {
	static void stInit(struct softphone_config *csip,  const char *addr ) {
		SoftPhone::GetInstance().Init(csip, addr);
	}
	static void stListen() {
		SoftPhone::GetInstance().Listen();
	}
	static void stConnect(const char *account) {
		SoftPhone::GetInstance().Connect(account);
	}
	static void stDestroy() {
		SoftPhone::GetInstance().Destroy();
	}
};

#endif