#include <pj/types.h>
#include <pjsua.h>

#include "../bob/types.h"
#include "../bob/FastDelegate/FastDelegate.h"
typedef fastdelegate::FastDelegate2<u08*,u32,u32> DelBlockPush;
typedef fastdelegate::FastDelegate2<u08*,u32,u32> DelBlockPull;

struct direct_port {
	pjmedia_port             base;
	pjsua_conf_port_id       id;
	pj_bool_t                eof;
	pj_timestamp             timestamp;
	pj_pool_t               *pool;
	struct softphone_config *cfg;
};
struct softphone_config {
//	DelPush            dst;
//	DelPull            src;
	DelBlockPush       push;
	DelBlockPull       pull;
	bool               available;
	pjsua_transport_id transp_id;
	pjsua_acc_id       acc_id;
};

#ifdef _WINDLL
#define DLL_CALL __declspec(dllexport) 
#else
#pragma comment(lib, "softphone.lib")
#define DLL_CALL __declspec(dllimport) 
#endif

int DLL_CALL softphone_init( struct softphone_config *csip );
int DLL_CALL softphone_listen();
int DLL_CALL softphone_connect( const char *account );
int DLL_CALL softphone_connect( char *domain, char *username, char *password );
int DLL_CALL softphone_destroy();