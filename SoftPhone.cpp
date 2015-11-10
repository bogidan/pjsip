
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define lengthOf(a) (sizeof(a)/sizeof(*a))

#ifdef _DEBUG
#pragma comment(lib, "libpjproject-i386-Win32-vc8-Debug-Dynamic.lib")
#else
#pragma comment(lib, "libpjproject-i386-Win32-vc8-Release-Dynamic.lib")
#endif
#pragma comment(lib, "Ws2_32.lib")

#include "SoftPhone.DLL.h"
#include "../bob/debug.h"

int null_printf(const char* fmt, ...) { return -1; }
print_ft dprintf = null_printf, eprintf = null_printf;
extern "C" void softphone_config_log( print_ft dprint, print_ft eprint ) {
	dprintf = dprint; eprintf = eprint;
}


#define _export(type,name) extern "C" type softphone_##name
// External printing implementation
//typedef int (__cdecl *print_f)(const char*, ...);
//static int print_null(const char* fmt, ...) { return -1; }
//print_f print = print_null, warn = print_null, error = print_null;
//
//_export(void,config_log) ( print_f _print, print_f _warn, print_f _error ) {
//	print = _print; warn = _warn; 
//}

#include <pj/types.h>
#include <pjsua.h>

static const u16 sin_440Hz[] = {
4096,3853,3156,2085,767,-640,-1973,-3072,-3808,-4093,-3895,-3236,-2194,-893,513,1859,2985,3759,4087,3933,3313,2302,1018,-385,-1743,-2896,-3706,-4077,-3967,-3387
,-2407,-1142,257,1626,2803,3649,4063,3997,3458,2510,1265,-128,-1507,-2708,-3589,-4045,-4023,-3525,-2610,-1387,0,1387,2610,3525,4023,4045,3589,2708,1507,128
,-1265,-2510,-3458,-3997,-4063,-3649,-2803,-1626,-257,1142,2407,3387,3967,4077,3706,2896,1743,385,-1018,-2302,-3313,-3933,-4087,-3759,-2985,-1859,-513,893,2194,3236
,3895,4093,3808,3072,1973,640,-767,-2085,-3156,-3853,-4096,-3853,-3156,-2085,-767,640,1973,3072,3808,4093,3895,3236,2194,893,-513,-1859,-2985,-3759,-4087,-3933
,-3313,-2302,-1018,385,1743,2896,3706,4077,3967,3387,2407,1142,-257,-1626,-2803,-3649,-4063,-3997,-3458,-2510,-1265,128,1507,2708,3589,4045,4023,3525,2610,1387
,0,-1387,-2610,-3525,-4023,-4045,-3589,-2708,-1507,-128,1265,2510,3458,3997,4063,3649,2803,1626,257,-1142,-2407,-3387,-3967,-4077,-3706,-2896,-1743,-385,1018,2302
,3313,3933,4087,3759,2985,1859,513,-893,-2194,-3236,-3895,-4093,-3808,-3072,-1973,-640,767,2085,3156,3853};

struct softphone_config_pj {
	struct softphone_config *cfg;
	pjsua_transport_id       transp_id;
	pjsua_acc_id             acc_id;
};

struct direct_port {
	pjmedia_port             base;
	pjsua_conf_port_id       id;
	pj_bool_t                eof;
	pj_timestamp             timestamp;
	pj_pool_t               *pool;
	struct softphone_config *cfg;
};

#include <pjmedia/port.h>
#include <pj/assert.h>
#include <pj/errno.h>
#include <pj/pool.h>
#include <pjsip-ua/sip_inv.h>

#define PJMEDIA_SIG_PORT_DIRECT	PJMEDIA_SIG_CLASS_PORT_AUD('D','P')
#define SIGNATURE PJMEDIA_SIG_PORT_DIRECT
#define BYTES_PER_SAMPLE 2


// SIP Handlers
static void on_incoming_call( pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata );
static void on_call_state(pjsua_call_id call_id, pjsip_event *e);
static void on_call_sdp_created(pjsua_call_id call_id, pjmedia_sdp_session *sdp, pj_pool_t *pool, const pjmedia_sdp_session *rem_sdp);
static void on_call_media_state(pjsua_call_id call_id);
static void on_dtmf_digit( pjsua_call_id call_id, int digit );
static int error_exit(const char *title, pj_status_t status)
{
	dprintf( "Error: %s [%u]\n",  title, status );
	return softphone_destroy();
}
// Audio Handlers
static pj_status_t put_frame(pjmedia_port *this_port, pjmedia_frame *frame);
static pj_status_t get_frame(pjmedia_port *this_port, pjmedia_frame *frame);
static pj_status_t on_destroy(pjmedia_port *this_port);

static struct softphone_config_pj csip = {0};
extern "C" int softphone_init( struct softphone_config *_csip ) {
	pj_status_t status;

	csip.cfg = _csip;
	// Create pjsua first!
	status = pjsua_create();
	if (status != PJ_SUCCESS) error_exit("Error in pjsua_create()", status);
	
	// Disable default sound setup
//	pjsua_set_null_snd_dev();
//	pjsua_set_no_snd_dev(); // called in pjsua_config_default();

	{	// Init pjsua
		pjsua_config cfg;
		pjsua_logging_config log_cfg;
		pjsua_media_config media_cfg; // http://www.pjsip.org/pjsip/docs/html/structpjsua__media__config.htm

		pjsua_config_default(&cfg);
		cfg.cb.on_incoming_call = &on_incoming_call;
		cfg.cb.on_call_media_state = &on_call_media_state;
		cfg.cb.on_call_sdp_created = &on_call_sdp_created;
		cfg.cb.on_call_state = &on_call_state;
		cfg.cb.on_dtmf_digit = &on_dtmf_digit;
	//	cfg.stun_host = pj_str(""); // disable stun server
		
		pjsua_logging_config_default(&log_cfg);
		log_cfg.console_level = 5;
	//	log_cfg.level = 4;
	//	log_cfg.log_filename = pj_str("sip.log");

		pjsua_media_config_default(&media_cfg);
		media_cfg.snd_auto_close_time = -1; // disable auto close
		media_cfg.no_vad = 1; // Disable Voice Activity Detector (VAD)
	//	media_cfg.jb_init = -1;
	//	media_cfg.jb_max = -1;
	//	media_cfg.jb_max_pre = -1;
	//	media_cfg.jb_min_pre = -1;
		media_cfg.clock_rate = 8000;
		media_cfg.audio_frame_ptime = 80; // 20
		media_cfg.ptime = 80; // 20
		media_cfg.has_ioqueue = true;
	//	media_cfg.tx_drop_pct = 1;

		status = pjsua_init(&cfg, &log_cfg, &media_cfg);
		if( status != PJ_SUCCESS ) return error_exit("Error in pjsua_init()", status);

		// Cancel echo cancellation (Disabled in Library Header)
	//	status = pjsua_set_ec(0, 0);
	//	if( status != PJ_SUCCESS ) return error_exit("Error in pjsua_set_ec()", status);
		
		{	// Filter acceptable codecs
			pjsua_codec_info info[32]; // sizeof returns zero
			unsigned count = 32;
			if( pjsua_enum_codecs(info, &count) == PJ_SUCCESS ) {
				auto i = info - 1;
				while( ++i < info + count ) {
					if( strstr( i->codec_id.ptr, "PCMU" ) ) {
						println("Codec: %s - %d", i->codec_id.ptr, i->priority);
					} else pjsua_codec_set_priority( &i->codec_id, 0 );
				}
			}
		}
	}

	{	// Create UDP transport.
		pjsua_transport_config cfg;
		pjsua_transport_config_default(&cfg);
		cfg.port = 5060;
		status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, &csip.transp_id);
		if (status != PJ_SUCCESS) return error_exit("Error creating transport", status);
	}

	// Initialization is done, now start pjsua
	status = pjsua_start();
	if (status != PJ_SUCCESS) return error_exit("Error starting pjsua", status);
	status = pjsua_set_null_snd_dev();
	if (status != PJ_SUCCESS) return error_exit("Error setting NULL device", status);

	return PJ_SUCCESS;
}
extern "C" int softphone_listen() {
	pj_status_t status;

	if( csip.cfg == NULL ) return PJ_EINVALIDOP;
	if( pjsua_acc_is_valid( csip.acc_id ) )
		pjsua_acc_del( csip.acc_id );
	
	status = pjsua_acc_add_local( csip.transp_id, PJ_TRUE, &csip.acc_id);
	if (status != PJ_SUCCESS) return error_exit("Error adding account", status);

	pjsua_acc_set_user_data(csip.acc_id, csip.cfg);
	return PJ_SUCCESS;
}

/* SIP reference
	Dev -> 831@172.16.0.20 < kombea
Kunnect -> dns(ip?):15606
	ACS ACSER5 -> only listen
	ACS ACSER3 -> {{sip_station}}@sf-at904-{{sip_station/24+1}}.acs.local < 1234
	JAK -> JAK_TRAINER_01@jak.sip.kunnect.com:15606 < train
*/
int softphone_connect( char *domain, char *username, char *password ) {
	pj_status_t status;
	pjsua_acc_config cfg;
	cstr_t id;
	cstr_t uri;

	if( csip.cfg == NULL ) return PJ_EINVALIDOP;
	if( pjsua_acc_is_valid( csip.acc_id ) )
		pjsua_acc_del( csip.acc_id );

	// Register to SIP domain by creating SIP account.
	pjsua_acc_config_default(&cfg);
	sprintf_s( id, "sip:%s@%s", username, domain );
	cfg.id = pj_str(id);
	sprintf_s( uri, "sip:%s", domain );
	cfg.reg_uri = pj_str(uri);
	cfg.cred_count = 1;
	cfg.media_stun_use = PJSUA_STUN_USE_DISABLED;
	cfg.cred_info[0].realm = pj_str("*");//pj_str(domain);
	cfg.cred_info[0].scheme = pj_str("digest");
	cfg.cred_info[0].username = pj_str(username);
	cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
	cfg.cred_info[0].data = pj_str(password);
	cfg.user_data = csip.cfg;

	status = pjsua_acc_add(&cfg, PJ_FALSE, &csip.acc_id);
	if (status != PJ_SUCCESS) return error_exit("Error adding account", status);

	return PJ_SUCCESS;
}
extern "C" int softphone_connect_account( const char *account ) {
	cstr_t buf; strcpy(buf, account);
	char *username = buf, *domain = strchr(buf, '@') + 1, *password = strchr(buf, ':') + 1;
	domain[-1] = password[-1] = '\0';
	return softphone_connect( domain, username, password );
}
extern "C" int softphone_destroy() {
	csip.cfg = NULL;
	pjsua_destroy();
	return PJ_SUCCESS;
}

pj_str_t vpn_ip;
char vpn_ip_str[18] = "";//"172.16.0.220";
extern "C" void softphone_force_vpn_ip( const char* ip ) {
	strcpy_s(vpn_ip_str, ip);
	vpn_ip = pj_str(vpn_ip_str);
}

//========================================================
// SIP Handlers
//========================================================

static void on_incoming_call( pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata ) {
	pjsua_call_info ci;
	pj_pool_t *pool;
	struct softphone_config *cfg = NULL;
	struct direct_port *port = NULL;

	PJ_UNUSED_ARG(rdata);

	cfg = (struct softphone_config*) pjsua_acc_get_user_data( acc_id );
	pjsua_call_get_info(call_id, &ci);
//	dprintf( "Incoming call from %.*s!!\n", (int)ci.remote_info.slen, ci.remote_info.ptr );

	if( cfg->available) {
		pool = pjsua_pool_create( NULL, 256, 256 );
		port = PJ_POOL_ZALLOC_T(pool, struct direct_port);
		port->cfg = cfg;
		port->pool = pool;
		cfg->available = false;
		
		pjsua_call_set_user_data( call_id, port );
		// 200 OK - Indicates the request was successful
		pjsua_call_answer(call_id, 200, NULL, NULL);
	} else {
		// 486 Busy Here - Callee is Busy
		pjsua_call_answer(call_id, 486, NULL, NULL);
	}
}

// Callback called by the library when call's state has changed
static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
	pjsua_call_info ci;
	struct direct_port *port;

	PJ_UNUSED_ARG(e);

	pjsua_call_get_info(call_id, &ci);
	port = (struct direct_port*) pjsua_call_get_user_data(call_id);

	switch( ci.state ) {
//	case PJSIP_INV_STATE_NULL: // Before INVITE is sent or received
//	case PJSIP_INV_STATE_CALLING: // After INVITE is sent
//	case PJSIP_INV_STATE_INCOMING: // After INVITE is received.
//	case PJSIP_INV_STATE_EARLY: // After response with To tag.
//	case PJSIP_INV_STATE_CONNECTING: // After 2xx is sent/received.
//	case PJSIP_INV_STATE_CONFIRMED: // After ACK is sent/received.
	case PJSIP_INV_STATE_DISCONNECTED: // Session is terminated.
		port->cfg->available = true;
	}

	dprintf( "Call %d state=%.*s\n", call_id, (int)ci.state_text.slen, ci.state_text.ptr );
}


static pj_status_t pjmedia_direct_port_create( struct direct_port *port ) {
	pj_status_t status;
	unsigned channel_count = 1;
	unsigned clock_rate = 8000; // 22000 // 22050
	unsigned bits_per_sample = 16;
	unsigned samples_per_frame = 64; // 80 // 110; // 220

	const pj_str_t name = pj_str("direct");

	/* Create the port */
	status = pjmedia_port_info_init( &port->base.info, &name, SIGNATURE, clock_rate, channel_count, bits_per_sample, samples_per_frame);
	if( status != PJ_SUCCESS ) { return status; }
	
	port->base.put_frame = &put_frame;
	port->base.get_frame = &get_frame;
	port->base.on_destroy = &on_destroy;
	port->timestamp.u64 = 0;
	port->eof = PJ_FALSE;

	return PJ_SUCCESS;
}

static void on_call_sdp_created(pjsua_call_id call_id, pjmedia_sdp_session *sdp, pj_pool_t *pool, const pjmedia_sdp_session *rem_sdp)
{
	if( strlen(vpn_ip_str) == 0 ) return;
	// Manual entry of IP for VPNs
	sdp->origin.addr = vpn_ip;
	for( u32 i = 0; i < sdp->media_count; i++) {
		sdp->media[i]->conn->addr = vpn_ip;
	}
}


pjmedia_port *mem_player;
pjsua_conf_port_id render_id = 0;
// Callback called by the library when call's media state has changed
static void on_call_media_state(pjsua_call_id call_id)
{
	pj_status_t status;
	pjsua_call_info ci;
	struct direct_port *port;

	port = (struct direct_port*) pjsua_call_get_user_data(call_id);

	pjsua_call_get_info(call_id, &ci);

	switch( ci.media_status ) {
	case PJSUA_CALL_MEDIA_ACTIVE:
	//	status = pjmedia_direct_port_create( port );
	//	if( status != PJ_SUCCESS ) return;
	//	status = pjsua_conf_add_port( port->pool, &port->base, &port->id );
	//	if( status != PJ_SUCCESS ) { return; }
	//	// When media is active, connect call to direct_port
	//	pjsua_conf_connect( port->id, ci.conf_slot );
	//	pjsua_conf_connect( ci.conf_slot, port->id );
		// When media is active, connect call to sound device.
	//	pjsua_conf_connect(ci.conf_slot, 0);
	//	pjsua_conf_connect(0, ci.conf_slot);
		// Test of memplayer
		status = pjmedia_mem_player_create( port->pool, sin_440Hz, sizeof(sin_440Hz), 8000, 1, 441, 16, 0, &mem_player );
		status = pjsua_conf_add_port( port->pool, mem_player, &render_id );
		status = pjsua_conf_connect( render_id, ci.conf_slot );
		
		pjsua_call_set_user_data(call_id, port);
		break;
	case PJSUA_CALL_MEDIA_LOCAL_HOLD:
	case PJSUA_CALL_MEDIA_REMOTE_HOLD:
	case PJSUA_CALL_MEDIA_ERROR:
		break;
	case PJSUA_CALL_MEDIA_NONE:
		
			pjsua_conf_disconnect( render_id, ci.conf_slot );
			pjsua_conf_remove_port( render_id );
			pjmedia_port_destroy( mem_player );
			render_id = 0;
			mem_player = NULL;

		if( port->id ) {
			pjsua_conf_disconnect( port->id, ci.conf_slot );
			pjsua_conf_disconnect( ci.conf_slot, port->id );
			pjsua_conf_remove_port( port->id );
			pjmedia_port_destroy( &port->base );
		}
		if( ! port->cfg->available ) port->cfg->available = true;

		pj_pool_release( port->pool );
		break;
	}
}

static void on_dtmf_digit( pjsua_call_id call_id, int digit ) {
	PJ_UNUSED_ARG(call_id);

	println(" DTMF_Recieved:  %d  %c", digit, digit );
}

//========================================================
// Audio Handlers
//========================================================

static pj_status_t put_frame( pjmedia_port *this_port, pjmedia_frame *frame)
{
	struct direct_port *port;

	PJ_ASSERT_RETURN(this_port->info.signature == SIGNATURE, PJ_EINVALIDOP);

	port = (struct direct_port*) this_port;
	if( port->eof ) return PJ_EEOF;

	s16 *src = (s16*) frame->buf;
	u32 size = frame->size;

	port->cfg->push( pU08(frame->buf), size / sizeof(s16) );
	return PJ_SUCCESS;
}

static pj_status_t get_frame( pjmedia_port *this_port, pjmedia_frame *frame)
{
	struct direct_port *port;

	PJ_ASSERT_RETURN(this_port->info.signature == SIGNATURE, PJ_EINVALIDOP);

	port = (struct direct_port*) this_port;
	if( port->eof ) return PJ_EEOF;

	s16 *dst = (s16*) frame->buf;
	u32 size = frame->size / sizeof(s16);// = PJMEDIA_PIA_AVG_FSZ(&this_port->info);

	port->cfg->pull( pU08(frame->buf), size );
	frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
	return PJ_SUCCESS;
//	pj_get_timestamp( &frame->timestamp );
//	frame->timestamp.u64 = port->timestamp.u64;
//	port->timestamp.u64 += PJMEDIA_PIA_SPF(&this_port->info);
}

static pj_status_t on_destroy(pjmedia_port *this_port)
{
	struct direct_port *port;

	PJ_ASSERT_RETURN(this_port->info.signature == SIGNATURE, PJ_EINVALIDOP);

	port = (struct direct_port*) this_port;
	port->eof = PJ_TRUE;

	return PJ_SUCCESS;
}
