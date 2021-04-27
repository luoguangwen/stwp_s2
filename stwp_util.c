//#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <shadow.h>

#include "stwp_list.h"
#include "stwp_util.h"
#include "stwp_logdump.h"
#include "cJSON.h"
#include "stwp_p2.h"
#include "stwp_config.h"



int stwp_run_once()
{
    int ret = 0, fd, len, ipid;
    char spid[32] = {0};
    
    if(!access("/var/run/stwp.pid", F_OK))
        fd = open("/var/run/stwp.pid", O_RDWR, 0666);
    else
        fd = open("/var/run/stwp.pid", O_RDWR | O_CREAT, 0666);
    if(fd < 0){ 
        ret = -1; 
        return ret;
    }

    struct flock lock;
    memset(&lock, 0x0, sizeof(lock));
    if(fcntl(fd, F_GETLK, &lock) < 0){ 
        ret = -1; 
        return ret;
    }   

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;

    if(fcntl(fd, F_SETLK, &lock) < 0){ 
        ret = -1; 
        return -1; 
    }   

    pid_t pid = getpid();
    ipid = (int)pid;
    len = sprintf(spid, "%d\n", ipid);
    write(fd, spid, len);

    return 0;
}




/* UI2 JSON type字段对应需要操作的数据库表名称*/
struct STWP_TYPE_NAME g_type_name[]={
    {STWP_VALUE_TYPE_SelectAppList,             TABLE_APP_},
    {STWP_VALUE_TYPE_SelectBootList,            TABLE_BOOT_},
    {STWP_VALUE_TYPE_SelectConifgLis,           TABLE_CONFIG_},
    {STWP_VALUE_TYPE_SelectDynamicList,         TABLE_DYNAMIC_},
   
    {STWP_VALU_TYPE_CollectQuery,               TABLE_WARNING},
    {STWP_VALUE_TYPE_SelectHistory,             TABLE_WARNING},

    {STWP_VALUE_TYPE_SelectAWarning,            TABLE_WARNING_},
    {STWP_VALUE_TYPE_SelectAWarningCount,       TABLE_WARNING_},

    {STWP_VALUE_TYPE_SelectAudit,               TABLE_AUDIT_},
    {STWP_VALUE_TYPE_SelectAuditCount,          TABLE_AUDIT_},
    {STWP_VALUE_TYPE_SelectCenterAudit,         TABLE_CENTER},

	{STWP_VALUE_TYPE_SelectHost,                TABLE_HOSTS},	
    {STWP_VALUE_TYPE_SelectHostByUuid,          TABLE_HOSTS},
    {STWP_VALUE_TYPE_SetSecurityStatus,         TABLE_HOSTS},    
    {STWP_VALUE_TYPE_SelectCotrlMode,           TABLE_HOSTS},
    {STWP_VALUE_TYPE_SelectSecurityMode,        TABLE_HOSTS},
    {STWP_VALUE_TYPE_SelectALLHost,             TABLE_HOSTS},
    {STWP_VALUE_TYPE_SelectGroup,               TABLE_GROUP},
    {STWP_VALUE_TYPE_SelectNoGroupHost,           TABLE_HOSTS},

    {STWP_VALUE_TYPE_SelectScrollInfo,          TABLE_SCROLL},

    {STWP_VALUE_TYPE_SelectPolicyTask,          TABLE_TASK},
    {STWP_VALUE_TYPE_SelectHostPolicy,          TABLE_TASK},
    {STWP_VALUE_TYPE_ConfigPolicy,              TABLE_TASK},
    {STWP_VALUE_TYPE_DeletePolicyTask,          TABLE_TASK},
 

    {STWP_VALUE_TYPE_SelectPolicy,              TABLE_POLICIES},    
    {STWP_VALUE_TYPE_AddPolicy,                 TABLE_POLICIES},
    {STWP_VALUE_TYPE_DelectPolicy,              TABLE_POLICIES},
    {STWP_VALUE_TYPE_ModifyPolicy,              TABLE_POLICIES},


    {STWP_VALUE_TYPE_AddUser,                   TABLE_USER},
    {STWP_VALUE_TYPE_DeleteUser,                TABLE_USER},
    {STWP_VALUE_TYPE_ModifyUser,                TABLE_USER},
    {STWP_VALUE_TYPE_SelectUser,                TABLE_USER},

    {STWP_VALUE_TYPE_ExportAppList,             TABLE_APP_},
    {STWP_VALUE_TYPE_ExportBootList,            TABLE_BOOT_},
    {STWP_VALUE_TYPE_ExportConifgList,          TABLE_CONFIG_},
    {STWP_VALUE_TYPE_ExportDynamicList,         TABLE_DYNAMIC_},
    {STWP_VALUE_TYPE_ExportHostAudit,           TABLE_AUDIT_},
    {STWP_VALUE_TYPE_ExportCenterAudit,         TABLE_CENTER},
    {STWP_VALUE_TYPE_ExportWarning,             TABLE_WARNING_},
    {0,""}
};
char* stwp_util_get_tname_bytype( int nType)
{
    int i = 0 ;
    for( i = 0 ; ; i++)
    {
        if(strlen(g_type_name[i].name) == 0)
            return "";
        if( nType == g_type_name[i].type)
            return g_type_name[i].name;
        
    }
    return "";
}

//将Uinix time stamp 转换成字符串格式化
// Unix time stamp from 1970 is 1615125986 
//Format : 2021-03-07 06:06:26

int stwp_util_time_u2s(long long t,char str[64])
{
   
	struct tm *tblock;
    time_t tmp = t;
	tblock = localtime(&tmp);

    printf( "input  time stampis %d \n",t);
    strftime(str,64,"%Y-%m-%d %H:%M:%S", tblock);
    printf( "output format is  %s\n",str);
    return 0;

}
//TODO: time格式统一或转换
long long stwp_util_get_time()
{
    //time_t h;
    //time(&h);//计算从1970到现在的秒数
  
    return time(0);
}
#include <uuid/uuid.h>
int stwp_util_get_uuid(char buf[64])
{ 
    uuid_t uu[4];
    char uuid[256]={0x00};
    uuid_generate_time_safe(uu[0]);
    uuid_unparse(uu[0],buf);
    return 0;
    stwp_util_gen_salt(buf);
    return 0;

    const char *c = "89ab";
    char *p = buf;
    int n;  
    for( n = 0; n < 16; ++n )
    {
        int b = rand()%255;  
        switch( n )
        {
            case 6:
                sprintf(
                    p,
                    "4%x",
                    b%15 );
                break;
            case 8:
                sprintf(
                    p,
                    "%c%x",
                    c[rand()%strlen( c )],
                    b%15 );
                break;
            default:
                sprintf(
                    p,
                    "%02x",
                    b );
                break;
        }
  
        p += 2;
  
        switch( n )
        {
            case 3:
            case 5:
            case 7:
            case 9:
                *p++ = '-';
                break;
        }
    }
  
    *p = 0;
  
    return 0;
}

int stwp_util_gen_salt(char* salt)
{
    int nonce=0;
    char temp[10] = {0x00};
    
    srandom((unsigned int)time(0)); 
    nonce =  random();
    memset(temp,0,sizeof(temp));
    sprintf(temp,"%08X",nonce);
    strcat(salt,temp);

    srandom((unsigned int)time(0)+1 ); 
    nonce =  random();
     memset(temp,0,sizeof(temp));
    sprintf(temp,"%08X",nonce);
    strcat(salt,temp);

    srandom((unsigned int)time(0)+2); 
    nonce =  random();
     memset(temp,0,sizeof(temp));
    sprintf(temp,"%08X",nonce);
    strcat(salt,temp);

    srandom((unsigned int)time(0)+4); 
    nonce =  random();
     memset(temp,0,sizeof(temp));
    sprintf(temp,"%08X",nonce);
    strcat(salt,temp);

    return 0;
}
int stwp_util_get_encpwd(char* pwd,char salt[16],char* enc_pwd)
{
    char temp_pwd[MID_LENGTH] = {0x00};
    unsigned char temp_digest[16] = {0x00};
    char temp[10] = {0x00};
    int i = 0;
    //TODO:注意Length
    sprintf(temp_pwd,"%s%s",pwd,salt);
    MDString(temp_pwd,temp_digest);
    for(i=0;i<16;i++)
    {
        memset(temp,0,sizeof(temp));
        sprintf(temp,"%02X",temp_digest[i]);
        strcat(enc_pwd,temp);
        
    }
    return 0;
}

int stwp_util_coreDump()
{
	#define CORE_SIZE   1024 * 1024 * 500
    struct rlimit rlmt;
    if (getrlimit(RLIMIT_CORE, &rlmt) == -1) {
        return -1;
    }
    printf("Before set rlimit CORE dump current is:%d, max is:%d\n",
           (int) rlmt.rlim_cur, (int) rlmt.rlim_max);

    rlmt.rlim_cur = (rlim_t) CORE_SIZE;
    rlmt.rlim_max = (rlim_t) CORE_SIZE;

    if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
        return -1;
    }

    if (getrlimit(RLIMIT_CORE, &rlmt) == -1) {
        return -1;
    }
    printf("After set rlimit CORE dump current is:%d, max is:%d\n",
           (int) rlmt.rlim_cur, (int) rlmt.rlim_max);
    return 0;

}