/*
=================================================================================
 Name        : video_entryphone.c
 Version     : 0.1

 Copyright (C) 2014 by Vito Ammirata, 2014, vito.ammirata@gmail.com
                       Giuseppe Zeppieri, 2014, g.zeppieri@libero.it

 Description :
     Video Entryphone over SIP/VOIP with PJSUA library.

 Dependencies:
	- PJSUA API (PJSIP)
 
 References  :
 http://www.pjsip.org/
 http://www.pjsip.org/docs/latest/pjsip/docs/html/group__PJSUA__LIB.htm
 http://binerry.de/post/29180946733/raspberry-pi-caller-and-answering-machine
 
================================================================================
This tool is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This tool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
================================================================================
*/

// definition of endianess (e.g. needed on raspberry pi)
#define PJ_IS_LITTLE_ENDIAN 1
#define PJ_IS_BIG_ENDIAN 0

#define PJMEDIA_HAS_VIDEO 1

// includes
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pjsua-lib/pjsua.h>

// disable pjsua logging
#define PJSUA_LOG_LEVEL 0

// define max supported dtmf settings
#define MAX_DTMF_SETTINGS 9

#define THIS_FILE "video_entryphone"

// struct for app dtmf settings
struct dtmf_config { 
	int id;
	int active;
	int processing_active;
	char *description;
	char *cmd;
};

// struct for app configuration settings
struct app_config { 
	char *sip_domain;
	char *sip_user;
	char *sip_password;
	int   log_level;
	char *log_file;
	struct dtmf_config dtmf_cfg[MAX_DTMF_SETTINGS];
} app_cfg;  

// global helper vars
int call_confirmed = 0;
int media_counter = 0;
int app_exiting = 0;

// global vars for pjsua
pjsua_acc_id acc_id;
pjsua_call_id current_call = PJSUA_INVALID_ID;

// header of helper-methods
static void parse_config_file(char *);
static void register_sip(void);
static void setup_sip(void);
static void usage(int);
static int try_get_argument(int, char *, char **, int, char *[]);

// header of callback-methods
static void on_incoming_call(pjsua_acc_id, pjsua_call_id, pjsip_rx_data *);
static void on_call_media_state(pjsua_call_id);
static void on_call_state(pjsua_call_id, pjsip_event *);
static void on_dtmf_digit(pjsua_call_id, int);
static void signal_handler(int);
static char *trim_string(char *);

// header of app-control-methods
static void app_exit();
static void error_exit(const char *, pj_status_t);

// main application
int main(int argc, char *argv[])
{
	// init LOG level
	app_cfg.log_level = PJSUA_LOG_LEVEL;

	// init dtmf settings (dtmf 0 is reserved for exit call)
	int i;
	for (i = 0; i < MAX_DTMF_SETTINGS; i++)
	{
		struct dtmf_config *d_cfg = &app_cfg.dtmf_cfg[i];
		d_cfg->id = i+1;
		d_cfg->active = 0;
		d_cfg->processing_active = 0;
	}

	// parse arguments
	if (argc > 1)
	{
		int arg;
		for( arg = 1; arg < argc; arg+=2 )
		{
			// check if usage info needs to be displayed
			if (!strcasecmp(argv[arg], "--help"))
			{
				// display usage info and exit app
				usage(0);
				exit(0);			
			}
			
			// check for config file location
			if (!strcasecmp(argv[arg], "--config-file"))
			{
				if (argc >= (arg+1))
				{
					app_cfg.log_file = argv[arg+1];	
				}
				continue;
			}
		}
	} 
	
	if (!app_cfg.log_file)
	{
		// too few arguments specified - display usage info and exit app
		usage(1);
		exit(1);
	}
	
	// read app configuration from config file
	parse_config_file(app_cfg.log_file);
	
	if (!app_cfg.sip_domain || !app_cfg.sip_user || !app_cfg.sip_password)
	{
		// too few arguments specified - display usage info and exit app
		usage(1);
		exit(1);
	}
	
	// print infos
	//PJ_LOG(3,(THIS_FILE, "Video Entryphone - DTMF-based                     \n"));
	//PJ_LOG(3,(THIS_FILE, "==================================================\n"));
	
	// register signal handler for break-in-keys (e.g. ctrl+c)
	signal(SIGINT, signal_handler);
	signal(SIGKILL, signal_handler);
	
	for (i = 0; i < MAX_DTMF_SETTINGS; i++)
	{
		struct dtmf_config *d_cfg = &app_cfg.dtmf_cfg[i];
		
		if (d_cfg->active == 1)
		{
			printf("SSSSSS\n"); 
		}
	}
	
	//PJ_LOG(3,(THIS_FILE, "Done.\n"));
	
	
	
	// setup up sip library pjsua
	setup_sip();
	
	// create account and register to sip server
	register_sip();
	
	printf("Camera: %d\n", pjsua_vid_dev_count());
	
	// app loop
	for (;;) {
		// Codice di chiamata, lo sleep èusato solo per non consumare tutta la cpu sostituire con un thread che legge i tasti
		pj_str_t uri = pj_str("sip:1012@pbx.vmh.it");
		sleep(10);
		pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, NULL);
	}
	
	// exit app
	app_exit();
	
	return 0;
}

// helper for displaying usage infos
static void usage(int error)
{
	if (error == 1)
	{
		puts("Error, to few arguments.");
		puts  ("");
	}
	puts  ("Usage:");
	puts  ("  video_entryphone [options]");
	puts  ("");
	puts  ("Commandline:");
	puts  ("Mandatory options:");
	puts  ("  --config-file <string>   Set config file");
	puts  ("");
	puts  ("");
	puts  ("Config file:");
	puts  ("Mandatory options:");
	puts  ("  sd=string   Set sip provider domain.");
	puts  ("  su=string   Set sip username.");
	puts  ("  sp=string   Set sip password.");
	puts  ("");
	puts  ("  ll=0        Set LOG level.");
	puts  ("");
	puts  (" and at least one dtmf configuration (X = dtmf-key index):");
	puts  ("  dtmf.X.active=int           Set dtmf-setting active (0/1).");
	puts  ("  dtmf.X.description=string   Set description.");
	puts  ("  dtmf.X.cmd=string           Set dtmf command.");
	puts  ("");
	
	fflush(stdout);
}

// helper for parsing command-line-argument
static int try_get_argument(int arg, char *arg_id, char **arg_val, int argc, char *argv[])
{
	int found = 0;
	
	// check if actual argument is searched argument
	if (!strcasecmp(argv[arg], arg_id)) 
	{
		// check if actual argument has a value
		if (argc >= (arg+1))
		{
			// set value
			*arg_val = argv[arg+1];			
			found = 1;
		}
	}	
	return found;
}

// helper for parsing config file
static void parse_config_file(char *cfg_file)
{
	// open config file
	char line[200];
	FILE *file = fopen(cfg_file, "r");
	
	if (file!=NULL)
	{
		// start parsing file
		while(fgets(line, 200, file) != NULL)
		{
			char *arg, *val;
			
			// ignore comments and just new lines
			if(line[0] == '#') continue;
			if(line[0] == '\n') continue;
			
			// split string at '='-char
			arg = strtok(line, "=");
			if (arg == NULL) continue;			
			val = strtok(NULL, "=");
			
			// check for new line char and remove it
			char *nl_check;
			nl_check = strstr (val, "\n");
			if (nl_check != NULL) strncpy(nl_check, " ", 1);
			
			// remove trailing spaces
			
			
			// duplicate string for having own instance of it
			char *arg_val = strdup(val);	
			
			// check for sip domain argument
			if (!strcasecmp(arg, "sd")) 
			{
				app_cfg.sip_domain = trim_string(arg_val);
				continue;
			}
			
			// check for sip user argument
			if (!strcasecmp(arg, "su")) 
			{
				app_cfg.sip_user = trim_string(arg_val);
				continue;
			}
			
			// check for sip domain argument
			if (!strcasecmp(arg, "sp")) 
			{
				app_cfg.sip_password = trim_string(arg_val);
				continue;
			}
			
			// check for LOG level argument
			if (!strcasecmp(arg, "ll")) 
			{
				app_cfg.log_level = atoi(trim_string(arg_val));
				continue;
			}
			
			// check for a dtmf argument
			char dtmf_id[1];
			char dtmf_setting[25];
			if(sscanf(arg, "dtmf.%1[^.].%s", dtmf_id, dtmf_setting) == 2)
			{
				// parse dtmf id (key)
				int d_id;
				d_id = atoi(dtmf_id);	

				// check if actual dtmf id blasts maxium settings 
				if (d_id >= MAX_DTMF_SETTINGS) continue;
				
				// get pointer to actual dtmf_cfg entry
				struct dtmf_config *d_cfg = &app_cfg.dtmf_cfg[d_id-1];
				
				// check for dtmf active setting
				if (!strcasecmp(dtmf_setting, "active"))
				{
					d_cfg->active = atoi(val);
					continue;
				}
				
				// check for dtmf description setting
				if (!strcasecmp(dtmf_setting, "description"))
				{
					d_cfg->description = arg_val;
					continue;
				}

				// check for dtmf cmd setting
				if (!strcasecmp(dtmf_setting, "cmd"))
				{
					d_cfg->cmd = arg_val;
					continue;
				}
			}
			
			// write warning if unknown configuration setting is found
			//PJ_LOG(3,(THIS_FILE, "Warning: Unknown configuration with arg '%s' and val '%s'\n", arg, val));
		}
	}
	else
	{
		// return if config file not found
		//PJ_LOG(3,(THIS_FILE, "Error while parsing config file: Not found.\n"));
		exit(1);
	}
}

// helper for removing leading and trailing strings (source taken from kernel source, lib/string.c)
static char *trim_string(char *str)
{
	while (isspace(*str)) ++str;
	
	char *s = (char *)str;
	
	size_t size;
	char *end;

	size = strlen(s);
	if (!size) return s;

	end = s + size - 1;
	while (end >= s && isspace(*end)) end--;
	*(end + 1) = '\0';

	return s;
}

// helper for setting up sip library pjsua
static void setup_sip(void)
{
	pj_status_t status;
	
	printf("Setting up pjsua ... \n");
	
	// create pjsua  
	status = pjsua_create();
	if (status != PJ_SUCCESS) error_exit("Error in pjsua_create()", status);
	
	// configure pjsua	
	pjsua_config cfg;
	pjsua_config_default(&cfg);
	
	// enable just 1 simultaneous call 
	cfg.max_calls = 1;
	
	// callback configuration	
	cfg.cb.on_incoming_call = &on_incoming_call;
	cfg.cb.on_call_media_state = &on_call_media_state;
	cfg.cb.on_call_state = &on_call_state;
	cfg.cb.on_dtmf_digit = &on_dtmf_digit;
		
	// logging configuration
	pjsua_logging_config log_cfg;		
	pjsua_logging_config_default(&log_cfg);
	log_cfg.console_level = app_cfg.log_level;
	
	// media configuration
	pjsua_media_config media_cfg;
	pjsua_media_config_default(&media_cfg);
	media_cfg.snd_play_latency = 100;
	media_cfg.thread_cnt = 1;
	media_cfg.clock_rate = 8000;
	media_cfg.snd_clock_rate = 8000;
	media_cfg.quality = 10;
	
	// initialize pjsua 
	status = pjsua_init(&cfg, &log_cfg, &media_cfg);
	if (status != PJ_SUCCESS) error_exit("Error in pjsua_init()", status);
	
	// add udp transport
	pjsua_transport_config udpcfg;
	pjsua_transport_config_default(&udpcfg);
		
	udpcfg.port = 5060;
	status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &udpcfg, NULL);
	if (status != PJ_SUCCESS) error_exit("Error creating transport", status);
	
	// initialization is done, start pjsua
	status = pjsua_start();
	if (status != PJ_SUCCESS) error_exit("Error starting pjsua", status);
	
	printf("Done.\n");
}

// helper for creating and registering sip-account
static void register_sip(void)
{
	pj_status_t status;
	
	printf("Registering account ... ");
	
	// prepare account configuration
	pjsua_acc_config cfg;
	pjsua_acc_config_default(&cfg);
	
	// build sip-user-url
	char sip_user_url[40];
	sprintf(sip_user_url, "sip:%s@%s", app_cfg.sip_user, app_cfg.sip_domain);
	
	// build sip-provder-url
	char sip_provider_url[40];
	sprintf(sip_provider_url, "sip:%s", app_cfg.sip_domain);
	
	// create and define account
	cfg.id = pj_str(sip_user_url);
	cfg.reg_uri = pj_str(sip_provider_url);
	cfg.cred_count = 1;
	cfg.cred_info[0].realm = pj_str("*");
	cfg.cred_info[0].scheme = pj_str("digest");
	cfg.cred_info[0].username = pj_str(app_cfg.sip_user);
	cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
	cfg.cred_info[0].data = pj_str(app_cfg.sip_password);
	
	cfg.vid_out_auto_transmit = PJ_TRUE; //Vito
	cfg.vid_in_auto_show = PJ_FALSE; //Vito
	cfg.vid_cap_dev = PJMEDIA_VID_DEFAULT_CAPTURE_DEV; //Vito
	cfg.vid_rend_dev = PJMEDIA_VID_DEFAULT_RENDER_DEV; //Vito
	
	// add account
	status = pjsua_acc_add(&cfg, PJ_TRUE, &acc_id);
	if (status != PJ_SUCCESS) error_exit("Error adding account", status);
	
	printf("Done.\n");
}

// handler for incoming-call-events
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
{
	// get call infos
	pjsua_call_info ci;
	pjsua_call_get_info(call_id, &ci);
	
	PJ_UNUSED_ARG(acc_id);
	PJ_UNUSED_ARG(rdata);
	
	current_call = call_id;

	printf("Incoming call from %.*s!!\n", (int)ci.remote_info.slen, ci.remote_info.ptr);
	
	// automatically answer incoming call with 200 status/OK 
	pjsua_call_answer(call_id, 200, NULL, NULL);
}

// handler for call-media-state-change-events
static void on_call_media_state(pjsua_call_id call_id)
{
	// get call infos
	pjsua_call_info ci; 
	pjsua_call_get_info(call_id, &ci);
	
	pjsua_call_vid_strm_op op = PJSUA_CALL_VID_STRM_CHANGE_DIR;
	pjsua_call_vid_strm_op_param vop;
	
	vop.med_idx = 0;
	if (0 <= vop.med_idx && vop.med_idx < ci.media_cnt && ci.media[vop.med_idx].type == PJMEDIA_TYPE_VIDEO) {
		if (ci.media[vop.med_idx].status == PJSUA_CALL_MEDIA_NONE || ci.media[vop.med_idx].dir == PJMEDIA_DIR_NONE) {
			vop.dir = PJMEDIA_DIR_ENCODING;
		} else {
			op = PJSUA_CALL_VID_STRM_START_TRANSMIT;
		}
	} else {
		return;
	}
	
	pj_status_t status = PJ_ENOTFOUND;
	pjsua_call_set_vid_strm(call_id, op, &vop);

	// check state if call is established/active
	if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
		pjsua_conf_connect(ci.conf_slot, 0);
		pjsua_conf_connect(0, ci.conf_slot);
		printf("Call media activated.\n");
	} 
}

// handler for call-state-change-events
static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
	// get call infos
	pjsua_call_info ci;
	pjsua_call_get_info(call_id, &ci);
	
	// prevent warning about unused argument e
	PJ_UNUSED_ARG(e);
	
	// check call state
	if (ci.state == PJSIP_INV_STATE_CONFIRMED) 
	{
		printf("Call confirmed.\n");
		
		call_confirmed = 1;
	}
	if (ci.state == PJSIP_INV_STATE_DISCONNECTED) 
	{
		printf("Call disconnected.\n");
	}
	
	printf("Call %d state=%.*s\n", call_id, (int)ci.state_text.slen, ci.state_text.ptr);
}

// handler for dtmf-events
static void on_dtmf_digit(pjsua_call_id call_id, int digit) 
{
	// get call infos
	pjsua_call_info ci; 
	pjsua_call_get_info(call_id, &ci);
	
	// work on detected dtmf digit
	int dtmf_key = digit - 48;
	
	printf("DTMF command detected: %i\n", dtmf_key);
	
	struct dtmf_config *d_cfg = &app_cfg.dtmf_cfg[dtmf_key-1];
	if (d_cfg->processing_active == 0)
	{
		d_cfg->processing_active = 1;
		
		if (d_cfg->active == 1)
		{
			printf("Active DTMF command found for received digit.\n");
			
			int error = 0;
			FILE *fp;
			
			fp = popen(d_cfg->cmd, "r");
			if (fp == NULL) {
				error = 1;
				printf(" (Failed to run command) ");
			}
			
			char result[20];
			if (!error)
			{
				if (fgets(result, sizeof(result)-1, fp) == NULL)
				{
					error = 1;
					//PJ_LOG(3,(THIS_FILE, " (Failed to read result) "));
				}
			}
			
			//PJ_LOG(3,(THIS_FILE, "Done.\n"));
		}
		else
		{
			printf("No active DTMF command found for received digit.\n");
		}
		
		d_cfg->processing_active = 0;
	}
	else
	{
		printf("DTMF command dropped - state is actual processing.\n");
	}
}

// handler for "break-in-key"-events (e.g. ctrl+c)
static void signal_handler(int signal) 
{
	// exit app
	app_exit();
}

// clean application exit
static void app_exit()
{
	if (!app_exiting)
	{
		app_exiting = 1;
		printf("Stopping application ... ");
		
		// hangup open calls and stop pjsua
		pjsua_call_hangup_all();
		pjsua_destroy();

		printf("Done.\n");
		
		exit(0);
	}
}

// display error and exit application
static void error_exit(const char *title, pj_status_t status)
{
	if (!app_exiting)
	{
		app_exiting = 1;
		
		pjsua_perror("SIP Call", title, status);
		
		// hangup open calls and stop pjsua
		pjsua_call_hangup_all();
		pjsua_destroy();
		
		exit(1);
	}
}


