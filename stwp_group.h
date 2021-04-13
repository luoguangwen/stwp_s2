#ifndef _STWP_GROUP_H_
#define _STWP_GROUP_H_


//处理UI2发来的用户添加、删除、修改信息请求
//extern int doOpUser(cJSON *json,int socket_fd);

extern int doOpGroupAddNew(cJSON *json,int socket_fd);
extern int doOpGroupDelete(cJSON *json,int socket_fd);
extern int doOpGroupModify(cJSON *json,int socket_fd);
extern int doOpGroupAddHost(cJSON *json,int socket_fd);
extern int doOpGroupRemoveHost(cJSON *json,int socket_fd);


extern int doOpHostAddNew(cJSON *json,int socket_fd);
extern int doOpHostDelete(cJSON *json,int socket_fd);
extern int doOpHostModify(cJSON *json,int socket_fd);

#endif //_STWP_GROUP_H_