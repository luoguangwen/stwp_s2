#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "stwp_list.h"
#include "stwp_util.h"
#include "stwp_uievent.h"
#include "stwp_logdump.h"
#include "stwp_config.h"
#include "cJSON.h"
#include "stwp_p2.h"

#include "stwp_group.h"
#include "stwp_define.h"




int doOpGroupAddNew(cJSON *json,int socket_fd)
{
/*
"{
""type"":this , //必选
""name"": """"，//必选
""info"":"""",
}"
*/   
    int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;

    int type =0; 
    char name[64]={0x00};
    char info[512]={0x00};
    char uuid[64] = {0x00};
    char sql[1024]={0x00};
    char outdata[MAX_LENGTH] = {0};
    
#if 1  //按协议定义，取字段。
    //当前用户
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    
    cJSON *puser = cJSON_GetObjectItem(json, "name");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(name, puser->valuestring);
    }
    else {
        goto error;
    }
    cJSON *ppwd = cJSON_GetObjectItem(json, "info");
    if(ppwd && (ppwd->type ==cJSON_String)){
        strcpy(info, ppwd->valuestring);
    }
    else{
        goto error;
    } 

#endif
#if 1

    if( type == STWP_VALUE_TYPE_AddGroup)
    {
        
        //    write_ui_audit(STWP_AUDIT_TYPE_CreateUser,c_user," ","create user");
        // 查询是该组名是否存在
        sprintf(sql,"select group_name from stwp_group where group_name = '%s'",name);
        if(0!= stwp_mysql_select_cnt(sql) )
        {
            errcode = STWP_VALUE_TYPE_Existed;  
            goto error;
        }
        
        memset(sql,0,sizeof(sql));
        stwp_util_get_uuid(uuid);
        sprintf( sql, "INSERT INTO stwp_group (group_name,group_uuid,remark) VALUES\
        (\'%s\','%s','%s')",\
        name,uuid,info);
        
        stwp_mysql_write(sql, type, outdata);
        printf("-----------------------------------------------\n");
        printf("sql    = %s \n", sql); 
        printf("result = %s \n",outdata);
        errcode = STWP_VALUE_TYPE_ErrSucceed;            
    }
    else
    {
        errcode = STWP_VALUE_TYPE_ErrData;
        goto error;
    }
#endif 
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return 0;
error:
   stwp_data_error_for_ui2(type,socket_fd,errcode);
    return -1;
}

int doOpGroupModify(cJSON *json,int socket_fd)
{
/*
{
"type":this , //必选
"uuid":  //必选
"name": "name"，//
"info":""//
}
*/   
    int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;

    int type =0; 
    char name[64]={0x00};
    char info[512]={0x00};
    char uuid[64] = {0x00};
    char sql[1024]={0x00};
    char outdata[MAX_LENGTH] = {0};
    
#if 1  //按协议定义，取字段。
    //当前用户
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }

    cJSON *puuid = cJSON_GetObjectItem(json, "uuid");
    if(puuid && (puuid->type ==cJSON_String)){
        strcpy(uuid, puuid->valuestring);
    }
    else{
        goto error;
    } 
    
    cJSON *puser = cJSON_GetObjectItem(json, "name");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(name, puser->valuestring);
    }
    else {
        goto error;
    }
    cJSON *ppwd = cJSON_GetObjectItem(json, "info");
    if(ppwd && (ppwd->type ==cJSON_String)){
        strcpy(info, ppwd->valuestring);
    }
    else{
        goto error;
    } 

#endif
#if 1

    if( type == STWP_VALUE_TYPE_ModifyGroup)
    {
        
        //    write_ui_audit(STWP_AUDIT_TYPE_CreateUser,c_user," ","create user");
        // 查询是该组名是否存在
        sprintf(sql,"select group_name from stwp_group where group_uuid = '%s'",uuid);
        if(0!= stwp_mysql_select_cnt(sql) )
        {
            errcode = STWP_VALUE_TYPE_Existed;  
            goto error;
        }
        
        memset(sql,0,sizeof(sql));
      //  stwp_util_get_uuid(uuid);
        sprintf( sql, "update stwp_group set group_name='%s',remark ='%s' where group_uuid='%s'", name,info,uuid);
        
        stwp_mysql_write(sql, type, outdata);
        printf("-----------------------------------------------\n");
        printf("sql    = %s \n", sql); 
        printf("result = %s \n",outdata);
        errcode = STWP_VALUE_TYPE_ErrSucceed;            
    }
    else
    {
        errcode = STWP_VALUE_TYPE_ErrData;
        goto error;
    }
#endif 
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return 0;
error:
   stwp_data_error_for_ui2(type,socket_fd,errcode);
    return -1;
}

int  doOpGroupDelete(cJSON *json,int socket_fd)
{
/*
{
"type":this , //必选
 “uuid”:["","",""],，//可选

}
*/   
    int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;
    int type =0; 
    char sql[1024]={0x00};
    char outdata[MAX_LENGTH] = {0};
    
#if 1  //按协议定义，取字段。
    //当前用户
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
  #endif
#if 1

    if( type == STWP_VALUE_TYPE_DelGroup)
    {
        
         cJSON *task_arry     = cJSON_GetObjectItem( json, "uuid");
        if( task_arry != NULL)
        {
            int  task_size   = cJSON_GetArraySize ( task_arry );
            int i = 0;
            for( i = 0 ; i < task_size ; i ++ ){
                cJSON * pTaskSub = cJSON_GetArrayItem(task_arry, i);
                if(NULL == pTaskSub ){ continue ; }

                memset(sql,0,sizeof(sql));
                sprintf( sql, "DELETE FROM  stwp_group where group_uuid = '%s'",pTaskSub->valuestring);
                memset(outdata,0,sizeof(outdata)); 
                stwp_mysql_write(sql, type, outdata);
                printf("-----------------------------------------------\n");
                printf("sql    = %s \n", sql); 
                printf("result = %s \n",outdata);  
                
            //    write_ui_audit(STWP_AUDIT_TYPE_DeleteTask,user,pTaskSub->valuestring,"delete policy task");
            
            }
            errcode = STWP_VALUE_TYPE_ErrSucceed;  
         }
        else
            goto error;
    }
    else
    {
        errcode = STWP_VALUE_TYPE_ErrData;
        goto error;
    }
#endif 
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return 0;
error:
   stwp_data_error_for_ui2(type,socket_fd,errcode);
    return -1;
}

//批量将主机添回到某个组里
int doOpGroupAddHost(cJSON *json,int socket_fd)
{
/*
{
"type":this , //必选
"uuid":
 "host_uuid":["","",""],，//可选

}

*/   
   int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;
    int type =0; 
     char uuid[64] = {0x00};
    char sql[1024]={0x00};
    char outdata[MAX_LENGTH] = {0};
    
#if 1  //按协议定义，取字段。
    //当前用户
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    cJSON *puuid = cJSON_GetObjectItem(json, "uuid");
    if(puuid && (puuid->type ==cJSON_String)){
        strcpy(uuid, puuid->valuestring);
    }
    else{
        goto error;
    } 
  #endif
#if 1

    if( type == STWP_VALUE_TYPE_AddHostToGroup)
    {
        
         cJSON *task_arry     = cJSON_GetObjectItem( json, "host_uuid");
        if( task_arry != NULL)
        {
            int  task_size   = cJSON_GetArraySize ( task_arry );
            int i = 0;
            for( i = 0 ; i < task_size ; i ++ ){
                cJSON * pTaskSub = cJSON_GetArrayItem(task_arry, i);
                if(NULL == pTaskSub ){ continue ; }

                memset(sql,0,sizeof(sql));
                sprintf( sql, "update stwp_hosts set group_uuid ='%s' where uuid = '%s'",uuid,pTaskSub->valuestring);
                memset(outdata,0,sizeof(outdata)); 
                stwp_mysql_write(sql, type, outdata);
                printf("-----------------------------------------------\n");
                printf("sql    = %s \n", sql); 
                printf("result = %s \n",outdata);  
                
            //    write_ui_audit(STWP_AUDIT_TYPE_DeleteTask,user,pTaskSub->valuestring,"delete policy task");
            
            }
            errcode = STWP_VALUE_TYPE_ErrSucceed;  
         }
        else
            goto error;
    }
    else
    {
        errcode = STWP_VALUE_TYPE_ErrData;
        goto error;
    }
#endif 
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return 0;
error:
   stwp_data_error_for_ui2(type,socket_fd,errcode);
    return -1;
}

//批量将主机从某个组删除
int doOpGroupRemoveHost(cJSON *json,int socket_fd)
{
/*
{
"type":this , //必选
"uuid":
 "host_uuid":["","",""],，//可选

}

*/   
   int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;
    int type =0; 
     char uuid[64] = {0x00};
    char sql[1024]={0x00};
    char outdata[MAX_LENGTH] = {0};
    
#if 1  //按协议定义，取字段。
    //当前用户
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    cJSON *puuid = cJSON_GetObjectItem(json, "uuid");
    if(puuid && (puuid->type ==cJSON_String)){
        strcpy(uuid, puuid->valuestring);
    }
    else{
        goto error;
    } 
  #endif
#if 1

    if( type == STWP_VALUE_TYPE_RemoveHostFromGroup)
    {
        
         cJSON *task_arry     = cJSON_GetObjectItem( json, "host_uuid");
        if( task_arry != NULL)
        {
            int  task_size   = cJSON_GetArraySize ( task_arry );
            int i = 0;
            for( i = 0 ; i < task_size ; i ++ ){
                cJSON * pTaskSub = cJSON_GetArrayItem(task_arry, i);
                if(NULL == pTaskSub ){ continue ; }

                memset(sql,0,sizeof(sql));
                sprintf( sql, "update stwp_hosts set group_uuid ='' where uuid = '%s'",pTaskSub->valuestring);
                memset(outdata,0,sizeof(outdata)); 
                stwp_mysql_write(sql, type, outdata);
                printf("-----------------------------------------------\n");
                printf("sql    = %s \n", sql); 
                printf("result = %s \n",outdata);  
                
            //    write_ui_audit(STWP_AUDIT_TYPE_DeleteTask,user,pTaskSub->valuestring,"delete policy task");
            
            }
            errcode = STWP_VALUE_TYPE_ErrSucceed;  
         }
        else
            goto error;
    }
    else
    {
        errcode = STWP_VALUE_TYPE_ErrData;
        goto error;
    }
#endif 
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return 0;
error:
   stwp_data_error_for_ui2(type,socket_fd,errcode);
    return -1;
}

int doOpHostAddNew(cJSON *json,int socket_fd)
{
/*
{
"type":this , //必选
"name": "name"，//可选
"ip":“xxx”，//
"group_uuid":""//
}
*/   
    int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;

    int type =0; 
    char name[64]={0x00};
    char ip[512]={0x00};
    char uuid[64] = {0x00};
    char guuid[64] = {0x00};
    char sql[1024]={0x00};
    char outdata[MAX_LENGTH] = {0};
    
#if 1  //按协议定义，取字段。
    //当前用户
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    
    cJSON *puser = cJSON_GetObjectItem(json, "name");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(name, puser->valuestring);
    }

    cJSON *ppwd = cJSON_GetObjectItem(json, "ip");
    if(ppwd && (ppwd->type ==cJSON_String)){
        strcpy(ip, ppwd->valuestring);
    }
    else{
        goto error;
    } 
     cJSON *pguuid = cJSON_GetObjectItem(json, "group_uuid");
    if(pguuid && (pguuid->type ==cJSON_String)){
        strcpy(guuid, pguuid->valuestring);
    }
    else{
        goto error;
    } 
#endif
#if 1

    if( type == STWP_VALUE_TYPE_AddHost)
    {
        
        //    write_ui_audit(STWP_AUDIT_TYPE_CreateUser,c_user," ","create user");
        // 查询是该组名是否存在
        sprintf(sql,"select name from stwp_hosts where ip = '%s'",ip);
        if(0!= stwp_mysql_select_cnt(sql) )
        {
            errcode = STWP_VALUE_TYPE_Existed;  
            goto error;
        }
        
        memset(sql,0,sizeof(sql));
        stwp_util_get_uuid(uuid);
        sprintf( sql, "INSERT INTO stwp_hosts (uuid,name,ip,group_uuid) VALUES\
        (\'%s\',\'%s\','%s','%s')",\
        uuid,name,ip,guuid);
        
        stwp_mysql_write(sql, type, outdata);
        printf("-----------------------------------------------\n");
        printf("sql    = %s \n", sql); 
        printf("result = %s \n",outdata);
        errcode = STWP_VALUE_TYPE_ErrSucceed;            
    }
    else
    {
        errcode = STWP_VALUE_TYPE_ErrData;
        goto error;
    }
#endif 
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return 0;
error:
   stwp_data_error_for_ui2(type,socket_fd,errcode);
    return -1;
}
int doOpHostDelete(cJSON *json,int socket_fd)
{
    /*
{
"type":this , //必选
 “uuid”:["","",""],，//可选

}
*/   
    int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;
    int type =0; 
    char sql[1024]={0x00};
    char outdata[MAX_LENGTH] = {0};
    
#if 1  //按协议定义，取字段。
    //当前用户
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
  #endif
#if 1

    if( type == STWP_VALUE_TYPE_DeletHost)
    {
        
         cJSON *task_arry     = cJSON_GetObjectItem( json, "uuid");
        if( task_arry != NULL)
        {
            int  task_size   = cJSON_GetArraySize ( task_arry );
            int i = 0;
            for( i = 0 ; i < task_size ; i ++ ){
                cJSON * pTaskSub = cJSON_GetArrayItem(task_arry, i);
                if(NULL == pTaskSub ){ continue ; }

                memset(sql,0,sizeof(sql));
                sprintf( sql, "DELETE FROM  stwp_hosts where uuid = '%s'",pTaskSub->valuestring);
                memset(outdata,0,sizeof(outdata)); 
                stwp_mysql_write(sql, type, outdata);
                printf("-----------------------------------------------\n");
                printf("sql    = %s \n", sql); 
                printf("result = %s \n",outdata);  
                
            //    write_ui_audit(STWP_AUDIT_TYPE_DeleteTask,user,pTaskSub->valuestring,"delete policy task");
            
            }
            errcode = STWP_VALUE_TYPE_ErrSucceed;  
         }
        else
            goto error;
    }
    else
    {
        errcode = STWP_VALUE_TYPE_ErrData;
        goto error;
    }
#endif 
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return 0;
error:
   stwp_data_error_for_ui2(type,socket_fd,errcode);
    return -1;
}
int doOpHostModify(cJSON *json,int socket_fd)
{
/*
{
"type":this , //必选
"uuid":  //必选
"name": "name"，//
"info":""//
}
*/   
    int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;

    int type =0; 
    char name[64]={0x00};
    char guuid[512]={0x00};
    char uuid[64] = {0x00};
    char sql[1024]={0x00};
    char outdata[MAX_LENGTH] = {0};
    
#if 1  //按协议定义，取字段。
    //当前用户
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }

    cJSON *puuid = cJSON_GetObjectItem(json, "uuid");
    if(puuid && (puuid->type ==cJSON_String)){
        strcpy(uuid, puuid->valuestring);
    }
    else{
        goto error;
    } 
    
    cJSON *puser = cJSON_GetObjectItem(json, "name");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(name, puser->valuestring);
    }
    else {
        goto error;
    }
    cJSON *ppwd = cJSON_GetObjectItem(json, "group_uuid");
    if(ppwd && (ppwd->type ==cJSON_String)){
        strcpy(guuid, ppwd->valuestring);
    }
    else{
        goto error;
    } 

#endif
#if 1

    if( type == STWP_VALUE_TYPE_ModifyHost)
    {
        
        //    write_ui_audit(STWP_AUDIT_TYPE_CreateUser,c_user," ","create user");
        // 查询是该组名是否存在
        sprintf(sql,"select group_name from stwp_group where group_uuid = '%s'",uuid);
        if(0!= stwp_mysql_select_cnt(sql) )
        {
            errcode = STWP_VALUE_TYPE_Existed;  
            goto error;
        }
        
        memset(sql,0,sizeof(sql));
       // stwp_util_get_uuid(uuid);
        sprintf( sql, "update stwp_hosts set group_uuid='%s',name ='%s' where uuid='%s'", guuid,name,uuid);
        
        stwp_mysql_write(sql, type, outdata);
        printf("-----------------------------------------------\n");
        printf("sql    = %s \n", sql); 
        printf("result = %s \n",outdata);
        errcode = STWP_VALUE_TYPE_ErrSucceed;            
    }
    else
    {
        errcode = STWP_VALUE_TYPE_ErrData;
        goto error;
    }
#endif 
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return 0;
error:
   stwp_data_error_for_ui2(type,socket_fd,errcode);
    return -1;
}
