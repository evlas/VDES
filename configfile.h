#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

#define FILENAME "entrydoor.conf"
#define MAXBUF 1024
#define DELIM "="
 
struct config
{
   char domain[MAXBUF];
   char user[MAXBUF];
   char password[MAXBUF];
   char realm[MAXBUF];
   int loglevel;
   char button1[MAXBUF];
   char button2[MAXBUF];
   char button3[MAXBUF];
   char button4[MAXBUF];
} _config;
 
void get_config(char *filename, struct config *conf);

#endif