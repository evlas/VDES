#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "configfile.h"

//#define SIP_DOMAIN "pbx.vmh.it"
//#define SIP_USER   "1031"
//#define SIP_PASSWD "daa8afc94a9ab21b0ac963e1f4074a23"
//#define SIP_REALM  "*"

void get_config(char *filename, struct config *conf) {
 struct config configstruct;
 FILE *file = fopen (filename, "r");
 
 memset(&configstruct, 0, sizeof(struct config));
 
 memcpy(configstruct.domain,"pbx.vmh.it",10);
 printf("%s\n",configstruct.domain);
 memcpy(configstruct.user,"1031",4);
 printf("%s\n",configstruct.user);
 memcpy(configstruct.password,"daa8afc94a9ab21b0ac963e1f4074a23",32);
 printf("%s\n",configstruct.password);
 memcpy(configstruct.realm,"*",1);
 printf("%s\n",configstruct.realm);

 configstruct.loglevel=4;
 printf("%d\n",configstruct.loglevel);

 memcpy(configstruct.button1,"600@pbx.vmh.it",1);
 printf("%s\n",configstruct.button1);

 memcpy(configstruct.button2,"601@pbx.vmh.it",1);
 printf("%s\n",configstruct.button2);

 memcpy(configstruct.button3,"602@pbx.vmh.it",1);
 printf("%s\n",configstruct.button3);

 memcpy(configstruct.button4,"603@pbx.vmh.it",1);
 printf("%s\n",configstruct.button4);

/* 
        if (file != NULL)
        {
                char line[MAXBUF];
                int i = 0;
 
                while(fgets(line, sizeof(line), file) != NULL)
                {
                        char *cfline;
                        cfline = strstr((char *)line,DELIM);
                        cfline = cfline + strlen(DELIM);
   
                        if (i == 0){
                                memcpy(configstruct.domain,cfline,strlen(cfline));
                                printf("%s",configstruct.domain);
                        } else if (i == 1){
                                memcpy(configstruct.user,cfline,strlen(cfline));
                                printf("%s",configstruct.user);
                        } else if (i == 2){
                                memcpy(configstruct.password,cfline,strlen(cfline));
                                printf("%s",configstruct.password);
                        } else if (i == 3){
                                memcpy(configstruct.realm,cfline,strlen(cfline));
                                printf("%s",configstruct.realm);
                        }
                        i++;
                } // End while
        } // End if file
*/
  fclose(file);
  
  memcpy(conf, &configstruct, sizeof(struct config));
}
 
