#ifndef _STWP_USER_H_
#define _STWP_USER_H_


//处理UI2发来的用户添加、删除、修改信息请求
//extern int doOpUser(cJSON *json,int socket_fd);

extern int doOpUserAddNew(cJSON *json,int socket_fd);
extern int doOpUserDelete(cJSON *json,int socket_fd);
extern int doOpUserModify(cJSON *json,int socket_fd);
extern int doOpUserCheckLogin(cJSON *json,int socket_fd);
extern int doOpUserChangePWD(cJSON *json,int socket_fd);

extern int getUserPrivilege(char * user_account);
#endif //_STWP_USER_H_