#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

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
//#include "stwp_util.h"
#include "stwp_uievent.h"
#include "stwp_logdump.h"
#include "stwp_config.h"

#include "cJSON.h"
#include "stwp_md5.h"
#include "stwp_user.h"
#include "stwp_util.h"
#include "stwp_p2.h"
#include "stwp_group.h"


#define BACKLOG 1
static stwp_ui2_node_t event_ui2;
//处理UI2发来的查询类请求
int getUserUUID( char * name,char* uuid)
{//TODO :通过多表联查获取并使用
    sprintf(uuid,"%s","2112-22-44444-66666");
    return 0;
}

/**
//记录UI的各类操作审计
 stwp_audit_center 
  `type` '审计类型 1：中心操作',
  `contents` varchar(1024) '备注',
  `op_user` varchar(64)'审计操作的用户 account',
  `op_time` varchar(64)'审计操作时间',
  `op_object` varchar(64) '审计操作的对象',
  `op_type` int(11)  '审计操作类型'
op_type 取值定义参见：STWP_AUDIT_TYPE_xxxx
*/
int write_ui_audit(int op_type,char* op_user,char* op_object,char* contents)
{
    char sql[MAX_LENGTH] = {0};
    char outdata[MAX_LENGTH] = {0};

    if( !op_user || !op_object || !contents)
        return -1;    
    
    sprintf( sql, "INSERT INTO stwp_audit_center (type,op_type,op_object,op_user,op_time,contents)\
                    VALUES(%d,%d,\'%s\',\'%s\', FROM_UNIXTIME(%lu),\'%s\')",\
                    1,op_type,op_object,op_user,stwp_util_get_time(), contents);

    stwp_mysql_write(sql,0,outdata);
    return 0;
}

int stwp_data_error_for_ui(int type,int socket_fd)
{
    char *outdata;
    cJSON *json;
    int err_code = -1;
    int ret = 0;
    json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "type", type);
    cJSON_AddNumberToObject(json, "errcode", -1);
    cJSON_AddNumberToObject(json, "count", 0);
    outdata=cJSON_Print(json);
    //TODO: Length + data
    int nlen = strlen(outdata);
    send(socket_fd,&nlen,sizeof(int),0);
    ret = send(socket_fd,outdata,nlen,0);
    free(outdata);
    cJSON_Delete(json);
    
    return 0;
}

//给UI2回应应答，带错误码
int stwp_data_error_for_ui2(int type,int socket_fd,int errcode)
{
    char *outdata;
    cJSON *json;
   
    int ret = 0;
    json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "type", type);
    cJSON_AddNumberToObject(json, "errcode", errcode);
    cJSON_AddNumberToObject(json, "count", 0);
    outdata=cJSON_Print(json);
    int nlen = strlen(outdata);
    send(socket_fd,&nlen,sizeof(int),0);
    printf("send back :%s \n",outdata);
    ret = send(socket_fd,outdata,nlen,0);
    free(outdata);
    cJSON_Delete(json);
    
    return 0;
}

int doOpQuery(cJSON *json, int socket_fd)
{
    stwp_p2_query stwp_ui2_json_recv;
    char sql[MAX_LENGTH] = {0};
    char sql2[MAX_LENGTH] = {0x00};
    char temp[MAX_LENGTH] = {0};
    char outdata[MAX_LENGTH * 20] = {0};
    int num = 0;
    int page_num = 15;
    int ret = 0;
    int nstart=0xFF,nend=0xFF; 

    memset(&stwp_ui2_json_recv,0,sizeof(stwp_p2_query));
    //按QUERY类JSON协议定义，取字段。
    #if 1
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype){
        stwp_ui2_json_recv.type = ptype->valueint;
        //printf("type = %d\n",stwp_ui2_json_recv.type);
    }
    else{
        goto error;
    }
    cJSON *ppage = cJSON_GetObjectItem(json, "currentPage");
    if(ppage){
        stwp_ui2_json_recv.page= ppage->valueint;
        //printf("currentPage = %d\n",stwp_ui2_json_recv.page);
    }
    else{
        goto error;
    }

    cJSON *uuid_or_ip = cJSON_GetObjectItem(json, "uuid");
    if(uuid_or_ip){
        strcpy(stwp_ui2_json_recv.uuid, uuid_or_ip->valuestring);
    }
  

    cJSON *pkey = cJSON_GetObjectItem(json, "keystr");
    if(pkey){
        strcpy(stwp_ui2_json_recv.keystr, pkey->valuestring);
    }

    cJSON *pStart = cJSON_GetObjectItem(json, "start");
    if(pStart&& pStart->type == cJSON_Number){
        nstart = pStart->valueint;       
    }

    cJSON *pend = cJSON_GetObjectItem(json, "end");
    if(pend&& pend->type == cJSON_Number){
       nend = pend->valueint;
    }
  
  
    #endif
    //组织SQL，查表，发送给UI
    if(stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectAppList || 
        stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectBootList||
        stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectConifgLis||
        stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectDynamicList)
    {//表名带UUID后辍
        sprintf(sql2,"SELECT COUNT(*) from  `%s%s`  ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid);
        sprintf(sql,"select * from `%s%s` ", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid);
        if( (nstart !=0xFF && nend != 0xFF) || strlen(stwp_ui2_json_recv.keystr)  ){
            strcat( sql2,"where 1 ");
            strcat(sql,"where 1 ");
             if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ," AND ((i_modtime BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (i_modtime BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
                strcat(sql2,temp);
                strcat(sql, temp);
            }    
            if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ,"AND ( s_pathname like '%%%s%%' ) ",
                stwp_ui2_json_recv.keystr);
                strcat(sql2,temp);
                strcat(sql,temp);

            }
        }
        strcat(sql2,";");
        memset(temp,0,sizeof(temp));
        sprintf( temp ," order by s_uuid desc limit %d,%d; ",(stwp_ui2_json_recv.page - 1) * page_num,  page_num);
        strcat(sql,temp);
 
     //  sprintf(sql2,"SELECT COUNT(*) from  `%s%s` ; ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid);
     //  sprintf(sql,"select * from `%s%s` order by s_uuid desc limit %d,%d;",  stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid, (stwp_ui2_json_recv.page - 1) * page_num, page_num);
     }
    else if(stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectGroup)
    {
        sprintf(sql2,"SELECT COUNT(*) from  `%s`  ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
        sprintf(sql,"select * from `%s` ", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
        if( (nstart !=0xFF && nend != 0xFF) || strlen(stwp_ui2_json_recv.keystr)  ){
            strcat( sql2,"where 1 ");
            strcat(sql,"where 1 ");
            /*if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ," AND ((create_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (create_time BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
                strcat(sql2,temp);
                strcat(sql, temp);
            } */   
            if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ,"AND ( group_name like '%%%s%%' or remark like '%%%s%%' ) ",
                stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                strcat(sql2,temp);
                strcat(sql,temp);

            }
        }

        strcat(sql2,";");
        if( 0 == stwp_ui2_json_recv.page)
        {//查询全部记录
            strcat(sql," order by group_uuid desc ; ");
        }
        else{//按分页查询
            memset(temp,0,sizeof(temp));
            sprintf( temp ," order by group_uuid desc limit %d,%d; ",(stwp_ui2_json_recv.page - 1) * page_num, page_num);
            strcat(sql,temp);
        }
       
    }
    else if(stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectAudit)
    {
        sprintf(sql2,"SELECT COUNT(*) from  `%s%s`  ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid);
        sprintf(sql,"select * from `%s%s` ", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid);
        if( (nstart !=0xFF && nend != 0xFF) || strlen(stwp_ui2_json_recv.keystr)  ){
            strcat( sql2,"where 1 ");
            strcat(sql,"where 1 ");
             if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ," AND ((op_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (op_time BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
                strcat(sql2,temp);
                strcat(sql, temp);
            }    
            if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ,"AND (contents like '%%%s%%' or op_object like '%%%s%%') ",
                    stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                strcat(sql2,temp);
                strcat(sql,temp);

            }
        }
        strcat(sql2,";");
        memset(temp,0,sizeof(temp));
        sprintf( temp ," order by op_time desc limit %d,%d; ",(stwp_ui2_json_recv.page - 1) * page_num,page_num);
        strcat(sql,temp);
      // sprintf(sql2,"SELECT COUNT(*) from  `%s%s` ; ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid);
      // sprintf(sql,"select * from `%s%s` order by op_time desc limit %d,%d;",  stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid, (stwp_ui2_json_recv.page - 1) * page_num, page_num);
     }
    else if(stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectCenterAudit)
    {//查管理中心的审计信息
        sprintf(sql2,"SELECT COUNT(*) from  `%s`  ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
        sprintf(sql,"select * from `%s` ", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
        if( (nstart !=0xFF && nend != 0xFF) || strlen(stwp_ui2_json_recv.keystr)  ){
            strcat( sql2,"where 1 ");
            strcat(sql,"where 1 ");
             if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ," AND ((op_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (op_time BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
                strcat(sql2,temp);
                strcat(sql, temp);
            }    
            if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ,"AND ( contents like '%%%s%%' or op_object like '%%%s%%') ",
                stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                strcat(sql2,temp);
                strcat(sql,temp);

            }
        }
        strcat(sql2,";");
        memset(temp,0,sizeof(temp));
        sprintf( temp ," order by op_time desc limit %d,%d; ",(stwp_ui2_json_recv.page - 1) * page_num,  page_num);
        strcat(sql,temp);

       // sprintf(sql2,"SELECT COUNT(*) from  `%s` ; ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
       // sprintf(sql,"select * from `%s`  order by op_time DESC limit %d,%d;",  stwp_util_get_tname_bytype(stwp_ui2_json_recv.type),  (stwp_ui2_json_recv.page - 1) * page_num, page_num);
    }
    else if(stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectPolicy)
    {//表名不带UUID后辍
        sprintf(sql2,"select COUNT(*) from stwp_policies as p inner join stwp_users as u on p.create_user_uuid = u.uuid  ");
        sprintf(sql,"select p.uuid as uuid,p.name as name,p.type as type,p.contents as contents, u.account as create_user_uuid,\
        p.create_time as create_time,p.status as status from stwp_policies as p \
        inner join stwp_users as u on p.create_user_uuid = u.uuid  ");
        if( (nstart !=0xFF && nend != 0xFF) || strlen(stwp_ui2_json_recv.keystr)  ){
            strcat( sql2,"where 1 ");
            strcat(sql,"where 1 ");
             if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ," AND ((create_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (create_time BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
                strcat(sql2,temp);
                strcat(sql, temp);
            }    
            if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ,"AND ( p.contents like '%%%s%%' or p.name like '%%%s%%'or u.account like '%%%s%%') ",
                stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                strcat(sql2,temp);
                strcat(sql,temp);

            }
        }
        strcat(sql2,";");
        memset(temp,0,sizeof(temp));
        sprintf( temp ," order by create_time desc limit %d,%d; ",(stwp_ui2_json_recv.page - 1) * page_num,  page_num);
        strcat(sql,temp);
      //  sprintf(sql2,"SELECT COUNT(*) from  `%s` ; ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
      //  sprintf(sql,"select * from `%s`  order by create_time DESC limit %d,%d;",  stwp_util_get_tname_bytype(stwp_ui2_json_recv.type),  (stwp_ui2_json_recv.page - 1) * page_num, page_num);
    }
    else if(stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectPolicyTask)
    {//关联查询
        sprintf(sql2,"select count(t.uuid ) from stwp_policies_task as t inner join stwp_policies as p on t.policy_uuid = p.uuid  inner join stwp_hosts as h on t.host_uuid = h.uuid inner join stwp_users as u on t.create_user_uuid = u.uuid ");
        sprintf(sql,"select t.uuid as task_uuid, t.create_time,u.account as user,t.status as task_status,\
                    p.name as policy_name,p.type as policy_type,\
                    h.ip as host_ip \
                    from stwp_policies_task as t \
                    inner join stwp_policies as p on t.policy_uuid = p.uuid  \
                    inner join stwp_hosts as h on t.host_uuid = h.uuid \
                    inner join stwp_users as u on t.create_user_uuid = u.uuid ");

        if( (nstart !=0xFF && nend != 0xFF) || strlen(stwp_ui2_json_recv.keystr)  ){
            strcat( sql2,"where 1 ");
            strcat(sql,"where 1 ");
             if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ," AND ((t.create_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (t.create_time BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
                strcat(sql2,temp);
                strcat(sql, temp);
            }    
            if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ,"AND (p.name like '%%%s%%' or h.ip like '%%%s%%' or u.account like '%%%s%%') ",
                stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                strcat(sql2,temp);
                strcat(sql,temp);

            }
        }
        strcat(sql2,";");
        memset(temp,0,sizeof(temp));
        sprintf( temp ,"order by t.create_time desc limit %d,%d;",
                        (stwp_ui2_json_recv.page - 1) * page_num, page_num);
        strcat(sql,temp);
         /*
        sprintf(sql2,"select count(*) from (select t.uuid as task_uuid, t.create_time ,t.status as task_status,p.name as policy_name,h.ip as host_ip from stwp_policies_task as t inner join stwp_policies as p on t.policy_uuid = p.uuid  inner join stwp_hosts as h on t.host_uuid = h.uuid order by t.uuid)  as a");
        sprintf(sql,"select t.uuid as task_uuid, t.create_time,t.create_user_uuid as user,t.status as task_status,\
        p.name as policy_name,p.type as policy_type,\
        h.ip as host_ip \
        from stwp_policies_task as t \
        inner join stwp_policies as p on t.policy_uuid = p.uuid  \
        inner join stwp_hosts as h on t.host_uuid = h.uuid \
        order by t.create_time desc limit %d,%d;",   (stwp_ui2_json_recv.page - 1) * page_num, page_num);*/
    }
    else if( stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectUser)
    {
        sprintf(sql2,"SELECT COUNT(*) from  `%s`  ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
        sprintf(sql,"select uuid,account,name,address,email,phone,description,status ,permissions,create_time from `%s` ", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
        if( (nstart !=0xFF && nend != 0xFF) || strlen(stwp_ui2_json_recv.keystr)  ){
            strcat( sql2,"where 1 ");
            strcat(sql,"where 1 ");
             if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ," AND ((create_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (create_time BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
                strcat(sql2,temp);
                strcat(sql, temp);
            }    
            if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ,"AND ( account like '%%%s%%' or name like '%%%s%%' or address like '%%%s%%' or email like '%%%s%%' or phone like '%%%s%%' or description like '%%%s%%') ",
                stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                strcat(sql2,temp);
                strcat(sql,temp);

            }
        }
        strcat(sql2,";");
        memset(temp,0,sizeof(temp));
        sprintf( temp ," order by create_time desc limit %d,%d; ",(stwp_ui2_json_recv.page - 1) * page_num, page_num);
        strcat(sql,temp);

     //   sprintf(sql2,"SELECT COUNT(*) from  `%s` ; ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
     //   sprintf(sql,"select account,name,address,email,phone,description,status ,create_time from `%s`  order by create_time desc limit %d,%d;",  stwp_util_get_tname_bytype(stwp_ui2_json_recv.type),  (stwp_ui2_json_recv.page - 1) * page_num, page_num);
     }
    else if (stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectAWarningCount)
    {
        sprintf(sql,"select count(*) from `%s%s`  ;", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type),stwp_ui2_json_recv.uuid);
    }
    else if(stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectAuditCount)
    {
        sprintf(sql,"select count(*) from `%s%s` ;", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type),stwp_ui2_json_recv.uuid);
    }
    else if (stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectCotrlMode)
    {
        sprintf(sql,"select control_mode from `%s` where uuid = '%s'  ;", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type),stwp_ui2_json_recv.uuid);
    }
    else if (stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectSecurityMode)
    {
        sprintf(sql,"select safe_mode from `%s` where uuid = '%s' ;", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type),stwp_ui2_json_recv.uuid);
    }  
    else if (stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectHost)
    {

        sprintf(sql2,"SELECT COUNT(*) from  `%s`  ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
        sprintf(sql,"select * from `%s` ", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
        if( (nstart !=0xFF && nend != 0xFF) || strlen(stwp_ui2_json_recv.keystr)  ){
            strcat( sql2,"where 1 ");
            strcat(sql,"where 1 ");
             if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ," AND ((create_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (create_time BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
                strcat(sql2,temp);
                strcat(sql, temp);
            }    
            if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ,"AND ( name like '%%%s%%' or os_version like '%%%s%%' or mac_addr like '%%%s%%' or ip like '%%%s%%') ",
                stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                strcat(sql2,temp);
                strcat(sql,temp);

            }
        }
        strcat(sql2,";");
        memset(temp,0,sizeof(temp));
        sprintf( temp ," order by create_time desc limit %d,%d; ",(stwp_ui2_json_recv.page - 1) * page_num, page_num);
        strcat(sql,temp);

       // sprintf(sql2,"SELECT COUNT(*) from  `%s` ; ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
       // sprintf(sql,"select * from `%s` order by create_time desc  limit %d,%d;", \
          stwp_util_get_tname_bytype(STWP_VALUE_TYPE_SelectHost), \
           (stwp_ui2_json_recv.page - 1) * page_num, page_num);
 
    }
    else if(stwp_ui2_json_recv.type ==STWP_VALUE_TYPE_SelectHostByUuid){
        sprintf(sql2,"SELECT COUNT(*) from  `%s` where group_uuid ='%s' ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type),stwp_ui2_json_recv.uuid);
        sprintf(sql,"select * from `%s` where group_uuid ='%s' ", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type),stwp_ui2_json_recv.uuid);
        if( (nstart !=0xFF && nend != 0xFF) || strlen(stwp_ui2_json_recv.keystr)  ){
           
            if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ," AND ((create_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (create_time BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
                strcat(sql2,temp);
                strcat(sql, temp);
            }    
            if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ,"AND ( name like '%%%s%%' or os_version like '%%%s%%' or mac_addr like '%%%s%%' or ip like '%%%s%%') ",
                stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                strcat(sql2,temp);
                strcat(sql,temp);

            }
        }
        strcat(sql2,";");
        memset(temp,0,sizeof(temp));
        sprintf( temp ," order by create_time desc limit %d,%d; ",(stwp_ui2_json_recv.page - 1) * page_num, page_num);
        strcat(sql,temp);


    }
    else if (stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectAWarning)
    {
        sprintf(sql2,"SELECT COUNT(*) from  `%s%s`  ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid);
        sprintf(sql,"select uuid,type,contents,op_user, op_time,op_type,status from `%s%s` ", stwp_util_get_tname_bytype(STWP_VALUE_TYPE_SelectAWarning), stwp_ui2_json_recv.uuid);
        if( (nstart !=0xFF && nend != 0xFF) || strlen(stwp_ui2_json_recv.keystr)  ){
            strcat( sql2,"where 1 ");
            strcat(sql,"where 1 ");
             if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ," AND ((op_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (op_time BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
                strcat(sql2,temp);
                strcat(sql, temp);
            }    
            if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
                memset(temp,0,sizeof(temp));
                sprintf( temp ,"AND (uuid like '%%%s%%' or contents like '%%%s%%' or op_object like '%%%s%%') ",
                 stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                strcat(sql2,temp);
                strcat(sql,temp);

            }
        }
        strcat(sql2,";");
        memset(temp,0,sizeof(temp));
        sprintf( temp ," order by op_time desc limit %d,%d; ",(stwp_ui2_json_recv.page - 1) * page_num, page_num);
        strcat(sql,temp);
        #if 0
                if(strlen(stwp_ui2_json_recv.keystr))
                {
                    if( nstart !=0xFF && nend != 0xFF)
                    {//带时间间隔进行查询
                        sprintf(sql2,"SELECT COUNT(*) from  `%s%s` where ((op_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (op_time BETWEEN %d and %d)) AND (uuid like '%%%s%%' or contents like '%%%s%%' or op_object like '%%%s%%'); ",\
                            stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid,
                            nstart,nend,nstart,nend,
                            stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                        sprintf(sql,"select * from `%s%s` where ((op_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (op_time BETWEEN %d and %d)) AND (uuid like '%%%s%%' or contents like '%%%s%%' or op_object like '%%%s%%'); ", \
                            stwp_util_get_tname_bytype(STWP_VALUE_TYPE_SelectAWarning), 
                            stwp_ui2_json_recv.uuid,
                            nstart,nend,nstart,nend,
                            stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,
                            (stwp_ui2_json_recv.page - 1) * page_num, page_num);

                    }
                    else{
                        sprintf(sql2,"SELECT COUNT(*) from  `%s%s` where uuid like '%%%s%%' or contents like '%%%s%%' or op_object like '%%%s%%'; ",\
                            stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid,
                            stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
                        sprintf(sql,"select * from `%s%s` where uuid like '%%%s%%' or contents like '%%%s%%' or op_object like '%%%s%%' limit %d,%d;", \
                            stwp_util_get_tname_bytype(STWP_VALUE_TYPE_SelectAWarning), \
                            stwp_ui2_json_recv.uuid,
                            stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr,
                            (stwp_ui2_json_recv.page - 1) * page_num, page_num);
                    }
                //
                }
                else{
                    sprintf(sql2,"SELECT COUNT(*) from  `%s%s` ; ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type), stwp_ui2_json_recv.uuid);
                    sprintf(sql,"select * from `%s%s` limit %d,%d;", \
                        stwp_util_get_tname_bytype(STWP_VALUE_TYPE_SelectAWarning), \
                        stwp_ui2_json_recv.uuid, (stwp_ui2_json_recv.page - 1) * page_num, page_num);

                }
        #endif
            
    }
    else if(stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectHostPolicy){
        sprintf(sql2,"select count(t.uuid) from stwp_policies_task as t \
            inner join stwp_policies as p on t.policy_uuid = p.uuid  \
            inner join stwp_hosts as h on t.host_uuid = h.uuid \
            where t.host_uuid = '%s' ", stwp_ui2_json_recv.uuid);

        sprintf(sql,"select t.uuid as task_uuid, t.create_time,t.create_user_uuid as user,t.status as task_status,\
            p.name as policy_name,p.type as policy_type,\
            h.ip as host_ip \
            from stwp_policies_task as t \
            inner join stwp_policies as p on t.policy_uuid = p.uuid  \
            inner join stwp_hosts as h on t.host_uuid = h.uuid \
            where t.host_uuid = '%s' ", stwp_ui2_json_recv.uuid);

        
        if( nstart !=0xFF && nend != 0xFF ){//带时间间隔进行查询
            memset(temp,0,sizeof(temp));
            sprintf( temp ," AND ((t.create_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (t.create_time BETWEEN %d and %d)) ", nstart,nend,nstart,nend);
            strcat(sql2,temp);
            strcat(sql, temp);
        }    
        if( strlen(stwp_ui2_json_recv.keystr) ){//带模糊查询
            memset(temp,0,sizeof(temp));
            sprintf( temp ,"AND (p.name like '%%%s%%' or h.ip like '%%%s%%') ",
            stwp_ui2_json_recv.keystr,stwp_ui2_json_recv.keystr);
            strcat(sql2,temp);
            strcat(sql,temp);

        }
    
        strcat(sql2,";");
        memset(temp,0,sizeof(temp));
        sprintf( temp ,"order by t.create_time desc limit %d,%d;",
                        (stwp_ui2_json_recv.page - 1) * page_num, page_num);
        strcat(sql,temp);


        //sprintf(sql2,"select count(*) from (\
          select t.uuid as task_uuid, t.create_time,t.create_user_uuid as user,t.status as task_status,\
                 p.name as policy_name,p.type as policy_type,\
                 h.ip as host_ip from stwp_policies_task  as t\
              inner join stwp_policies as p on t.policy_uuid = p.uuid \
              inner join stwp_hosts as h on t.host_uuid = h.uuid  where t.host_uuid = '%s')  as a",stwp_ui2_json_recv.uuid);

        //sprintf(sql,"select t.uuid as task_uuid, t.create_time,t.create_user_uuid as user,t.status as task_status,\
        p.name as policy_name,p.type as policy_type,\
        h.ip as host_ip \
        from stwp_policies_task as t \
        inner join stwp_policies as p on t.policy_uuid = p.uuid  \
        inner join stwp_hosts as h on t.host_uuid = h.uuid \
        where t.host_uuid = '%s' \
        order by t.create_time desc limit %d,%d;", \
        stwp_ui2_json_recv.uuid,\
        (stwp_ui2_json_recv.page - 1) * page_num, page_num);
  
    }
    else if(stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectALLHost){
        //TODO: 这是查询所有主机的name，ip,uuid，注意outdata的长度
        sprintf(sql2,"SELECT COUNT(*) from  `%s` ; ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
        sprintf(sql,"select name,uuid,ip from `%s`;", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
    }
    else if(stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectNoGroupHost){
        //TODO: 这是查询不属于任何组的所有主机的name，ip,uuid，注意outdata的长度
        sprintf(sql2,"SELECT COUNT(*) from  `%s` where group_uuid is NULL or group_uuid=''; ",stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
        sprintf(sql,"select name,uuid,ip from `%s` where group_uuid is NULL or group_uuid='';", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
    }
    else if (stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectScrollInfo)
    {
        sprintf(sql,"select * from %s order by op_time desc limit 0,1;", stwp_util_get_tname_bytype(stwp_ui2_json_recv.type));
    }
    else if (stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectAll)
    {
        stwp_data_error_for_ui(stwp_ui2_json_recv.type,socket_fd);
    }
    else if (stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectHostDir)
    {
        stwp_data_error_for_ui(stwp_ui2_json_recv.type,socket_fd);
    }
    
    else
        goto error;
    
    if( strlen(sql))
    {
        memset(outdata,0,sizeof(outdata)); 
        
        stwp_mysql_select(sql, sql2,stwp_ui2_json_recv.type, outdata);
        printf("-----------------------------------------------\n");
        printf("sql    = %s \n", sql); 
        printf("result = %s \n",outdata);
        int nlen = strlen(outdata);
        send(socket_fd,&nlen,sizeof(int),0);
        ret= send(socket_fd,outdata,strlen(outdata),0);
        return ret;
    }  
    return 0;

error:
    stwp_data_error_for_ui(stwp_ui2_json_recv.type,socket_fd);
    return -1;
}

int doOpPolicyTaskDelete(cJSON *json,int socket_fd)
{
    /**
     *  
        {
        “type”：“”， //删除策略任务
        “user”：“”，//当前操作的用户
        “uuid”：[“”,""] //任务UUID
        }
    */
    char sql[MAX_LENGTH] = {0};
     
    int ret = 0;
    int errcode = STWP_VALUE_TYPE_ErrData;
    int type =0;
    char user[MINI_LENGTH] = {0x00};
    char uuid[MID_LENGTH] = {0x00};
    char outdata[MAX_LENGTH] = {0};

#if 1  //按协议定义，取字段。
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }

    cJSON *puser = cJSON_GetObjectItem(json, "user");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(user, puser->valuestring);
    }
    else {
      //  goto error;
    }
  
#endif
#if 1
    memset(sql,0,sizeof(sql));
    if(type == STWP_VALUE_TYPE_DeletePolicyTask)
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
                sprintf( sql, "DELETE FROM  %s where uuid = '%s'",stwp_util_get_tname_bytype(type),pTaskSub->valuestring);
                memset(outdata,0,sizeof(outdata)); 
                stwp_mysql_write(sql, type, outdata);
                printf("-----------------------------------------------\n");
                printf("sql    = %s \n", sql); 
                printf("result = %s \n",outdata);  
                
                write_ui_audit(STWP_AUDIT_TYPE_DeleteTask,user,pTaskSub->valuestring,"delete policy task");
            
            }
         }
        else
            goto error;
    }  
    else
        goto error;
#endif 
    int nlen = strlen(outdata);
    send(socket_fd,&nlen,sizeof(int),0);
    ret= send(socket_fd,outdata,strlen(outdata),0);
    return ret;    
   
error:
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return -1;

}

//处理UI2发来的将告警信息审批生成策略任务的请求
int doOpWarningToPolicyTask(cJSON *json,int socket_fd)
{
#if 0  //备注
    /** json :
    {
    "type":495,
    "user":"",  //当前操作用户
    "host_uuid":"",//主机UUID
    "data":[ //多条告警信息
    {"object":"xxxx","type":1,"op_type":2},
    {"object":"xxxx","type":2,"op_type":3}
        ]
    }
    */
   /** 策略任务表定义
     CREATE TABLE `stwp_policies_task`  (
     `id` int(11) NOT NULL AUTO_INCREMENT,
    `uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT 'TASK UUID',
    `host_uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '主机 UUID',
    `policy_uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '策略 UUID',
    `create_user_uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '创建者的UUID',
    `create_time` datetime(0) NULL DEFAULT NULL COMMENT '创建的时间',
    `status` int(11) NULL DEFAULT NULL COMMENT '状态：0：未执行；1：执行中 2：执行完成',
    PRIMARY KEY (`id`) USING BTREE,
    INDEX `index_uuid`(`uuid`) USING BTREE
    ) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic;

    */
#endif
    char sql[MAX_LENGTH] = {0};
    char outdata[MAX_LENGTH] = {0};
     
    int ret = 0;
    int i,j;
    int type =0; //请求包里type值 =495
    int w_type,w_optype; //warning表中的type 和 optype
    int p_type;//policies表中的type

    char user[MINI_LENGTH] = {0x00};
    char hosts[MAX_LENGTH] = {0x00};
    char object[MAX_LENGTH] = {0x00};    
    char new_policy_uuid[MID_LENGTH] = {0x00};
    char new_policy_task_uuid[MID_LENGTH]={0x00};
    char p_name[MINI_LENGTH] = {0x00};

   
    
#if 1  //按协议定义，取字段。
    cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype && ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    cJSON *puser = cJSON_GetObjectItem(json, "user");
    if(puser && (puser->type ==cJSON_String) ){
        strcpy(user, puser->valuestring);
    }
    else{
        goto error;
    }   
    cJSON *phost    = cJSON_GetObjectItem( json, "host_uuid");
    if(phost && (phost->type == cJSON_String ))
    {
        strcpy(hosts, phost->valuestring);
    }
    else{
        goto error;
    }   
    
#endif

    if( type != STWP_VALUE_TYPE_WarningToPolicyTask )
    {
         goto error;                
    }
#if 1
    cJSON *pdata     = cJSON_GetObjectItem( json, "data");
    //TODO: 修改告警的状态
    if( pdata != NULL )
    {
        int  data_size   = cJSON_GetArraySize ( pdata );
        for( i = 0 ; i < data_size ; i ++ ){
            cJSON * pDataSub = cJSON_GetArrayItem(pdata, i);
            if(NULL == pDataSub ){ continue ; }
            {
                cJSON *pobject = cJSON_GetObjectItem(pDataSub, "object");
                if(pobject && pobject->type == cJSON_String){
                    strcpy(object,pobject->valuestring);
                } else{ continue;}
                
                //cJSON *pptype = cJSON_GetObjectItem(pDataSub, "type");
                //if(pptype && (pptype->type ==cJSON_Number) ){
                //    w_type = pptype->valueint;
                //} else{ continue;}
               
                cJSON *poptype    = cJSON_GetObjectItem( pDataSub, "op_type");
                
                //cJSON *poptype    = cJSON_GetObjectItem( pDataSub, "type");
                if(poptype && (poptype->type == cJSON_Number ))
                {
                    w_optype = poptype->valueint;
                    /*审计动作：'1：非白名单列表内APP启动、2：白名单内程序被篡改、3：白名单内文件名路径被篡改、4：白名单内配置文件内容被篡改 '*/
                    #define STWP_AUDIT_TYPE_ACL_BOOT 1  //对应到stwp_polices表type = 0x00000008
                    #define STWP_AUDIT_TYPE_ACL_APP 2 //对应到stwp_polices表type = 0x10
                    #define STWP_AUDIT_TYPE_ACL_CONFIG 3 //对应到stwp_polices表type =0x20
                    #define STWP_AUDIT_TYPE_MEASURE_APP 4 //对应到stwp_polices表type =0x40
                    #define STWP_AUDIT_TYPE_MEASURE_DYNAMIC 5 //对应到stwp_polices表type =0x40

                    if( w_optype == STWP_AUDIT_TYPE_ACL_BOOT ){
                        sprintf(p_name,"AutoPolicy-ScanBoot-%lu",stwp_util_get_time());
                        p_type = 0x08;
                    }
                    else if(w_optype == STWP_AUDIT_TYPE_ACL_APP){
                        sprintf(p_name,"AutoPolicy-AddApp-%lu",stwp_util_get_time());
                        p_type = 0x10;
                    }
                    else if(w_optype ==STWP_AUDIT_TYPE_ACL_CONFIG ){
                        sprintf(p_name,"AutoPolicy-AddConfig-%lu",stwp_util_get_time());
                        p_type = 0x20;
                    }
                    else if(w_optype == STWP_AUDIT_TYPE_MEASURE_APP || w_optype == STWP_AUDIT_TYPE_MEASURE_DYNAMIC){
                        sprintf(p_name,"AutoPolicy-AddApp-%lu",stwp_util_get_time());
                        p_type = 0x10;
                    }
                    else continue;
                } else{ continue;}

                
                memset(new_policy_uuid,0,sizeof(new_policy_uuid));
                stwp_util_get_uuid(new_policy_uuid);
                memset(new_policy_task_uuid,0,sizeof(new_policy_task_uuid));
                stwp_util_get_uuid(new_policy_task_uuid);
               /* warning_to_policytask(
                    IN t_uuid varchar(64),
                    IN host_uuid varchar(64),    
                    IN t_time datetime(0),
                    IN p_uuid varchar(64),
                    IN p_contents mediumtext,
                    IN p_type int(11),
                    IN p_name varchar(256),
                    IN user varchar(64) )
                    */
                memset(sql,0,sizeof(sql));
                sprintf( sql, "call warning_to_policytask(\'%s\',\'%s\',FROM_UNIXTIME(%lu),\'%s\',\'%s\',%d,\'%s\',\'%s\') ;",
                 new_policy_task_uuid,hosts,stwp_util_get_time(),new_policy_uuid,object,p_type,p_name,user);

                memset(outdata,0,sizeof(outdata));                 
                stwp_mysql_write(sql, type, outdata);

                memset(sql,0,sizeof(sql));
                sprintf( sql, "update `stwp_warning_%s` set status=1 where op_object='%s' ; ",hosts,object);
                memset(outdata,0,sizeof(outdata));                 
                stwp_mysql_write(sql, type, outdata);
            
                printf("-----------------------------------------------\n");
                printf("sql    = %s \n", sql); 
                printf("result = %s \n",outdata);  

                write_ui_audit(STWP_AUDIT_TYPE_CreateTask,user,new_policy_task_uuid,"add from warning list to  policy task");
            }   
            
        }
    }
    else
        goto error;
    int nlen = strlen(outdata);
    send(socket_fd,&nlen,sizeof(int),0);
    ret= send(socket_fd,outdata,strlen(outdata),0);

    return ret;
#endif
error:
    stwp_data_error_for_ui(type,socket_fd);
    return -1;
}
//处理UI2发来的策略任务管理操作请求
int doOpPolicyTask(cJSON *json,int socket_fd)
{
   
#if 0  //备注
    /** json :
        {
        "type":0 ,
        “user”：“”，//当前操作的用户
        “host”：[“”，“”…] //uuid
        “policy”：[“”，“”…] //uuid
        }
    */
   /** 策略任务表定义
     CREATE TABLE `stwp_policies_task`  (
     `id` int(11) NOT NULL AUTO_INCREMENT,
    `uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT 'TASK UUID',
    `host_uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '主机 UUID',
    `policy_uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '策略 UUID',
    `create_user_uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '创建者的UUID',
    `create_time` datetime(0) NULL DEFAULT NULL COMMENT '创建的时间',
    `status` int(11) NULL DEFAULT NULL COMMENT '状态：0：未执行；1：执行中 2：执行完成',
    PRIMARY KEY (`id`) USING BTREE,
    INDEX `index_uuid`(`uuid`) USING BTREE
    ) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic;

    */
#endif
    char sql[MAX_LENGTH] = {0};
    char table_name[MID_LENGTH] = {0};
    char outdata[MAX_LENGTH] = {0};
     
    int ret = 0;
    int i,j;
    int type =0;
    char user[MINI_LENGTH] = {0x00};
    char hosts[MAX_LENGTH] = {0x00};
    char policies[MAX_LENGTH] = {0x00};
    
    char new_uuid[MID_LENGTH] = {0x00};
    char user_uuid[MID_LENGTH]={0x00};
    
#if 1  //按协议定义，取字段。
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype && ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    cJSON *puser = cJSON_GetObjectItem(json, "user");
    if(puser && (puser->type ==cJSON_String) ){
        strcpy(user, puser->valuestring);
    }
    else{
        goto error;
    }   

    
#endif

    if( type != STWP_VALUE_TYPE_ConfigPolicy )
    {
         goto error;                
    }

    cJSON *policy_arry     = cJSON_GetObjectItem( json, "policy");
    cJSON *host_arry     = cJSON_GetObjectItem( json, "host");
    if( policy_arry != NULL && host_arry != NULL )
    {
        int  policy_size   = cJSON_GetArraySize ( policy_arry );
        int  host_size = cJSON_GetArraySize(host_arry);
        printf("policy size = %d ,host size = %d\n" , policy_size,host_size);
        for( i = 0 ; i < policy_size ; i ++ ){
            cJSON * pPolicySub = cJSON_GetArrayItem(policy_arry, i);
            if(NULL == pPolicySub ){ continue ; }
            memset(policies,0,sizeof(policies));
            strcpy(policies,pPolicySub->valuestring);
            for( j = 0 ; j < host_size ; j ++ ){
                cJSON * pHostSub = cJSON_GetArrayItem(host_arry, j);
                if(NULL == pHostSub ){ continue ; }
                
                memset(hosts,0,sizeof(hosts));
                strcpy(hosts,pHostSub->valuestring);                
                printf("policy[%d] = %s, --> host[%d]= %s \n",i,policies,j,hosts);
                
                memset(sql,0,sizeof(sql));
                memset(new_uuid,0,sizeof(new_uuid));
                stwp_util_get_uuid(new_uuid);
                memset(user_uuid,0,sizeof(user_uuid));
               // getUserUUID(user,user_uuid);
                sprintf( sql, "INSERT INTO %s (uuid,host_uuid,policy_uuid,status,create_time,create_user_uuid)\
                 VALUES(\'%s\',\'%s\',\'%s\', %d,FROM_UNIXTIME(%lu),(select uuid from stwp_users where account = '%s'))",\
                 stwp_util_get_tname_bytype(type),new_uuid,hosts,policies,0,stwp_util_get_time(), user);
                memset(outdata,0,sizeof(outdata)); 
                
                stwp_mysql_write(sql, type, outdata);
                printf("-----------------------------------------------\n");
                printf("sql    = %s \n", sql); 
                printf("result = %s \n",outdata);  

                write_ui_audit(STWP_AUDIT_TYPE_CreateTask,user,new_uuid,"create policy task");
            }          
           
        }
    }
    else
        goto error;
    int nlen = strlen(outdata);
    send(socket_fd,&nlen,sizeof(int),0);
    ret= send(socket_fd,outdata,strlen(outdata),0);

    return ret;
error:
    stwp_data_error_for_ui(type,socket_fd);
    return -1;
}

//处理UI2发来的策略添加、修改、删除请求
int doOpPolicy(cJSON *json,int socket_fd)
{
#if 0  //备注
    /** json :
       {
        “type”：“”， //添加、修改或删除策略
        “user”：“”，//当前操作的用户
        “name”：“”，//策略名称
        “uuid”：“”， //修改和删除时使用
        “op_type”:“ ”，//具体策略类型（删除策略时为空）
        “data”:“” //策略内容（以；隔开）
        }
    */
   /** 策略表定义
    CREATE TABLE `stwp_policies`  (
    `id` int(11) NOT NULL AUTO_INCREMENT,
    `uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '策略的 UUID',
    `name` varchar(256) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '策略名称',
    `type` int(11) NULL DEFAULT NULL COMMENT '策略类型：STWP_POLICY_TYPE_SecurityMode       0x00000001; STWP_POLICY_TYPE_ControlMode        0x00000002;STWP_POLICY_TYPE_ScanAllApp         0x00000004; STWP_POLICY_TYPE_ScanBoot           0x00000008;STWP_POLICY_TYPE_AddAppList         0x00000010;STWP_POLICY_TYPE_AddConfigList      0x00000020;STWP_POLICY_TYPE_AddDynamicList     0x00000040;STWP_POLICY_TYPE_UpdateAppList      0x00000080;STWP_POLICY_TYPE_UpdateBootList     0x00000100;STWP_POLICY_TYPE_UpdateConfigList   0x00000200;STWP_POLICY_TYPE_UpdateDynamicList  0x00000400;STWP_POLICY_TYPE_UpdateApp          0x00000800;STWP_POLICY_TYPE_CleanHost          0x00001000;',
    `contents` mediumtext CHARACTER SET utf8 COLLATE utf8_general_ci NULL COMMENT '策略内容，最大可放100条列表目录 1024*25',
    `create_user_uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '创建者的UUID',
    `create_time` datetime(0) NULL DEFAULT NULL COMMENT '策略创建的时间',
    `status` int(11) NULL DEFAULT NULL COMMENT '策略状态：0：无效；1：有效。',
    PRIMARY KEY (`id`) USING BTREE,
    INDEX `index_uuid`(`uuid`) USING BTREE
    ) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic;

    */
#endif
    char sql[MAX_LENGTH] = {0};
    char table_name[MID_LENGTH] = {0};
    char outdata[MAX_LENGTH * 20] = {0};
     
    int ret = 0;

    int type =0, op_type = -1;
    char user[MINI_LENGTH] = {0x00};
    char uuid[MID_LENGTH] = {0x00};
    char name[MID_LENGTH] = {0x00};
    char data[MAX_LENGTH*10]={0x00};

 
    char new_uuid[MID_LENGTH] = {0x00};
    char user_uuid[MID_LENGTH]={0x00};
#if 1  //按协议定义，取字段。
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    cJSON *ppage = cJSON_GetObjectItem(json, "op_type");
    if(ppage&& ppage->type == cJSON_Number){
        op_type= ppage->valueint;
    }
    else{
        //在添加策略时判断：无此字段返回错误
    }

    cJSON *puuid = cJSON_GetObjectItem(json, "uuid");
    if(puuid && (puuid->type ==cJSON_String)){
        strcpy(uuid, puuid->valuestring);
    }
    else{
       // goto error;
    }   

    cJSON *pname = cJSON_GetObjectItem(json, "name");
    if(pname && (pname->type ==cJSON_String)){
        strcpy(name, pname->valuestring);
    }
    else {
        //goto error;
    }
    cJSON *puser = cJSON_GetObjectItem(json, "user");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(user, puser->valuestring);
    }
    else {
        goto error;
    }
     cJSON *pdata = cJSON_GetObjectItem(json, "data");
    if(pdata && (pdata->type ==cJSON_String)){
        strcpy(data, pdata->valuestring);
    }
    else {
        //goto error;
    }
#endif
#if 1
    if( type == STWP_VALUE_TYPE_AddPolicy )
    {
     //   if(getPolicyType(op_type) == -1)
     //       goto error;
        stwp_util_get_uuid(new_uuid);
        getUserUUID(user,user_uuid);
    
        //printf("INSERT INTO %s (uuid,name,type,contents,status,create_time,create_user_uuid) VALUES\
        (\'%s\',\'%s\',%d, \'%s\', %d,%lu,\'%s\')",\
        stwp_util_get_tname_bytype(type),new_uuid,name,getPolicyType(op_type),data,1,stwp_util_get_time(), user_uuid);
            
        sprintf( sql, "INSERT INTO %s (uuid,name,type,contents,status,create_time,create_user_uuid) VALUES\
        (\'%s\','%s',%d, '%s', %d,FROM_UNIXTIME(%lu),(select uuid from stwp_users where account = '%s'))",\
        stwp_util_get_tname_bytype(type),new_uuid,name,(op_type),data,1,stwp_util_get_time(), user);
        write_ui_audit(STWP_AUDIT_TYPE_CreatePolicy,user,new_uuid,"create policy");    
    }
    else if(type == STWP_VALUE_TYPE_DelectPolicy)
    {
        cJSON *policy_arry     = cJSON_GetObjectItem( json, "uuid");
        if( policy_arry != NULL)
        {
            int  task_size   = cJSON_GetArraySize ( policy_arry );
            int i = 0;
            for( i = 0 ; i < task_size ; i ++ ){
                cJSON * pTaskSub = cJSON_GetArrayItem(policy_arry, i);
                if(NULL == pTaskSub ){ continue ; }

                memset(sql,0,sizeof(sql));
                //sprintf( sql, "DELETE FROM  %s where uuid = '%s'",stwp_util_get_tname_bytype(type),pTaskSub->valuestring);
                sprintf( sql, "DELETE FROM  %s where uuid = '%s'",stwp_util_get_tname_bytype(type),pTaskSub->valuestring);
                memset(outdata,0,sizeof(outdata)); 
                stwp_mysql_write(sql, type, outdata);
                printf("-----------------------------------------------\n");
                printf("sql    = %s \n", sql); 
                printf("result = %s \n",outdata);                  
                write_ui_audit(STWP_AUDIT_TYPE_DeletePolicy,user,pTaskSub->valuestring,"delete policy"); 
            
            }
            int nlen = strlen(outdata);
            send(socket_fd,&nlen,sizeof(int),0);
            ret= send(socket_fd,outdata,strlen(outdata),0);
            return ret;
          
         }
        else
            goto error;
   
       
        
    }
    else if(type == STWP_VALUE_TYPE_ModifyPolicy )
    {
        
        sprintf( sql, "UPDATE %s SET name='%s',type=%d,contents='%s',create_time=FROM_UNIXTIME(%lu) where uuid = '%s'",\
        stwp_util_get_tname_bytype(type),name,(op_type),data,stwp_util_get_time(),uuid); 
        write_ui_audit(STWP_AUDIT_TYPE_ModifyPolicy,user,uuid,"modify policy");
    }
    else
        goto error;
#endif 
    if(strlen(sql))
    {
        memset(outdata,0,sizeof(outdata)); 
        stwp_mysql_write(sql, type, outdata);
        printf("-----------------------------------------------\n");
        printf("sql    = %s \n", sql); 
        printf("result = %s \n",outdata);
        int nlen = strlen(outdata);
        send(socket_fd,&nlen,sizeof(int),0);
        ret= send(socket_fd,outdata,strlen(outdata),0);
        return ret;
    }       
    
    return 0;
error:
   stwp_data_error_for_ui(type,socket_fd);
    return -1;
}

//处理UI2发来的统计类查询
int doOpCollectQuery(cJSON *json,int socket_fd)
{

   stwp_p2_query stwp_ui2_json_recv;
    char sql[MAX_LENGTH] = {0};
    char table_name[MID_LENGTH] = {0};
    char outdata[MAX_LENGTH * 5] = {0};
    int num = 0;
    int page_num = 15;
    int ret = 0;

    memset(&stwp_ui2_json_recv,0,sizeof(stwp_p2_query));
    //按QUERY类JSON协议定义，取字段。
#if 1
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        stwp_ui2_json_recv.type = ptype->valueint;
        //printf("type = %d\n",stwp_ui2_json_recv.type);
    }
    else{
        goto error;
    }
    /*
    cJSON *ppage = cJSON_GetObjectItem(json, "currentPage");
    if(ppage){
        stwp_ui2_json_recv.page= ppage->valueint;
        //printf("currentPage = %d\n",stwp_ui2_json_recv.page);
    }
    else{
        goto error;
    }

    cJSON *uuid_or_ip = cJSON_GetObjectItem(json, "uuid");
    if(uuid_or_ip){
        //strcpy(stwp_ui2_json_recv.uuid, uuid_or_ip->valuestring);
    }
    else{
       goto error;
    }    

    cJSON *pkey = cJSON_GetObjectItem(json, "keystr");
    if(pkey){
        strcpy(stwp_ui2_json_recv.ip, pkey->valuestring);
    }
    else{
        goto error;
    }*/
#endif
    //组织SQL，查表，发送给UI
    if(stwp_ui2_json_recv.type == STWP_VALU_TYPE_CollectQuery )
    {
#if 0
{
“type”：0，
“errcode”:0,
 “safestatus”：0， //查询安全模式比例
“onlinestatus”：0， 查询在线比例
“controlstatus”：0，//查询控制模式比例
“op_type1_daycount”:0，//查询异常启动信息,非白名单列表内APP启动
“op_type2_daycount”:0, //查询异常启动信息,白名单内程序被篡改
“op_type3_daycount”:0, //查询异常启动信息,白名单内文件名路径被篡改
“op_type4_daycount”:0，//查询异常启动信息,篡改白名单内配置文件内容被篡改事件
}
`host_status` int(10) NULL DEFAULT NULL COMMENT '主机状态: 0:离线、1:在线',
  `safe_mode` int(10) NULL DEFAULT NULL COMMENT '安全模式: 0:关闭、1:审计、2:拦截',
  `control_mode` int(10) NULL DEFAULT NULL COMMENT '控制模式: 0:单机控制、1:联网控制',
#endif
/*
    sprintf(sql,"select count(*) as t_host_sts,
    sum(case when host_status=0 then 1 else 0 end) host_offline,
    sum(case when host_status=0 then 1 else 0 end)*1.0/count(*) host_offline_percent,
    sum(case when host_status=1 then 1 else 0 end) host_online ,
    sum(case when host_status=1 then 1 else 0 end)*1.0/count(*)  host_online_percent,
    sum(case when control_mode=0 then 1 else 0 end) ctrl_offline,
    sum(case when control_mode=0 then 1 else 0 end)*1.0/count(*) ctrl_offline_percent,
    sum(case when control_mode=1 then 1 else 0 end) ctrl_online ,
    sum(case when control_mode=1 then 1 else 0 end)*1.0/count(*)  ctrl_online_percent,
    sum(case when safe_mode=0 then 1 else 0 end) safe_off,
    sum(case when safe_mode=0 then 1 else 0 end)*1.0/count(*) safe_off_percent,
    sum(case when safe_mode=1 then 1 else 0 end) safe_on ,
    sum(case when safe_mode=1 then 1 else 0 end)*1.0/count(*)  safe_on_percent,
    sum(case when safe_mode=2 then 1 else 0 end) safe_ctrl ,
    sum(case when safe_mode=2 then 1 else 0 end)*1.0/count(*)  safe_ctrl_percent
    from stwp_hosts;");

    sprintf(sql,"SELECT * FROM stwp_warning ORDER BY create_time DESC LIMIT 1;");
*/  
    //Select a.*,b.* from( select * from t1 ) as a, (select * from t2) as b;
    sprintf(sql,"SELECT per.*,error.* from \
             (SELECT count(*) as t_host_sts,\
                sum(case when host_status=0 then 1 else 0 end) host_offline,\
                sum(case when host_status=1 then 1 else 0 end) host_online ,\
                sum(case when control_mode=0 then 1 else 0 end) ctrl_offline,\
                sum(case when control_mode=1 then 1 else 0 end) ctrl_online ,\
                sum(case when safe_mode=0 then 1 else 0 end) safe_off,\
                sum(case when safe_mode=1 then 1 else 0 end) safe_on ,\
                sum(case when safe_mode=2 then 1 else 0 end) safe_ctrl \
                from stwp_hosts) as per ,\
            (SELECT op_type1_daycount,op_type2_daycount,op_type3_daycount,op_type4_daycount FROM stwp_warning ORDER BY time DESC LIMIT 1) as error;");
    }
    else if (stwp_ui2_json_recv.type == STWP_VALUE_TYPE_SelectHistory)
    {//查询stwp_warning表中最近30天的记录，每天一条记录
        sprintf(sql,"SELECT time,op_total_count FROM stwp_warning where DATE_SUB(CURDATE(), INTERVAL 30 DAY) <= date(time)  order by time DESC ");
    }
    else
        goto error;
    
   
    if( strlen(sql))
    {
        memset(outdata,0,sizeof(outdata)); 
        stwp_mysql_select(sql,"", stwp_ui2_json_recv.type, outdata);
        printf("-----------------------------------------------\n");
        printf("sql    = %s \n", sql); 
        printf("result = %s \n",outdata);
        int nlen = strlen(outdata);
        send(socket_fd,&nlen,sizeof(int),0);
        ret= send(socket_fd,outdata,strlen(outdata),0);
        return ret;
    }  
    return 0;

error:
    stwp_data_error_for_ui(stwp_ui2_json_recv.type,socket_fd);
    return -1;
}

//处理UI2发来的导出数据请求
extern int  doDirServer(cJSON *root_json,int socket_fd);
int doOpExportData(cJSON* json,int socket_fd)
{

    #if 0  //备注
        /** json :
        {
        “type”：“”， //导出操作 
        “user”：“”，//当前操作的用户
        “op_type”：[int，int …]， //具体的导出操作
        “starttime”：“”，
        “endtime”：“”，
        “host”：[“”，“”…] //主机 uuid
        }
        */
    #endif
    char sql[MAX_LENGTH] = {0};
    char temp[PATH_LENGTH] = {0};
    
    int ret = 0,page_num=15;
    int errcode =STWP_VALUE_TYPE_ErrData; 
    int type =0, op_type =-1,i=0,j=0;
    int nstart=1,nend=1;
    char user[MINI_LENGTH] = {0x00};


    
    char host[MID_LENGTH] = {0x00};
    char filename[PATH_LENGTH]={0x00};
 
 
    //char new_uuid[MID_LENGTH] = {0x00};
    char user_uuid[MID_LENGTH]={0x00};
  //按协议定义，取字段。
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    cJSON *puser = cJSON_GetObjectItem(json, "user");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(user, puser->valuestring);
    }
    else {
        goto error;
    }
    cJSON *pfile = cJSON_GetObjectItem(json, "file");
    if(pfile && (pfile->type ==cJSON_String)){
        strcat(filename,"/usr/local/stwp/sftp/mysftp/upload/");
        strcat(filename, pfile->valuestring);
        
    }
    else {
        goto error;
    }
    cJSON *phost  = cJSON_GetObjectItem( json, "host");
    if(phost && phost->type == cJSON_String){
       strcpy(host,phost->valuestring);  
    }

    cJSON *pStart = cJSON_GetObjectItem(json, "start");
    if(pStart&& pStart->type == cJSON_Number){
        nstart = pStart->valueint;
       
    }
    else{
        nstart=0xFF;
    }   

    cJSON *pend = cJSON_GetObjectItem(json, "end");
    if(pend&& pend->type == cJSON_Number){
       nend = pend->valueint;
    }
    else {
        nend=0xFF;
    }
 
    if( type != STWP_VALUE_TYPE_Export )
    {
        goto error;           
    }
    cJSON *op_arry     = cJSON_GetObjectItem( json, "op_type");
  //  cJSON *host_arry     = cJSON_GetObjectItem( json, "host");
    if( op_arry != NULL)// && host_arry != NULL )
    {
        int  op_size   = cJSON_GetArraySize ( op_arry );
    //    int  host_size = cJSON_GetArraySize(host_arry);
     //   printf("policy size = %d ,host size = %d\n" , op_size,host_size);
        for( i = 0 ; i < op_size ; i ++ ){
            cJSON * pOpSub = cJSON_GetArrayItem(op_arry, i);
            if(NULL == pOpSub ){ continue ; }
            op_type = pOpSub->valueint;
        
            memset(sql,0,sizeof(sql));
        
            if( op_type == STWP_VALUE_TYPE_ExportAppList||
                op_type == STWP_VALUE_TYPE_ExportBootList||
                op_type == STWP_VALUE_TYPE_ExportConifgList||
                op_type == STWP_VALUE_TYPE_ExportDynamicList ){
                
                if(nstart == 0xFF  && 0xFF == nend){
                    //取所有
                    sprintf(sql,"select * from `%s%s`;", \
                    stwp_util_get_tname_bytype(op_type), host);
                }
                else{
                    sprintf(sql,"select * from `%s%s`  limit %d,%d;", \
                    stwp_util_get_tname_bytype(op_type), host, (nstart - 1) * page_num, nend * page_num);
                } 
                sprintf(temp,"%s%s",stwp_util_get_tname_bytype(op_type), host);
                write_ui_audit(STWP_AUDIT_TYPE_ExportWhiteList,user,temp,"export white list data");
                
            }
            else if( op_type == STWP_VALUE_TYPE_ExportHostAudit||
                op_type == STWP_VALUE_TYPE_ExportWarning){
                    if(nstart ==0xFF  && 0xFF == nend){
                    //取所有
                    sprintf( sql, "SELECT * from `%s%s`;", stwp_util_get_tname_bytype(op_type),host);
                }
                else{
                sprintf( sql, "SELECT * from `%s%s` where (op_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (op_time BETWEEN %d and %d) ;", \
                stwp_util_get_tname_bytype(op_type),host,nstart,nend,nstart,nend);
                }
                sprintf(temp,"%s%s",stwp_util_get_tname_bytype(op_type), host);
                write_ui_audit(STWP_AUDIT_TYPE_ExportWhiteList,user,temp,"export data");
            }
            else if( op_type == STWP_VALUE_TYPE_ExportCenterAudit){
                    if(nstart ==0xFF  && 0xFF == nend){
                    //取所有
                    sprintf( sql, "SELECT * from `%s`;", stwp_util_get_tname_bytype(op_type));
                }
                else{
                sprintf( sql, "SELECT * from `%s` where  (op_time BETWEEN FROM_UNIXTIME(%lu) and FROM_UNIXTIME(%lu)) OR (op_time BETWEEN %d and %d);", \
                stwp_util_get_tname_bytype(op_type),nstart,nend,nstart,nend);
                }
                sprintf(temp,"%s",stwp_util_get_tname_bytype(op_type));
                write_ui_audit(STWP_AUDIT_TYPE_ExportWhiteList,user,temp,"export data");
            }
            else 
                continue;

            stwp_mysql_export2csv(sql, filename);
            errcode = STWP_VALUE_TYPE_ErrSucceed;
        }          
        
    
    }
    else
        goto error;
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return 0;
error:
    stwp_data_error_for_ui2(type,socket_fd,errcode);
    return -1;
}

st_cmd_table cmd_table[]={
//查询应用白名单
{STWP_VALUE_TYPE_SelectAppList      ,    doOpQuery, 0xFF},//          220
//查询boot白名单        
{STWP_VALUE_TYPE_SelectBootList     ,    doOpQuery, 0xFF},//              221
//查询配置白名单        
{STWP_VALUE_TYPE_SelectConifgLis    ,    doOpQuery, 0xFF},//             222
//查询动态度量白名单
{STWP_VALUE_TYPE_SelectDynamicList  ,    doOpQuery, 0xFF},//          223
//查询表数量
{ STWP_VALUE_TYPE_SelectAll         ,    doOpQuery, 0xFF},//                228
//查询告警信息
{STWP_VALUE_TYPE_SelectAWarning     ,    doOpQuery, 0xFF},//            230
//查询告警信息数量
{STWP_VALUE_TYPE_SelectAWarningCount,    doOpQuery, 0xFF},//         231
//查询控制模式
{STWP_VALUE_TYPE_SelectCotrlMode    ,    doOpQuery, 0xFF},//         235
//查询安全模式
{STWP_VALUE_TYPE_SelectSecurityMode ,    doOpQuery, 0xFF},//        236
//查询审计信息
{STWP_VALUE_TYPE_SelectAudit        ,    doOpQuery, 0xFF},//        237
//查询审计信息数量
{STWP_VALUE_TYPE_SelectAuditCount   ,    doOpQuery, 0xFF},//       238
//查询注册主机信息
{STWP_VALUE_TYPE_SelectHost         ,    doOpQuery, 0xFF},//        251
//查询主机的策略，在策略任务表中查指定主机的策略
{STWP_VALUE_TYPE_SelectHostPolicy   ,    doOpQuery, 0xFF},//         252
//查询所有主机的IP，NAME，UUID
{STWP_VALUE_TYPE_SelectALLHost      ,    doOpQuery, 0xFF},//       253

{STWP_VALUE_TYPE_SelectNoGroupHost  ,    doOpQuery,0xFF},
//根据组查主机
{STWP_VALUE_TYPE_SelectHostByUuid   ,   doOpQuery,0xFF},//254
//查询所有组
{STWP_VALUE_TYPE_SelectGroup        ,    doOpQuery, 0xFF},//        255
//查询用户信息
{STWP_VALUE_TYPE_SelectUser         ,    doOpQuery, 0xFF},//          271
//查询center审计
{STWP_VALUE_TYPE_SelectCenterAudit  ,    doOpQuery, 0xFF},//        291
//查询策略表，无条件分页查
{STWP_VALUE_TYPE_SelectPolicy       ,    doOpQuery, 0xFF},//      300
//查询策略任务表，无条件分页查所有
{STWP_VALUE_TYPE_SelectPolicyTask   ,    doOpQuery, 0xFF},//      301
//查询主机目录
{STWP_VALUE_TYPE_SelectHostDir      ,    doOpQuery, 0xFF},//      310
//查询滚动信息
{STWP_VALUE_TYPE_SelectScrollInfo   ,    doOpQuery, 0xFF},//      320
//查询统计数据
{STWP_VALU_TYPE_CollectQuery        ,    doOpCollectQuery, 0xFF},//        330
//查询异常启动信息历史数据
{STWP_VALUE_TYPE_SelectHistory      ,    doOpCollectQuery, 0xFF},//     331
//主机删除 
{STWP_VALUE_TYPE_DeletHost         ,    doOpHostDelete     , 0xFF},//    460
//主机修改	461	
{STWP_VALUE_TYPE_ModifyHost        ,    doOpHostModify     , 0xFF},//    461
//主机添加	462	
{STWP_VALUE_TYPE_AddHost           ,    doOpHostAddNew     , 0xFF},//    462
//添加组	470	
{STWP_VALUE_TYPE_AddGroup          ,      doOpGroupAddNew  , 0xFF},// 470
//修改组	471	
{STWP_VALUE_TYPE_ModifyGroup       ,    doOpGroupModify     , 0xFF},//  471
//删除组	472	
{STWP_VALUE_TYPE_DelGroup           ,   doOpGroupDelete     , 0xFF},// 472
//批量向组内加主机	473	
{STWP_VALUE_TYPE_AddHostToGroup     ,   doOpGroupAddHost    , 0xFF},// 473
//批量删除组内主机	474	
{STWP_VALUE_TYPE_RemoveHostFromGroup,   doOpGroupRemoveHost , 0xFF},//  474

/*写数据库操作*/
//用户添加，0管理，1审计，2操作
{STWP_VALUE_TYPE_AddUser           ,    doOpUserAddNew, 0xFF},//          480
//用户删除
{STWP_VALUE_TYPE_DeleteUser        ,    doOpUserDelete, 0xFF},//                  481
//用户修改
{STWP_VALUE_TYPE_ModifyUser        ,    doOpUserModify, 0xFF},//    482
//策略添加      
{STWP_VALUE_TYPE_AddPolicy         ,    doOpPolicy, 0xFF},//       490
//策略删除
{STWP_VALUE_TYPE_DelectPolicy      ,    doOpPolicy, 0xFF},//      491
//策略修改   
{STWP_VALUE_TYPE_ModifyPolicy      ,    doOpPolicy, 0xFF},//        492
//策略配置 ，生成策略任务  
{STWP_VALUE_TYPE_ConfigPolicy      ,    doOpPolicyTask, 0xFF},//     493
//删除策略任务
{STWP_VALUE_TYPE_DeletePolicyTask  ,    doOpPolicyTaskDelete, 0xFF},//         494

//告警信息审批
{STWP_VALUE_TYPE_WarningToPolicyTask  ,doOpWarningToPolicyTask,0xFF},//        495
//导出信息	500	
{STWP_VALUE_TYPE_Export            ,    doOpExportData, 0xFF},//    500
//登录验证	601	
{STWP_VALUE_TYPE_VerifyUser        ,    doOpUserCheckLogin, 0xFF},//     601
//修改密码	602	
{STWP_VALUE_TYPE_ChangePassword    ,    doOpUserChangePWD, 0xFF},//    602
{STWP_VALUE_TYPE_DirServer,doDirServer,0xFF},
{0,0,-1}
};

/**
 * 根据请求查表返回处理函数
 */
pfunc_req_handler* get_req_handler( int nType)
{
    int i = 0 ;
    for( i = 0 ; ; i++)
    {
        if(cmd_table[i].req_type == 0)
            return NULL;
        if( nType == cmd_table[i].req_type)
            return &cmd_table[i].handler;        
    }
    return NULL;
}

/**
 * 根据请求查表返回要求的权限值
 */
int get_req_priv(int nType)
{
    int i = 0 ;
    for( i = 0 ; ; i++)
    {
        if(cmd_table[i].req_type == 0)
            return -1;
        if( nType == cmd_table[i].req_type)
            return cmd_table[i].priv;        
    }
    return -1;
}

/**
 * @description: 处理socket client UI2 发来的请求
 * @param: json_from_ui ,以JSON格式发来的请求数据
 * @param: socket_fd,socket连接句柄
 */
int handle_ui_request(char *json_from_ui, int socket_fd)
{
    int ret = 0;
    int type = 0;
    char *outdata;
    cJSON *json;
  
   // printf("Receive data from client [%d] :%s\n",socket_fd,json_from_ui);
    json = cJSON_Parse(json_from_ui);
    if (NULL == json)
    {
        stwp_logdump_module.push("[stwp_p2] message is not json ,error=:%s\n", cJSON_GetErrorPtr());
       // cJSON_Delete(json);
        stwp_data_error_for_ui(0,socket_fd);
        return -1;
    }
  
  
    cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number)
    {
   
        type = ptype->valueint;
        int priv = get_req_priv(type);
        if(priv == -1) //no type
            goto error;
        if( priv == 0xFF) //不需要权限即可执行
            ;
        pfunc_req_handler* phandler;
        phandler = get_req_handler(type);
        if( phandler )
        {
            return (*phandler)(json,socket_fd);
        }
        else
            goto error;
#if 0
        stwp_logdump_module.push("[stwp_p2] receive json ,type = \n", type);
        
        //TODO: UI各类操作入审计库
        /* ptype->valueint ：200~399之间为查询请求 */
        if( type > 200 && type < 329){
            return doOpQuery(json,socket_fd);
        }
        else if(type == STWP_VALUE_TYPE_AddPolicy ||
                type == STWP_VALUE_TYPE_DelectPolicy||
                type ==STWP_VALUE_TYPE_ModifyPolicy){
            return doOpPolicy(json,socket_fd);
        }
        else if(type == STWP_VALUE_TYPE_ConfigPolicy){
            return doOpPolicyTask(json,socket_fd);
        }
        else if(type == STWP_VALUE_TYPE_DeletePolicyTask){
            return doOpPolicyTaskDelete(json,socket_fd);
        }
        else if(type == STWP_VALUE_TYPE_AddUser||
            type == STWP_VALUE_TYPE_DeleteUser ||
            type == STWP_VALUE_TYPE_ModifyUser ||
            type == STWP_VALUE_TYPE_VerifyUser||
            type == STWP_VALUE_TYPE_ChangePassword){
            return doOpUser(json,socket_fd);
        }
        else if(type == STWP_VALU_TYPE_CollectQuery||
            type == STWP_VALUE_TYPE_SelectHistory)
            return doOpCollectQuery(json,socket_fd);
        else if(type == STWP_VALUE_TYPE_Export){
            return doOpExportData(json,socket_fd);
        }
#endif
    }
    else
    {
        goto error;
    } 
   
    
error:
    cJSON_Delete(json);  
    stwp_data_error_for_ui2(type,socket_fd,STWP_VALUE_TYPE_ErrData);
    return -1;
}
