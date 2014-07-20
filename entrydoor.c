#include <stdint.h>
#include <bcm2835.h>

#include <pjsua-lib/pjsua.h>

#include "configfile.h"
#include "gpio.h"

#define THIS_FILE "ENTRYDOOR"


/* Callback called by the library upon receiving incoming call */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata) {
 pjsua_call_info ci;

 pjsua_acc_config acc_cfg;
 pjsua_call_setting call_setting;

 PJ_UNUSED_ARG(acc_id);
 PJ_UNUSED_ARG(rdata);

 acc_cfg.vid_cap_dev = PJMEDIA_VID_DEFAULT_CAPTURE_DEV;
 acc_cfg.vid_out_auto_transmit = PJ_TRUE;
 call_setting.flag = PJSUA_CALL_INCLUDE_DISABLED_MEDIA;

 pjsua_call_get_info(call_id, &ci);

 PJ_LOG(3,(THIS_FILE, "Incoming call from %.*s!!", (int)ci.remote_info.slen, ci.remote_info.ptr));

 /* Automatically answer incoming calls with 200/OK */
 pjsua_call_answer(call_id, 200, NULL, NULL);
}

/* Callback called by the library when call's state has changed */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e) {
 pjsua_call_info ci;

 PJ_UNUSED_ARG(e);

 pjsua_call_get_info(call_id, &ci);
 PJ_LOG(3,(THIS_FILE, "Call %d state=%.*s", call_id, (int)ci.state_text.slen, ci.state_text.ptr));
}

/* Callback called by the library when call's media state has changed */
static void on_call_media_state(pjsua_call_id call_id) {
 pjsua_call_info ci;

 pjsua_call_get_info(call_id, &ci);

 if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
  // When media is active, connect call to sound device.
  pjsua_conf_connect(ci.conf_slot, 0);
  pjsua_conf_connect(0, ci.conf_slot);
 }
}

/* Display error and exit application */
static void error_exit(const char *title, pj_status_t status) {
 pjsua_perror(THIS_FILE, title, status);
 pjsua_destroy();
 exit(1);
}

/* Display registration state via LED */
static void led_registered(int value) {
 //printf("LED %d\n", value);
}

/*
 * main()
 *
 */
int main(int argc, char *argv[]) {
 int callbutton = 0;

 pjsua_acc_id acc_id;
 pjsua_acc_info acc_info;
 pj_status_t status;
 
 pjsua_acc_config acc_cfg;
 pjsua_call_setting call_setting;

 struct config configstruct;

 acc_cfg.vid_cap_dev = PJMEDIA_VID_DEFAULT_CAPTURE_DEV;
 acc_cfg.vid_out_auto_transmit = PJ_TRUE;
 call_setting.flag = PJSUA_CALL_INCLUDE_DISABLED_MEDIA;

 if (open_gpio() != 0) {
  return(1);
 }

 led_registered(0);

 get_config(FILENAME, &configstruct);

 /* Create pjsua first! */
 status = pjsua_create();
 if (status != PJ_SUCCESS) {
  error_exit("Error in pjsua_create()", status);
 }

 /* Init pjsua */
 {
  pjsua_config cfg;
  pjsua_logging_config log_cfg;
  pjsua_media_config media_cfg;

  pjsua_config_default(&cfg);
  cfg.cb.on_incoming_call = &on_incoming_call;
  cfg.cb.on_call_media_state = &on_call_media_state;
  cfg.cb.on_call_state = &on_call_state;

  pjsua_logging_config_default(&log_cfg);
  log_cfg.console_level = configstruct.loglevel; //4

  pjsua_media_config_default(&media_cfg);

  status = pjsua_init(&cfg, &log_cfg, &media_cfg);
  if (status != PJ_SUCCESS) {
   error_exit("Error in pjsua_init()", status);
  }
 }

 /* Add UDP transport. */
 {
  pjsua_transport_config cfg;

  pjsua_transport_config_default(&cfg);
  cfg.port = 5060;
  status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, NULL);
  if (status != PJ_SUCCESS) {
   error_exit("Error creating transport", status);
  }
 }

 /* Initialization is done, now start pjsua */
 status = pjsua_start();
 if (status != PJ_SUCCESS) {
  error_exit("Error starting pjsua", status);
 }

 /* Register to SIP server by creating SIP account. */
 {
  char id[1024],uri[1024];
  pjsua_acc_config cfg;

  pjsua_acc_config_default(&cfg);
  
  //cfg.id = pj_str("sip:" configstruct.user "@" configstruct.domain);
  memset(id, 0, sizeof(uri));
  sprintf(id, "sip:%s@%s",configstruct.user, configstruct.domain);
  cfg.id = pj_str(id);
  
  //cfg.reg_uri = pj_str("sip:" configstruct.domain);
  memset(uri, 0, sizeof(uri));
  sprintf(uri, "sip:%s", configstruct.domain);
  cfg.reg_uri = pj_str(uri);
  
  cfg.cred_count = 1;
  cfg.cred_info[0].realm = pj_str(configstruct.realm);
  cfg.cred_info[0].scheme = pj_str("digest");
  cfg.cred_info[0].username = pj_str(configstruct.user);
  cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
  cfg.cred_info[0].data = pj_str(configstruct.password);

  status = pjsua_acc_add(&cfg, PJ_TRUE, &acc_id);
  if (status != PJ_SUCCESS) {
   error_exit("Error adding account", status);
  }
 }

 /* Wait until user press "q" to quit. */
 for (;;) {
  pjsua_acc_get_info(acc_id, &acc_info);

  if (acc_info.status == 200) {
   led_registered(1);
  } else {
   led_registered(0);
  }

  if (read_gpio_pin(BUTTON1)==1) {
   if (callbutton != 1) {
    pjsua_call_hangup_all();
   
    pj_str_t uri = pj_str(configstruct.button1);
    status = pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, NULL);
    if (status != PJ_SUCCESS) {
     callbutton = 1;
    } else {
     callbutton = 0;
    }
   }
  }

  if (read_gpio_pin(BUTTON2)==1) {
   if (callbutton != 2) {
    pjsua_call_hangup_all();
   
    pj_str_t uri = pj_str(configstruct.button1);
    status = pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, NULL);
    if (status != PJ_SUCCESS) {
     callbutton = 2;
    } else {
     callbutton = 0;
    }
   }
  }
  
  if (read_gpio_pin(BUTTON3)==1) {
   if (callbutton != 3) {
    pjsua_call_hangup_all();
   
    pj_str_t uri = pj_str(configstruct.button1);
    status = pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, NULL);
    if (status != PJ_SUCCESS) {
     callbutton = 1;
    } else {
     callbutton = 0;
    }
   }
  }
  
  if (read_gpio_pin(BUTTON4)==1) {
   if (callbutton != 4) {
    pjsua_call_hangup_all();
   
    pj_str_t uri = pj_str(configstruct.button1);
    status = pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, NULL);
    if (status != PJ_SUCCESS) {
     callbutton = 4;
    } else {
     callbutton = 0;
    }
   }
  }

  usleep(250);
  
  if (pjsua_call_is_active(acc_id) != 0) {
   callbutton = 0;
  }
 }

 /* Destroy pjsua */
 pjsua_destroy();
 close_gpio();
 
 return (0);
}

