#ifndef _stwp_session_h_
#define _stwp_session_h_

/*权限组与功能列表
管理员权限：可查看所有信息、可执行的操作管理用户、管理可信验证模块的安装
操作员权限：可查看的有信息、可执行的操作创建策略、配置策略、管理主机
运维员权限：可查看所有信息、可执行的操作导出审计、查看系统审计信息
审计员权限：只能查看不能操作
系统管理员角色、审计管理员角色为系统内置角色（即：角色对应的权限为初始化产生，不可配置）；*/
#define STWP_PRIV_Admin             1
#define STWP_PRIV_Operation         2
#define STWP_PRIV_Maintenance       3
#define STWP_PRIV_Audit             4

typedef struct _PRIV_FUNC_LIST{
    int priv_type; //权限
    int func;      
}priv_func_list;


typedef struct _STWP_SESSION{
    char cur_user[65];
    int  sess_status;//0 未登录，1登录
    int  user_priv; //当前用的权限
    
} stwp_session;
void stwp_sessio_init(void);

int stwp_session_set(stwp_session* sess);
int stwp_session_setstatus( int sts);
int stwp_session_getstatus( );

int stwp_session_setuser(char * user);
char* stwp_session_getuser();

int stwp_session_setpriv(int priv);
int stwp_session_getpriv();

#endif