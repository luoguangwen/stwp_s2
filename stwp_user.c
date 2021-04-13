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

#include "stwp_md5.h"
#include "stwp_user.h"
#include "stwp_define.h"
#include "stwp_session.h"
//处理UI2发来的用户添加请求
int doOpUserAddNew(cJSON *json,int socket_fd)
{
/**  UI2 --> S2 的JSON包
  {
   "type":0 ,
   “c_user”："" //当前登录用户， 
   "c_pwd" : ""  //当前用户的密码
   
    “account”:""// 被操作用户的account
    “name”：”xxx”, 被操作的
    “passwd”：”xxxx”，被操作的
    “permision”：0，被操作的。
    “phone”:”xxxx”,被操作的
    “mail”:”xxxxx”被操作的
    }
 */
/** stwp_users表字段
 CREATE TABLE `stwp_users`  (
  `id` int(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `uuid` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL,
  `account` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '账号，(关键字，唯一 限英文字符)',
  `password` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '密码hmac加密结果',
  `password_salt` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMM盐'ENT '密码 STORAGE,
  `histroy_password` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '历史上次密码hmac加密结果',
  `histroy_password_salt` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '历史上次密码盐',
  `pwd_mat` int(11) NULL DEFAULT NULL COMMENT '24小时内密码最大尝试次数，自然天处理清除次数 1-10次',
  `pwd_rat` int(11) NULL DEFAULT NULL COMMENT '24小时内密码剩余尝试次数，自然天处理清除次数',
  `status` int(11) NULL DEFAULT -1 COMMENT '用户状态：0=未激活；1=已激活；99=已锁定；-1=已删除  (创建用户后未激活，修改密码激活)',
  `name` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '用户姓名',
  `address` varchar(1024) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '用户地址',
  `email` varchar(128) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '电子邮件',
  `phone` varchar(24) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '手机号码，用于密码找回和消息推送',
  `description` varchar(255) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL,
  `permissions` varchar(64) CHARACTER SET utf8 COLLATE utf8_general_ci NULL DEFAULT NULL COMMENT '用户权限 关联权限表uuid',
  `pwd_update_time` datetime(0) NULL DEFAULT NULL COMMENT '密码修改时间',
  `pwd_expire_time` datetime(0) NULL DEFAULT NULL COMMENT '密码有效期3个月，过期强制修改密码后使用',
  `create_time` datetime(0) NULL DEFAULT NULL,
  `delete_time` datetime(0) NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `uuid`(`uuid`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1 CHARACTER SET = utf8 COLLATE = utf8_general_ci ROW_FORMAT = Dynamic;

 */

     
    int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;

    int type =0, lock = -1;
   
    char c_user[MINI_LENGTH] = {0x00};
    char c_pwd[MINI_LENGTH] = {0x00};

    
    char new_uuid[MID_LENGTH] = {0x00};    
    
    char password[MID_LENGTH]={0x00};
    char encpwd[MID_LENGTH]={0x00};
    

    stwp_user stwpuser;
    memset(&stwpuser,0,sizeof(stwp_user));
    stwp_user st_newuser;
    memset(&st_newuser,0,sizeof(stwp_user));

#if 1  //按协议定义，取字段。
    //当前用户
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    
    cJSON *puser = cJSON_GetObjectItem(json, "c_user");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(c_user, puser->valuestring);
    }
    else {
        goto error;
    }
    cJSON *ppwd = cJSON_GetObjectItem(json, "c_pwd");
    if(ppwd && (ppwd->type ==cJSON_String)){
        strcpy(c_pwd, ppwd->valuestring);
    }
    else{
        goto error;
    } 

    //新建用户信息
    cJSON *pname = cJSON_GetObjectItem(json, "name");
    if(pname && (pname->type ==cJSON_String)){
        strcpy(st_newuser.name, pname->valuestring);
    }
    else {
        goto error;
    }
    cJSON *pnewaccount = cJSON_GetObjectItem(json, "account");
    if(pnewaccount && (pnewaccount->type ==cJSON_String)){
        strcpy(st_newuser.account, pnewaccount->valuestring);
    }
    else {
        goto error;
    }
      cJSON *ppasswd= cJSON_GetObjectItem(json, "passwd");
    if(ppasswd && (ppasswd->type ==cJSON_String)){
        strcpy(password, ppasswd->valuestring);
    }
    else {
        goto error;
    }
    cJSON *ppermision = cJSON_GetObjectItem(json, "permision");
    if(ppermision && (ppermision->type ==cJSON_Number)){
        st_newuser.privilege = ppermision->valueint;
    }
    else{
        goto error;
    }
    cJSON *pphone = cJSON_GetObjectItem(json, "phone");
    if(pphone && (pphone->type ==cJSON_String)){
        strcpy(st_newuser.phone, pphone->valuestring);
    }    
    cJSON *pmail = cJSON_GetObjectItem(json, "mail");
    if(pmail && (pmail->type ==cJSON_String)){
        strcpy(st_newuser.email, pmail->valuestring);
    }      
 
  
#endif
#if 1

    if( type == STWP_VALUE_TYPE_AddUser)
    {
        
        write_ui_audit(STWP_AUDIT_TYPE_CreateUser,c_user," ","create user");
        
        //加载当前用户
        if(stwp_mysql_loaduser_byaccount(c_user,&stwpuser)){
            errcode = STWP_VALUE_TYPE_ErrData;
            goto error; 
        };
        //验证当前用户的密码
        stwp_util_get_encpwd(c_pwd,stwpuser.password_salt,encpwd);
        if( strcmp(encpwd,stwpuser.password) !=0)
        {   //密码比对不通过，
            stwpuser.pwd_rat += 1;
            if(stwpuser.pwd_rat >= stwpuser.pwd_mat){
                stwpuser.status = STWP_DB_USER_STS_LOCKED;               
            }
            errcode = STWP_VALUE_TYPE_ErrPWD;  
            stwp_mysql_updateuser(&stwpuser);          
            goto error;
        }
        else{
            stwpuser.pwd_rat =0;
            stwp_mysql_updateuser(&stwpuser);
        }

        //判断新添加的用户是否存在
        memset(&stwpuser,0,sizeof(stwp_user));
        if(stwp_mysql_loaduser_byaccount(st_newuser.account,&stwpuser)){
            errcode = STWP_VALUE_TYPE_ErrData;
            goto error; 
        };
        if(strcmp(stwpuser.account,st_newuser.account)==0){
            //用户已存在
            errcode = STWP_VALUE_TYPE_ErrUserName;
            goto error; 
        }
         
        //uuid
        stwp_util_get_uuid(st_newuser.uuid);
        printf("***new uuid %s \n",st_newuser.uuid);
        //Status
        st_newuser.status = STWP_DB_USER_STS_UNACTIVE;
        //passwd passwd_salt
        memset(st_newuser.password_salt,0,sizeof(st_newuser.password_salt));
        memset(st_newuser.password,0,sizeof(st_newuser.password));
        stwp_util_gen_salt(st_newuser.password_salt);
        stwp_util_get_encpwd(password,st_newuser.password_salt,st_newuser.password);      
        //create time
        st_newuser.create_time = stwp_util_get_time();
        st_newuser.pwd_exp_time = stwp_util_get_time() +  2592000;
        st_newuser.pwd_update_time = stwp_util_get_time();
             
        if(stwp_mysql_saveuser(&st_newuser))
        {
            errcode = STWP_VALUE_TYPE_ErrData;            
            goto error;
        }
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

//处理UI2发来的修改有户信息的请求
int doOpUserModify(cJSON *json,int socket_fd)
{
/** json
    {
    "type":0 ,
    “c_user”："" //当前登录用户， 
    "c_pwd" : ""  //当前用户的密码
    “uuid”:”xxxx”,//(修改的用户的UUID)
    
    “account”:""// 被操作用户的account
    “name”：”xxx”,
    “permision”：0，
    “phone”:”xxxx”,
    “mail”:”xxxxx”
    }
*/
    int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;

    int type =0, lock = -1;
   
    char c_user[MINI_LENGTH] = {0x00};
    char c_pwd[MINI_LENGTH] = {0x00};
    char encpwd[MID_LENGTH]={0x00};
    
    char uuid[MID_LENGTH] = {0x00};     
    char new_account[MID_LENGTH]={0x00};
    stwp_user stwpuser;
    memset(&stwpuser,0,sizeof(stwp_user));
    stwp_user st_newuser;
    memset(&st_newuser,0,sizeof(stwp_user));

#if 1  //按协议定义，取字段。
    //当前用户
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    
    cJSON *puser = cJSON_GetObjectItem(json, "c_user");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(c_user, puser->valuestring);
    }
    else {
        goto error;
    }
    cJSON *ppwd = cJSON_GetObjectItem(json, "c_pwd");
    if(ppwd && (ppwd->type ==cJSON_String)){
        strcpy(c_pwd, ppwd->valuestring);
    }
    else{
        goto error;
    } 

    //新建用户信息
 
    cJSON *puuid = cJSON_GetObjectItem(json, "uuid");
    if(puuid && (puuid->type ==cJSON_String)){
        if(strlen(puuid->valuestring)== 0 )
            goto error;
        if(stwp_mysql_loaduser_byuuid(puuid->valuestring,&st_newuser)){
            errcode = STWP_VALUE_TYPE_ErrData;
            goto error; 
        };
        if(strcmp(st_newuser.uuid , puuid->valuestring) != 0)
        {
            errcode = STWP_VALUE_TYPE_UserNotExist;
            goto error;
        }
    }
    else {
        goto error;
    }

    cJSON *pnewaccount = cJSON_GetObjectItem(json, "account");
    if(pnewaccount && (pnewaccount->type ==cJSON_String)){
        strcpy(new_account, pnewaccount->valuestring);   
        if(stwp_mysql_loaduser_byaccount(new_account,&stwpuser)){
            errcode = STWP_VALUE_TYPE_ErrData;
            goto error; 
        };  
        strcpy(st_newuser.account,new_account);  
    }

   
    if(strcmp(stwpuser.account, st_newuser.account) == 0  && strcmp(stwpuser.uuid, st_newuser.uuid) != 0 )
    {//新修改的account已经存在
        errcode = STWP_VALUE_TYPE_ErrUserName;
        goto error;
    }
    memset(&stwpuser,0,sizeof(stwp_user));   
     
    cJSON *pname = cJSON_GetObjectItem(json, "name");
    if(pname && (pname->type ==cJSON_String)){
        memset(st_newuser.name,0,sizeof(st_newuser.name));
        strcpy(st_newuser.name, pname->valuestring);
    }
   
    cJSON *ppermision = cJSON_GetObjectItem(json, "permision");
    if(ppermision && (ppermision->type ==cJSON_Number)){
        st_newuser.privilege = ppermision->valueint;
    }
    cJSON *pphone = cJSON_GetObjectItem(json, "phone");
    if(pphone && (pphone->type ==cJSON_String)){
         memset(st_newuser.phone,0,sizeof(st_newuser.phone));
        strcpy(st_newuser.phone, pphone->valuestring);
    }    
    cJSON *pmail = cJSON_GetObjectItem(json, "mail");
    if(pmail && (pmail->type ==cJSON_String)){
         memset(st_newuser.email,0,sizeof(st_newuser.email));
        strcpy(st_newuser.email, pmail->valuestring);
    }      
 
  
#endif

#if 1

    if( type == STWP_VALUE_TYPE_ModifyUser)
    {
        //TODO: *** 所用用户操作，需要区分执行操作的用户和被操作的用户
        write_ui_audit(STWP_AUDIT_TYPE_ModifyUser,c_user," ","modify user");
        
        stwp_user stwpuser;
        memset(&stwpuser,0,sizeof(stwp_user));
        if(stwp_mysql_loaduser_byaccount(c_user,&stwpuser)){
            errcode = STWP_VALUE_TYPE_ErrData;
            goto error; 
        };
         //1. 判断操作用户是否有权限

        //2. 判断被修改用户是否存在
        if(strcmp(stwpuser.account,c_user) !=0){
            //认为没有此用户
            errcode = STWP_VALUE_TYPE_UserNotExist;
            goto error; 
        }
        
        //3. 验证Old password，如果错误，则次数加1，如果次数到了则锁
        if(stwpuser.status == STWP_DB_USER_STS_LOCKED)
        {//用户被锁定
            errcode = STWP_VALUE_TYPE_UserLocked;
            goto error;
        }

        stwp_util_get_encpwd(c_pwd,stwpuser.password_salt,encpwd);
        if( strcmp(encpwd,stwpuser.password) !=0)
        {   //密码比对不通过，
            stwpuser.pwd_rat += 1;
            if(stwpuser.pwd_rat >= stwpuser.pwd_mat){
                stwpuser.status = 99;
            }
            errcode = STWP_VALUE_TYPE_ErrPWD;  
            stwp_mysql_updateuser(&stwpuser);          
            goto error;
        }

        st_newuser.pwd_update_time= stwp_util_get_time();
        if(stwp_mysql_updateuser(&st_newuser))
        {
            errcode = STWP_VALUE_TYPE_ErrData;            
            goto error;
        }

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


//处理UI2发来的删除有户信息的请求
int  doOpUserDelete(cJSON *json,int socket_fd)
{
        
    int ret = 0;
    int errcode =STWP_VALUE_TYPE_ErrData;

    int type =0;

    char user[MINI_LENGTH] = {0x00};
    char uuid[MID_LENGTH] = {0x00};

    char password[MINI_LENGTH] = {0x00};
    char salt[MID_LENGTH]={0x00};
    char encpwd[MID_LENGTH]={0x00};

 
    #if 1  //按协议定义，取字段。
    cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    } 
    cJSON *puser = cJSON_GetObjectItem(json, "c_user");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(user, puser->valuestring);
    }
    else {
        goto error;
    }
    cJSON *ppwd = cJSON_GetObjectItem(json, "c_pwd");
    if(ppwd && (ppwd->type ==cJSON_String)){
        strcpy(password, ppwd->valuestring);
    }
    else{
        goto error;
    }   
 
    #endif
    #if 1
    if( type == STWP_VALUE_TYPE_DeleteUser )
    {
        //TODO: *** 所用用户操作，需要区分执行操作的用户和被操作的用户
        write_ui_audit(STWP_AUDIT_TYPE_DeleteUser,user," ","delete user");
        
        //判断当前用户的密码
        stwp_user stwpuser;
        memset(&stwpuser,0,sizeof(stwp_user));
        if(stwp_mysql_loaduser_byaccount(user,&stwpuser)){
            errcode = STWP_VALUE_TYPE_ErrData;
            goto error; 
        };
        //TODO:1. 判断操作用户是否有权限

        //判断被修改用户是否存在
        if(strcmp(stwpuser.account,user)!=0){
            //认为没有此用户
            errcode = STWP_VALUE_TYPE_UserNotExist;
            goto error; 
        }
        
        //3. 验证 password，如果错误，则次数加1，如果次数到了则锁
        if(stwpuser.status == 99)
        {//用户被锁定
            errcode = STWP_VALUE_TYPE_UserLocked;
            goto error;
        }
        stwp_util_get_encpwd(password,stwpuser.password_salt,encpwd);
        if( strcmp(encpwd,stwpuser.password) !=0)
        {   //密码比对不通过，
            stwpuser.pwd_rat += 1;
            if(stwpuser.pwd_rat >= stwpuser.pwd_mat){
                stwpuser.status = 99;
            }
            errcode = STWP_VALUE_TYPE_ErrPWD;  
            stwp_mysql_updateuser(&stwpuser);          
            goto error;
        }
        if( stwpuser.pwd_rat != 0 ){
            stwpuser.pwd_rat = 0;//密码验证过了，如果已有错误次数，要清空
            if(stwp_mysql_updateuser(&stwpuser)){
                errcode = STWP_VALUE_TYPE_ErrData;            
                goto error;
            }
        }
               
        cJSON *puuid = cJSON_GetObjectItem(json, "uuid");
        if( puuid != NULL )
        {
            int  data_size   = cJSON_GetArraySize ( puuid );
            int i= 0; 
            for( i = 0 ; i < data_size ; i ++ ){
                cJSON * puuidsub = cJSON_GetArrayItem(puuid, i);
                if(NULL == puuidsub ){ continue ; }
                
                memset(uuid,0,sizeof(uuid));
                strcpy(uuid,puuidsub->valuestring);  
                stwp_user st_deluser;
                memset(&st_deluser,0,sizeof(stwp_user));
                stwp_mysql_loaduser_byuuid(uuid,&st_deluser);
                if(strcmp(st_deluser.uuid,uuid) == 0){
                    //被删除的用户存在,置删除状态和删除时间
                    st_deluser.status =STWP_DB_USER_STS_DELETED;
                    st_deluser.delete_time = stwp_util_get_time();
                    stwp_mysql_updateuser(&st_deluser);  
                }
            }
            errcode = STWP_VALUE_TYPE_ErrSucceed;            
        }

         
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

//处理UI2发来的用户修改密码请求
int doOpUserChangePWD(cJSON *json,int socket_fd)
{
    /** JSON
{
"type":0 ,
“c_user”：“”，//当前操作的用户  
 "c_pwd" :当前用户的密码
 “uuid”：“”，被修改用户的UUID
  “newpwd”：“”被修改用户的新密码
}    **/

     
    int errcode  = STWP_VALUE_TYPE_ErrData;
    int type =0;
    char user[MINI_LENGTH] = {0x00};
    char c_pwd[MID_LENGTH] = {0x00};
    char uuid[MID_LENGTH] = {0x00};
    char newpwd[MINI_LENGTH] = {0x00};
    char salt[MID_LENGTH]={0x00};
    char encpwd[MID_LENGTH]={0x00}; 
    

#if 1  //按协议定义，取字段。
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }
    cJSON *puser = cJSON_GetObjectItem(json, "c_user");
    if(puser && (puser->type ==cJSON_String)){
        strcpy(user, puser->valuestring);
    }
    else {
        goto error;
    }
    
    cJSON *pc_pwd = cJSON_GetObjectItem(json, "c_pwd");
    if(pc_pwd && (pc_pwd->type ==cJSON_String)){
        strcpy(c_pwd, pc_pwd->valuestring);
    }    
    
    
    cJSON *puuid = cJSON_GetObjectItem(json, "uuid");
    if(puuid && (puuid->type ==cJSON_String)){
        strcpy(uuid, puuid->valuestring);
    }
    else {
        goto error;
    }
    cJSON *pnewpwd = cJSON_GetObjectItem(json, "newpwd");
    if(pnewpwd && (pnewpwd->type ==cJSON_String)){
        strcpy(newpwd, pnewpwd->valuestring);
    }
    else{
        goto error;
    }   



  
  
#endif
#if 1
    if( type == STWP_VALUE_TYPE_ChangePassword )
    {
       
      
        write_ui_audit(STWP_AUDIT_TYPE_ChangePwd,user," ","change passwrod");
         //1.验当前用户是否存在，再验密码
        stwp_user stwpuser;
        memset(&stwpuser,0,sizeof(stwp_user));
        if(stwp_mysql_loaduser_byaccount(user,&stwpuser)){
            errcode = STWP_VALUE_TYPE_ErrData;
            goto error; 
        };
        if(strcmp(stwpuser.account,user)!=0){
            //认为没有此用户
            errcode = STWP_VALUE_TYPE_UserNotExist;
            goto error; 
        }
        if(stwpuser.status == STWP_DB_USER_STS_LOCKED)
        {//用户被锁定
            errcode = STWP_VALUE_TYPE_UserLocked;
            goto error;
        }
       
        // 验证当前用户的密码
        memset(encpwd,0,sizeof(encpwd));
        stwp_util_get_encpwd(c_pwd,stwpuser.password_salt,encpwd);
        if( strcmp(encpwd,stwpuser.password) !=0)
        {   //密码比对不通过，
            stwpuser.pwd_rat += 1;
            if(stwpuser.pwd_rat >= stwpuser.pwd_mat){
                stwpuser.status = STWP_DB_USER_STS_LOCKED;
               
            }
            errcode = STWP_VALUE_TYPE_ErrPWD;  
            stwp_mysql_updateuser(&stwpuser);          
            goto error;
        }
        else{
            stwpuser.pwd_rat =0;
            stwp_mysql_updateuser(&stwpuser);
        }

        //加载被修改的用户
        memset(&stwpuser,0,sizeof(stwp_user));
        if(stwp_mysql_loaduser_byuuid(uuid,&stwpuser)){
            errcode = STWP_VALUE_TYPE_ErrData;
            goto error; 
        };
        if(strcmp(stwpuser.uuid,uuid)!=0){
            //认为没有此用户
            errcode = STWP_VALUE_TYPE_UserNotExist;
            goto error; 
        }

        if(strcmp(stwpuser.account,user) == 0)
        {//当前用户和被修改用户相同，即自己在修改自己的密码，要判断密码的历史信息

            memset(encpwd,0,sizeof(encpwd));
            stwp_util_get_encpwd(newpwd,stwpuser.password_salt,encpwd);
            if(strcmp(encpwd,stwpuser.password) == 0)
            {//新密码与当前密码一致
                errcode = STWP_VALUE_TYPE_ErrPWDRepeat;            
                goto error;
            }
            memset(encpwd,0,sizeof(encpwd));
            stwp_util_get_encpwd(newpwd,stwpuser.history_pwd_salt,encpwd);
            if(strcmp(encpwd,stwpuser.history_pwd) == 0)
            {//新密码与上次使用的密码一致
                errcode = STWP_VALUE_TYPE_ErrPWDRepeat;            
                goto error;
            }

            //保存旧密码
            strcpy(stwpuser.history_pwd,stwpuser.password);
            strcpy(stwpuser.history_pwd_salt,stwpuser.password_salt);
            //修改时间
            stwpuser.pwd_update_time = stwp_util_get_time();
            //过期时间
            stwpuser.pwd_exp_time = stwpuser.pwd_update_time + 2592000; //30天的秒数

            //保存新密码
            memset(stwpuser.password_salt,0,sizeof(stwpuser.password_salt));
            memset(stwpuser.password,0,sizeof(stwpuser.password));
            stwp_util_gen_salt(stwpuser.password_salt);
            stwp_util_get_encpwd(newpwd,stwpuser.password_salt,stwpuser.password);
        
            stwpuser.status = STWP_DB_USER_STS_ACTIVE;
            if(stwp_mysql_updateuser(&stwpuser))
            {
                errcode = STWP_VALUE_TYPE_ErrData;            
                goto error;
            }

            errcode = STWP_VALUE_TYPE_ErrSucceed;            


        }
        else{

            //被修改用户的新密码
            memset(stwpuser.password_salt,0,sizeof(stwpuser.password_salt));
            stwp_util_gen_salt(stwpuser.password_salt);
            memset(stwpuser.password,0,sizeof(stwpuser.password));           
            stwp_util_get_encpwd(newpwd,stwpuser.password_salt,stwpuser.password);   

            //被修改用户的旧密码清零
            memset(stwpuser.history_pwd,0,sizeof(stwpuser.history_pwd));
            memset(stwpuser.history_pwd_salt,0,sizeof(stwpuser.history_pwd_salt));
            //修改时间
            stwpuser.pwd_update_time = stwp_util_get_time();
            //过期时间
            stwpuser.pwd_exp_time = stwpuser.pwd_update_time + 2592000; //30天的秒数
      
            stwpuser.status = STWP_DB_USER_STS_ACTIVE;
            if(stwp_mysql_updateuser(&stwpuser))
            {
                errcode = STWP_VALUE_TYPE_ErrData;            
                goto error;
            }

            errcode = STWP_VALUE_TYPE_ErrSucceed;           

        }
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


//给UI2回应应答，login
int _sendback_login(int type,int socket_fd,int errcode,char* uuid,int priv)
{
    char *outdata;
    cJSON *json;
   
    int ret = 0;
    json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "type", type);
    cJSON_AddNumberToObject(json, "errcode", errcode);
    cJSON_AddNumberToObject(json, "count", 0);
    cJSON_AddNumberToObject(json, "permissions", priv);
    cJSON_AddStringToObject(json, "uuid", uuid);
    outdata=cJSON_Print(json);
    int nlen = strlen(outdata);
    send(socket_fd,&nlen,sizeof(int),0);
    printf("send back :%s \n",outdata);
    ret = send(socket_fd,outdata,nlen,0);
    free(outdata);
    cJSON_Delete(json);
    
    return 0;
}

//处理UI2发来的用户登录验证的请求
int doOpUserCheckLogin(cJSON *json,int socket_fd)
{
    /**
    {
    "type":0 ,
    “user”：“”， //输入的用户名（审计也需要）
    “passwd”：“”
    }
     */
    int errcode=STWP_VALUE_TYPE_ErrData;
    int type =0;
    char user[MINI_LENGTH] = {0x00};
    char password[MINI_LENGTH] = {0x00};
    char salt[MID_LENGTH]={0x00};
    char encpwd[MID_LENGTH]={0x00};
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
        goto error;
    }
    cJSON *ppwd = cJSON_GetObjectItem(json, "passwd");
    if(ppwd && (ppwd->type ==cJSON_String)){
        strcpy(password, ppwd->valuestring);
    }
    else{
        goto error;
    }   
    
   
#endif
#if 1
    if( type == STWP_VALUE_TYPE_VerifyUser )
    {
        //TODO: *** 所用用户操作，需要区分执行操作的用户和被操作的用户
        write_ui_audit(STWP_AUDIT_TYPE_Login,user," ","user login");
        
        stwp_user stwpuser;
        memset(&stwpuser,0,sizeof(stwp_user));
        stwp_mysql_loaduser_byaccount(user,&stwpuser);
        if( strcmp(stwpuser.account,user) != 0)
        {
            errcode = STWP_VALUE_TYPE_UserNotExist;
            goto error;
        }

        stwp_util_get_encpwd(password,stwpuser.password_salt,encpwd);
        if(strcmp(encpwd,stwpuser.password))
        {   //密码比对不通过，
            stwpuser.pwd_rat += 1;
            if(stwpuser.pwd_rat >= stwpuser.pwd_mat){
                stwpuser.status = 99;
            }
            errcode = STWP_VALUE_TYPE_ErrPWD;  
            stwp_mysql_updateuser(&stwpuser);          
            goto error;
        }
        else
        {//错误次数清零
            stwpuser.pwd_rat = 0;
            stwp_mysql_updateuser(&stwpuser);
        }
        stwp_session sess;
        sess.sess_status = 1;
        strncpy(sess.cur_user,user,sizeof(sess.cur_user));
        stwp_session_set(&sess );
        errcode = STWP_VALUE_TYPE_ErrSucceed;  

        return _sendback_login(type,socket_fd,errcode,stwpuser.uuid,stwpuser.privilege)  ;
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

#if 0
//处理UI2发来的用户添加、删除、修改信息请求
int doOpUser(cJSON *json,int socket_fd)
{
    int ret = 0;
    int type =0;
    int errcode = STWP_VALUE_TYPE_ErrData;
  //按协议定义，取字段。
     cJSON *ptype = cJSON_GetObjectItem(json, "type");
    if(ptype&& ptype->type == cJSON_Number){
        type = ptype->valueint;
    }
    else{
        goto error;
    }

    if(type == STWP_VALUE_TYPE_AddUser){
        return doOpUserAddNew(json,socket_fd);
    }
    else if(type == STWP_VALUE_TYPE_DeleteUser){
        return doOpUserDelete(json,socket_fd);
    }
    else if(type == STWP_VALUE_TYPE_ModifyUser){
        return doOpUserModify(json,socket_fd);
    }
    else if(type == STWP_VALUE_TYPE_VerifyUser){
        return doOpUserCheckLogin(json,socket_fd);
    }
    else if(type == STWP_VALUE_TYPE_ChangePassword){
        return doOpUserChangePWD(json,socket_fd);
    }
    else 
        goto error;

    
error:
   stwp_data_error_for_ui(type,socket_fd);
return -1;

}
#endif

int getUserPrivilege(char * user_account)
{
    stwp_user stwpuser;
    memset(&stwpuser,0,sizeof(stwp_user));
    if(stwp_mysql_loaduser_byaccount(user_account,&stwpuser)){
        return -1;
    };
    return 0;//stwpuser.permissions
}
