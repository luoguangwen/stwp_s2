#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "stwp_list.h"
#include "stwp_util.h"
#include "stwp_logdump.h"
#include "cJSON.h"
#include "stwp_p2.h"
#include "stwp_uievent.h"
#include "stwp_config.h"



extern volatile int stwp_uievent_stop_flag;

void show_usage()
{
    printf("--usage:\n\
-d :[run stwpd in background]\n\
-h :[ask for help]\n");
}

void stwp_receive_stop_sig(int signum)
{
    if (SIGUSR1 == signum || SIGUSR2 == signum)
    {
        /*task stop*/
  

        stwp_uievent_stop_flag = 0; 
        stwp_mysql_close();
    }
}

int main(int argc, char **argv)
{
    int opt, ndaemon = 0;
    printf( "****************************************************************\n");  
    printf( "**                                                            **\n");
    printf( "listen on port : %d \n",PORT);

    printf( "**       STWP Server2 Version 1.0.00-20210422                 **\n");
    printf( "****************************************************************\n");  
#if 0
    char pbuf[65]= {0x00};
    stwp_util_time_u2s(stwp_util_get_time(),pbuf);
    printf("result = %s\n",pbuf);
    return 0;
#endif
#if 0
    char  temp[64];
    int i = 0 ; 
    for(i = 0 ; i < 50 ;i++)
    {
        stwp_util_get_uuid(temp);
        printf("***new uuid %s \n",temp);
        memset(temp,0,sizeof(temp));
    }
   // return 0;
  
#endif 
#if 0
    #include "stwp_define.h"
   char sql[1024] = {0x00};
    stwp_user stwpuser;
    //stwp_inituser(&stwpuser);
    memset(&stwpuser,0,sizeof(stwp_user));
    printf("INSERT  INTO %s \
        (uuid,name,account,password,password_salt,\
        histroy_password,histroy_password_salt,create_time,phone,email,status,\
        address, description,pwd_update_time,pwd_expire_time,delete_time,\
        permissions,pwd_mat,pwd_rat) VALUES\
        (\
        \'%s\', \'%s\',\'%s\', \'%s\',\'%s\',\
        \'%s\',\'%s\',FROM_UNIXTIME(%lu),\'%s\',\'%s\',%d,\
        \'%s\',\'%s\',FROM_UNIXTIME(%lu),FROM_UNIXTIME(%lu),FROM_UNIXTIME(%lu),\
        \'%s\',\'%d\',%d)","stwp_users",\
        stwpuser.uuid,stwpuser.name,stwpuser.account,stwpuser.password,stwpuser.password_salt,\
        stwpuser.history_pwd,stwpuser.history_pwd_salt,stwpuser.create_time,stwpuser.phone,stwpuser.email,stwpuser.status,\
        stwpuser.address,stwpuser.description,stwpuser.pwd_update_time,stwpuser.pwd_exp_time,stwpuser.delete_time,\
        stwpuser.permissions,stwpuser.pwd_mat,stwpuser.pwd_rat);
    return 0;
#endif
#if 0
    char salt[MID_LENGTH]={0x00};
    char pwd[MID_LENGTH]={0x00};
    char encpwd[MID_LENGTH]={0x00};
    sprintf(pwd,"%s","testpwd123456");
    stwp_util_gen_salt(salt);
    stwp_util_get_encpwd(pwd,salt,encpwd);
    printf("PWD is    %s \n",pwd);
    printf("Salt is   %s \n",salt);
    printf("Encpwd is %s \n",encpwd);
    memset(encpwd,0,sizeof(encpwd));
    stwp_util_get_encpwd(pwd,salt,encpwd);
    printf("PWD is    %s \n",pwd);
    printf("Salt is   %s \n",salt);
    printf("Encpwd is %s \n",encpwd);
    memset(encpwd,0,sizeof(encpwd));
    stwp_util_get_encpwd(pwd,salt,encpwd);
    printf("PWD is    %s \n",pwd);
    printf("Salt is   %s \n",salt);
    printf("Encpwd is %s \n",encpwd);
    return 0;
#endif

#if 0
      printf("%s \n",stwp_util_get_tname_bytype(STWP_VALUE_TYPE_SelectAppList));
      printf("%s \n",stwp_util_get_tname_bytype(STWP_VALUE_TYPE_SelectBootList));
      printf("%s \n",stwp_util_get_tname_bytype(STWP_VALUE_TYPE_SelectConifgLis));
      printf("%s \n",stwp_util_get_tname_bytype(STWP_VALUE_TYPE_SelectDynamicList));
      printf("%s \n",stwp_util_get_tname_bytype(STWP_VALUE_TYPE_SelectPolicy));
#endif 
    // while((opt = getopt(argc, argv, "dh")) != -1){
    //     switch(opt){
    //         case 'd':
    //            ndaemon = 1; 
    //            break;
    //         case 'h':
    //            show_usage();
    //            return 0;
    //         default:
    //            break;
    //     } 
    // }

    // /*run in daemon*/
    // if(ndaemon)
    //     daemon(1, 0);

    // printf("mmmmmm\n");
    // /*run once*/
    // if(stwp_run_once())
    //     return 0;
    // /*close selinux*/
    // system("setenforce 0");
    // /*init signal*/
    // signal(SIGUSR1, stwp_receive_stop_sig);
    // signal(SIGUSR2, stwp_receive_stop_sig);

    /*init logdump*/
    stwp_logdump_module.init();
    stwp_logdump_module.run();

    
    stwp_mysql_init();

#if 0
    cJSON *json;
  
   // printf("Receive data from client [%d] :%s\n",socket_fd,json_from_ui);
    json = cJSON_Parse("{\
 \"type\":495,\
 \"user\":\"luo\",\
 \"host_uuid\":\"test-hostuid\",\
 \"data\":[\
   {\"object\":\"xxxx\",\"type\":1,\"op_type\":2},\
   {\"object\":\"xxxx\",\"type\":2,\"op_type\":3}\
     ]}");
     doOpWarningToPolicyTask(json,0);

#endif 

#if 0
    char outdata[10240]= {0x00};
    stwp_mysql_select("select * from stwp_users where uuid='11111111';","select count(*) from stwp_policies;",0,outdata);
    printf("tst:%s",outdata);
   // char sql[1024]={0x00};
   // sprintf( sql, "SELECT * from `stwp_whitelist_app_0867a44e-b481-4fde-9b3c-1767b60702b6` ;");

   // stwp_mysql_export2csv(sql, "./a.csv");
#endif
    /*init ui event*/
    stwp_uievent_module.init();
    stwp_uievent_module.evwait();

   

    

    return 0;
}
