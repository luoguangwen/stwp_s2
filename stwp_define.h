#ifndef _STWP_DEFINE_H_
#define _STWP_DEFINE_H_



#define MINI_LENGTH 64
#define MID_LENGTH 128
#define PATH_LENGTH 256
#define MAX_LENGTH 1024

//'用户状态：0=未激活；1=已激活；99=已锁定；-1=已删除  (创建用户后未激活，修改密码激活)',
#define STWP_DB_USER_STS_UNACTIVE 0
#define STWP_DB_USER_STS_ACTIVE 1
#define STWP_DB_USER_STS_LOCKED 99
#define STWP_DB_USER_STS_DELETED -1


typedef struct STWP_USER_{
//TODO: permission ,privilege
    char uuid[MINI_LENGTH];
    char account[MINI_LENGTH];
    char password[MINI_LENGTH];
    char password_salt[MINI_LENGTH];
    char history_pwd[MINI_LENGTH];
    char history_pwd_salt[MINI_LENGTH];  
    char name[MINI_LENGTH];    
    char email[MID_LENGTH];
    char phone[24];
    char description[PATH_LENGTH];
    char permissions[MINI_LENGTH];
    char address[MAX_LENGTH];
    long long pwd_update_time;
    long long pwd_exp_time;
    long long  create_time;
    long long delete_time;
    int pwd_mat;//24小时内密码最大可尝试次数
    int pwd_rat;//24小时内密码剩余可尝试次数
    int status;
    int privilege;
}stwp_user;

#if 0
inline void stwp_inituser(stwp_user* stwpuser)
{
    memset(stwpuser->uuid,0,MINI_LENGTH);
    sprintf(stwpuser->uuid,"%s","");
    memset(stwpuser->account,0,MINI_LENGTH);
    sprintf(stwpuser->account,"%s","");
    memset(stwpuser->password,0,MINI_LENGTH);
    sprintf(stwpuser->password,"%s","");
    memset(stwpuser->password_salt,0,MINI_LENGTH);
    sprintf(stwpuser->password_salt,"%s","");
    memset(stwpuser->history_pwd,0,MINI_LENGTH);
    sprintf(stwpuser->history_pwd,"%s","");
    memset(stwpuser->history_pwd_salt,0,MINI_LENGTH);  
    sprintf(stwpuser->history_pwd_salt,"%s","");
    memset(stwpuser->name,0,MINI_LENGTH);    
    sprintf(stwpuser->name,"%s","");
    memset(stwpuser->email,0,MID_LENGTH);
    sprintf(stwpuser->email,"%s","");
    memset(stwpuser->phone,0,24);
    sprintf(stwpuser->phone,"%s","");
    memset(stwpuser->description,0,PATH_LENGTH);
    sprintf(stwpuser->description,"%s","");
    memset(stwpuser->permissions,0,MINI_LENGTH);
    sprintf(stwpuser->permissions,"%s","");
    memset(stwpuser->address,0,MAX_LENGTH);
    sprintf(stwpuser->address,"%s","");
    stwpuser->pwd_update_time = 0;
    stwpuser->pwd_exp_time=0;
    stwpuser->create_time=0;
    stwpuser->delete_time=0;
    stwpuser->pwd_mat=0;//24小时内密码最大可尝试次数
    stwpuser->pwd_rat=0;//24小时内密码剩余可尝试次数
    stwpuser->status=0;
}
#endif

//主机
typedef struct STWP_HOSTS_
{
    int     status;
    int     safe_mode;
    int     control_mode;
    char    uuid[MINI_LENGTH];
    char    name[MINI_LENGTH];
    char    ip[MINI_LENGTH];
    char    version[MINI_LENGTH];
    char    mac[MINI_LENGTH];
    char    create_time[MINI_LENGTH];
}stwp_host;

#endif