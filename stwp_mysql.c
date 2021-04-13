#include <stdio.h>
#include <mysql/mysql.h>
#include "stwp_config.h"
#include "stwp_define.h"
#include "stwp_mysql.h"
#include "cJSON.h"
#include "stwp_logdump.h"

stwp_mysql_t stwp_mysql;

int stwp_mysql_init(void)
{    
    stwp_mysql.conn = mysql_init(NULL);
    if(!mysql_real_connect(stwp_mysql.conn ,SERVER,USER,PASSWD,DATABASE,0,NULL,0))
    {
        stwp_logdump_module.push("[STWP_MYSQL] cant conect to %s %s %s:%d:%s",SERVER,USER,DATABASE,mysql_errno(stwp_mysql.conn),mysql_error(stwp_mysql.conn));
        goto error;
        
    }
    stwp_logdump_module.push("[STWP_MYSQL] conect to %s %s %s",SERVER,USER,DATABASE);
  return 0;
error:
    mysql_close(stwp_mysql.conn);
    return -1;
    
}

int stwp_mysql_close(void)
{
    if(stwp_mysql.conn)
        mysql_close(stwp_mysql.conn);
    stwp_mysql.conn = NULL;
    return 0;
}
/**
 * 输入查询Sql语句，返回JSON格式数据
 * @param sql : 待执行的SQL语句
 * @param rtype ：待返回给UI2的type类型值
 * @param outdata:JSON数据，注意长度
 * 
 * 
 {
    “type”：0，
    “errcode”:0,
	“count”：2，//返回数据库数目
    “data”：[   
    {
      "字段名" ：value,
      "字段名" : value      
    },
    {
      "字段名" ：value,
      "字段名" : value      
    }
  ]
}
 */
int stwp_mysql_select(char *sql,char* sql2,int rtype,char* outdata)
 {
    cJSON *json;
    int err_code = 1;
    int ret = 0;
    int rows_cnt = 0; //记录条数
    int field_cnt= 0; //每条记录有几列
    int count = 0;
    if(strlen(sql2))
        count = stwp_mysql_select_cnt(sql2);
    json = cJSON_CreateObject();
    cJSON *JsonArray = cJSON_CreateArray();

    cJSON_AddNumberToObject(json, "type", rtype);
    
    ret = mysql_query(stwp_mysql.conn, sql);
     if (ret)
     {
         stwp_logdump_module.push("[STWP_MYSQL]  %s : %s",sql,mysql_error(stwp_mysql.conn));
         cJSON_AddNumberToObject(json, "errcode", 1);
         cJSON_AddNumberToObject(json, "count", 0);
         cJSON *ArrayItem = cJSON_CreateObject();  
         cJSON_AddItemToObject(json, "data", JsonArray);
     }
     else
     {
        cJSON_AddNumberToObject(json, "errcode", 0);
        stwp_mysql.res = mysql_store_result(stwp_mysql.conn);//mysql_use_result(stwp_mysql.conn);
        field_cnt = mysql_num_fields(stwp_mysql.res);  //取表的列总数
        rows_cnt =  mysql_num_rows(stwp_mysql.res);  //取表内数据记录条数.如果用mysql_use_result该值不正确
    
        stwp_mysql.fields = mysql_fetch_fields(stwp_mysql.res); //取列名(即表的字段名称)

        //printf( "rows = %d \n",rows_cnt);

        while(stwp_mysql.row = mysql_fetch_row(stwp_mysql.res))
        {    
            cJSON *ArrayItem = cJSON_CreateObject();    
            int i = 0 ;
            for(i = 0 ; i < field_cnt; i++)
            {
                //printf("%s \t:%s\n",stwp_mysql.fields[i].name,stwp_mysql.row[i]);
                //TODO:判断类型
               // printf("%s-%s | ",stwp_mysql.fields[i].name, stwp_mysql.row[i]);
                if(stwp_mysql.row[i])
                    cJSON_AddStringToObject(ArrayItem, stwp_mysql.fields[i].name, stwp_mysql.row[i]);
                else
                    cJSON_AddStringToObject(ArrayItem, stwp_mysql.fields[i].name, "");
            }
            printf("\n");
            
            cJSON_AddItemToArray(JsonArray, ArrayItem);     
        }
        if(strlen(sql2))
            cJSON_AddNumberToObject(json, "count", count);  
        else
            cJSON_AddNumberToObject(json, "count", rows_cnt); 
        cJSON_AddItemToObject(json, "data", JsonArray);
        mysql_free_result(stwp_mysql.res);
    
     }   
  
    
    strcpy(outdata,cJSON_Print(json));
    //printf("outdata %s \n",outdata);                         
    cJSON_Delete(json); 
   
    return ret;
 }

int stwp_mysql_select_cnt(char *sql)
 {
     int ncount = 0;
    if (mysql_query(stwp_mysql.conn, sql))
    {
        stwp_logdump_module.push("[STWP_MYSQL]  %s : %s",sql,mysql_error(stwp_mysql.conn));
        return 0;
    }
    else
    {
        stwp_mysql.res = mysql_store_result(stwp_mysql.conn);//mysql_use_result(stwp_mysql.conn);
        while(stwp_mysql.row = mysql_fetch_row(stwp_mysql.res))
        {    
           ncount = atoi(stwp_mysql.row[0]);
      
        }
            
        return  ncount;
    }     
    return 0;
 }

int stwp_mysql_write(char *sql,int rtype,char* outdata)
 {
    cJSON *json;
    int err_code = 0;
    int ret = 0;
    
    json = cJSON_CreateObject();
    cJSON *JsonArray = cJSON_CreateArray();
     cJSON_AddNumberToObject(json, "type", rtype);
     cJSON_AddNumberToObject(json, "count", 0);
    ret = mysql_query(stwp_mysql.conn, sql);

     if ( ret )
     {
         stwp_logdump_module.push("[STWP_MYSQL]  %s : %s",sql,mysql_error(stwp_mysql.conn));
        cJSON_AddNumberToObject(json, "errcode", 1);
     }
     else
     {
        cJSON_AddNumberToObject(json, "errcode", 0);
     }
     
   
    cJSON *ArrayItem = cJSON_CreateObject();  
    cJSON_AddItemToObject(json, "data", JsonArray);      
    strcpy(outdata,cJSON_Print(json));
    printf("outdata %s \n",outdata);                         
    cJSON_Delete(json); 
  
    return ret;
 }

int stwp_get_schema(char *table_name)
{
    char sql[1024] = {0};
    int num = 0;
    sprintf(sql,"%s'%s';",SCHEMA_GET,table_name);
    stwp_logdump_module.push("[STWP_MYSQl] get schema sql = %s",sql);
    if (mysql_query(stwp_mysql.conn, sql))
    {
        printf("get data error\n");
       
    }

    stwp_mysql.res = mysql_use_result(stwp_mysql.conn);

    while(stwp_mysql.row = mysql_fetch_row(stwp_mysql.res))
    {
        int i = 0;
        num = atoi(stwp_mysql.row[i]);
    }                                                                           
       
    mysql_free_result(stwp_mysql.res);

    return num;
}

void print_user(stwp_user* stwpuser)
{
    printf("------------------------------------\n");
    printf("uuid                %s\n",stwpuser->uuid);
    printf("account             %s\n",stwpuser->account);
    printf("name                %s\n",stwpuser->name);
    printf("password            %s\n",stwpuser->password);
    printf("password_salt       %s\n",stwpuser->password_salt);
    printf("historypwd          %s\n",stwpuser->history_pwd);
    printf("historypwdsalt      %s\n",stwpuser->history_pwd_salt);
    printf("phone               %s\n",stwpuser->phone);
    printf("email               %s\n",stwpuser->email);
    printf("address             %s\n",stwpuser->address);
    printf("permission          %s\n",stwpuser->permissions);
    printf("ctime        %lu\n",stwpuser->create_time);
    printf("dtime        %lu\n",stwpuser->delete_time);
    printf("utime        %lu\n",stwpuser->pwd_update_time);
    printf("etime        %lu\n",stwpuser->pwd_exp_time);
    printf("pwd_mat        %d\n",stwpuser->pwd_mat);
    printf("pwd_rat        %d\n",stwpuser->pwd_rat);
    printf("status         %d\n",stwpuser->status);
    printf("privilege         %d\n",stwpuser->privilege);
    printf("------------------------------------\n");

}
int stwp_mysql_loaduser(char *uuid,stwp_user* stwpuser)
{

    int err_code = 1;
    int ret = 0;
    int rows_cnt = 0; //记录条数
    int field_cnt= 0; //每条记录有几列
    char sql[1024] = {0x00};

    sprintf(sql,"select uuid, account,password,password_salt,\
    histroy_password,histroy_password_salt,pwd_mat,pwd_rat,status,name,address,email,phone,description,permissions,\
    unix_timestamp(pwd_update_time) as pwd_update_time,\
    unix_timestamp(pwd_expire_time) as pwd_expire_time,\
    unix_timestamp(create_time) as create_time,\
    unix_timestamp(delete_time) as delete_time\
    from stwp_users where uuid = \'%s\';",uuid);

    printf("loaduser sql = %s \n",sql);
    ret = mysql_query(stwp_mysql.conn, sql);
     if (ret)
     {
        stwp_logdump_module.push("[STWP_MYSQL]  %s : %s",sql,mysql_error(stwp_mysql.conn));
        return -1;
     }
     else
     {
        stwp_mysql.res = mysql_store_result(stwp_mysql.conn);//mysql_use_result(stwp_mysql.conn);
        rows_cnt =  mysql_num_rows(stwp_mysql.res);  //取表内数据记录条数.如果用mysql_use_result该值不正确
        if(rows_cnt > 1)
        {//用户不唯一
            mysql_free_result(stwp_mysql.res);
            return -1;
        }

        field_cnt = mysql_num_fields(stwp_mysql.res);  //取表的列总数        
        stwp_mysql.fields = mysql_fetch_fields(stwp_mysql.res); //取列名(即表的字段名称)

        while(stwp_mysql.row = mysql_fetch_row(stwp_mysql.res))
        {    
            int i = 0 ;
            for(i = 0 ; i < field_cnt; i++)
            {
                if(!stwp_mysql.row[i])
                    continue;
                if(strcmp(stwp_mysql.fields[i].name,"uuid") == 0){
                    strcpy(stwpuser->uuid, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"account") == 0){
                    strcpy(stwpuser->account, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"password") == 0){
                    strcpy(stwpuser->password, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"password_salt") == 0){
                    strcpy(stwpuser->password_salt, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"histroy_password") == 0){
                    strcpy(stwpuser->history_pwd, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"histroy_password_salt") == 0){
                    strcpy(stwpuser->history_pwd_salt, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_mat") == 0){
                    stwpuser->pwd_mat = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_rat") == 0){
                    stwpuser->pwd_rat = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"status") == 0){
                    stwpuser->status= atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"name") == 0){
                    strcpy(stwpuser->name, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"address") == 0){
                    strcpy(stwpuser->address, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"email") == 0){
                    strcpy(stwpuser->email, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"phone") == 0){
                    strcpy(stwpuser->phone, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"description") == 0){
                    strcpy(stwpuser->description, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"permissions") == 0){
                    strcpy(stwpuser->permissions, stwp_mysql.row[i]);
                    stwpuser->privilege = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_update_time") == 0){
                    stwpuser->pwd_update_time = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_expire_time") == 0){
                    stwpuser->pwd_update_time = atoi(stwp_mysql.row[i]);
                }                
                else if(strcmp(stwp_mysql.fields[i].name,"create_time") == 0){
                    stwpuser->create_time = atoi(stwp_mysql.row[i]);
                }                
                else if(strcmp(stwp_mysql.fields[i].name,"delete_time") == 0){
                    stwpuser->delete_time = atoi(stwp_mysql.row[i]);
                }
                else
                    continue;          
            }

        }
        mysql_free_result(stwp_mysql.res);
    
     }   
     print_user(stwpuser);
    return ret;
 
}

int stwp_mysql_loaduser_byaccount(char *account,stwp_user* stwpuser)
{

    int err_code = 1;
    int ret = 0;
    int rows_cnt = 0; //记录条数
    int field_cnt= 0; //每条记录有几列
    char sql[1024] = {0x00};

    sprintf(sql,"select uuid, account,password,password_salt,\
    histroy_password,histroy_password_salt,pwd_mat,pwd_rat,status,name,address,email,phone,description,permissions,\
    unix_timestamp(pwd_update_time) as pwd_update_time,\
    unix_timestamp(pwd_expire_time) as pwd_expire_time,\
    unix_timestamp(create_time) as create_time,\
    unix_timestamp(delete_time) as delete_time\
    from stwp_users where account = \'%s\';",account);

    printf("loaduser sql = %s \n",sql);
    ret = mysql_query(stwp_mysql.conn, sql);
     if (ret)
     {
        stwp_logdump_module.push("[STWP_MYSQL]  %s : %s",sql,mysql_error(stwp_mysql.conn));
        return -1;
     }
     else
     {
        stwp_mysql.res = mysql_store_result(stwp_mysql.conn);//mysql_use_result(stwp_mysql.conn);
        rows_cnt =  mysql_num_rows(stwp_mysql.res);  //取表内数据记录条数.如果用mysql_use_result该值不正确
        if(rows_cnt>1)
        {//用户不唯一
            mysql_free_result(stwp_mysql.res);
            return -1;
        }

        field_cnt = mysql_num_fields(stwp_mysql.res);  //取表的列总数        
        stwp_mysql.fields = mysql_fetch_fields(stwp_mysql.res); //取列名(即表的字段名称)

        while(stwp_mysql.row = mysql_fetch_row(stwp_mysql.res))
        {    
            int i = 0 ;
            for(i = 0 ; i < field_cnt; i++)
            {
                if(!stwp_mysql.row[i])
                    continue;
                if(strcmp(stwp_mysql.fields[i].name,"uuid") == 0){
                  
                    strcpy(stwpuser->uuid, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"account") == 0){
                    strcpy(stwpuser->account, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"password") == 0){
                    strcpy(stwpuser->password, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"password_salt") == 0){
                    strcpy(stwpuser->password_salt, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"histroy_password") == 0){
                    strcpy(stwpuser->history_pwd, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"histroy_password_salt") == 0){
                    strcpy(stwpuser->history_pwd_salt, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_mat") == 0){
                    stwpuser->pwd_mat = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_rat") == 0){
                    stwpuser->pwd_rat = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"status") == 0){
                    stwpuser->status= atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"name") == 0){
                    strcpy(stwpuser->name, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"address") == 0){
                    strcpy(stwpuser->address, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"email") == 0){
                    strcpy(stwpuser->email, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"phone") == 0){
                    strcpy(stwpuser->phone, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"description") == 0){
                    strcpy(stwpuser->description, stwp_mysql.row[i]);
                    
                }
                else if(strcmp(stwp_mysql.fields[i].name,"permissions") == 0){
                    strcpy(stwpuser->permissions, stwp_mysql.row[i]);
                    stwpuser->privilege = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_update_time") == 0){
                    stwpuser->pwd_update_time = atoi(stwp_mysql.row[i]);
                    
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_expire_time") == 0){
                    stwpuser->pwd_update_time = atoi(stwp_mysql.row[i]);
                }                
                else if(strcmp(stwp_mysql.fields[i].name,"create_time") == 0){
                    stwpuser->create_time = atoi(stwp_mysql.row[i]);
                }                
                else if(strcmp(stwp_mysql.fields[i].name,"delete_time") == 0){
                    stwpuser->delete_time = atoi(stwp_mysql.row[i]);
                }
                else
                    continue;          
            }

        }
        mysql_free_result(stwp_mysql.res);
    
     }   
    print_user(stwpuser);
    return ret;
 
}

int stwp_mysql_loaduser_byuuid(char *uuid,stwp_user* stwpuser)
{

    int err_code = 1;
    int ret = 0;
    int rows_cnt = 0; //记录条数
    int field_cnt= 0; //每条记录有几列
    char sql[1024] = {0x00};

    sprintf(sql,"select uuid, account,password,password_salt,\
    histroy_password,histroy_password_salt,pwd_mat,pwd_rat,status,name,address,email,phone,description,permissions,\
    unix_timestamp(pwd_update_time) as pwd_update_time,\
    unix_timestamp(pwd_expire_time) as pwd_expire_time,\
    unix_timestamp(create_time) as create_time,\
    unix_timestamp(delete_time) as delete_time\
    from stwp_users where uuid = \'%s\';",uuid);

    printf("loaduser sql = %s \n",sql);
    ret = mysql_query(stwp_mysql.conn, sql);
     if (ret)
     {
        stwp_logdump_module.push("[STWP_MYSQL]  %s : %s",sql,mysql_error(stwp_mysql.conn));
        return -1;
     }
     else
     {
        stwp_mysql.res = mysql_store_result(stwp_mysql.conn);//mysql_use_result(stwp_mysql.conn);
        rows_cnt =  mysql_num_rows(stwp_mysql.res);  //取表内数据记录条数.如果用mysql_use_result该值不正确
        if(rows_cnt>1)
        {//用户不唯一
            mysql_free_result(stwp_mysql.res);
            return -1;
        }

        field_cnt = mysql_num_fields(stwp_mysql.res);  //取表的列总数        
        stwp_mysql.fields = mysql_fetch_fields(stwp_mysql.res); //取列名(即表的字段名称)

        while(stwp_mysql.row = mysql_fetch_row(stwp_mysql.res))
        {    
            int i = 0 ;
            for(i = 0 ; i < field_cnt; i++)
            {
                if(!stwp_mysql.row[i])
                    continue;
                if(strcmp(stwp_mysql.fields[i].name,"uuid") == 0){
                  
                    strcpy(stwpuser->uuid, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"account") == 0){
                    strcpy(stwpuser->account, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"password") == 0){
                    strcpy(stwpuser->password, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"password_salt") == 0){
                    strcpy(stwpuser->password_salt, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"histroy_password") == 0){
                    strcpy(stwpuser->history_pwd, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"histroy_password_salt") == 0){
                    strcpy(stwpuser->history_pwd_salt, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_mat") == 0){
                    stwpuser->pwd_mat = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_rat") == 0){
                    stwpuser->pwd_rat = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"status") == 0){
                    stwpuser->status= atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"name") == 0){
                    strcpy(stwpuser->name, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"address") == 0){
                    strcpy(stwpuser->address, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"email") == 0){
                    strcpy(stwpuser->email, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"phone") == 0){
                    strcpy(stwpuser->phone, stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"description") == 0){
                    strcpy(stwpuser->description, stwp_mysql.row[i]);
                    
                }
                else if(strcmp(stwp_mysql.fields[i].name,"permissions") == 0){
                    strcpy(stwpuser->permissions, stwp_mysql.row[i]);
                    stwpuser->privilege = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_update_time") == 0){
                    stwpuser->pwd_update_time = atoi(stwp_mysql.row[i]);
                }
                else if(strcmp(stwp_mysql.fields[i].name,"pwd_expire_time") == 0){
                    stwpuser->pwd_update_time = atoi(stwp_mysql.row[i]);
                }                
                else if(strcmp(stwp_mysql.fields[i].name,"create_time") == 0){
                    stwpuser->create_time = atoi(stwp_mysql.row[i]);
                }                
                else if(strcmp(stwp_mysql.fields[i].name,"delete_time") == 0){
                    stwpuser->delete_time = atoi(stwp_mysql.row[i]);
                }
                else
                    continue;          
            }

        }
        mysql_free_result(stwp_mysql.res);
    
     }   
    print_user(stwpuser);
    return ret;
 
}


int stwp_mysql_saveuser(stwp_user* stwpuser)
{
   
   char sql[1024] = {0x00};
    print_user(stwpuser);
    sprintf( sql, "INSERT  INTO %s \
        (uuid,name,account,password,password_salt,\
        histroy_password,histroy_password_salt,create_time,phone,email,status,\
        address, description,pwd_update_time,pwd_expire_time,delete_time,\
        permissions,pwd_mat,pwd_rat) VALUES\
        (\
        \'%s\', \'%s\',\'%s\', \'%s\',\'%s\',\
        \'%s\',\'%s\',FROM_UNIXTIME(%lu),\'%s\',\'%s\',%d,\
        \'%s\',\'%s\',FROM_UNIXTIME(%lu),FROM_UNIXTIME(%lu),FROM_UNIXTIME(%lu),\
        \'%d\',\'%d\',%d)","stwp_users",\
        stwpuser->uuid,stwpuser->name,stwpuser->account,stwpuser->password,stwpuser->password_salt,\
        stwpuser->history_pwd,stwpuser->history_pwd_salt,stwpuser->create_time,stwpuser->phone,stwpuser->email,stwpuser->status,\
        stwpuser->address,stwpuser->description,stwpuser->pwd_update_time,stwpuser->pwd_exp_time,stwpuser->delete_time,\
        stwpuser->privilege,stwpuser->pwd_mat,stwpuser->pwd_rat);
    printf("saveuser sql :%s \n",sql);
    if(mysql_query(stwp_mysql.conn, sql))
    {
        stwp_logdump_module.push("[STWP_MYSQL]  %s : %s",sql,mysql_error(stwp_mysql.conn));
        return -1;
    }
    else
    {
        return 0;

    }
     
    return 0;
 
}

int stwp_mysql_updateuser(stwp_user* stwpuser)
{
   
   char sql[1024] = {0x00};
    print_user(stwpuser);
    sprintf( sql, "UPDATE  %s SET \
        uuid = \'%s\',name= \'%s\',account= \'%s\',password= \'%s\',password_salt= \'%s\',\
        histroy_password= \'%s\',histroy_password_salt= \'%s\',create_time=FROM_UNIXTIME(%lu),phone= \'%s\',email= \'%s\',status=%d,\
        address= \'%s\', description= \'%s\',pwd_update_time=FROM_UNIXTIME(%lu),pwd_expire_time=FROM_UNIXTIME(%lu),delete_time=FROM_UNIXTIME(%lu),\
        permissions= \'%s\',pwd_mat=%d,pwd_rat=%d  where uuid=\'%s\'",\
        "stwp_users",\
        stwpuser->uuid,stwpuser->name,stwpuser->account,stwpuser->password,stwpuser->password_salt,\
        stwpuser->history_pwd,stwpuser->history_pwd_salt,stwpuser->create_time,stwpuser->phone,stwpuser->email,stwpuser->status,\
        stwpuser->address,stwpuser->description,stwpuser->pwd_update_time,stwpuser->pwd_exp_time,stwpuser->delete_time,\
        stwpuser->permissions,stwpuser->pwd_mat,stwpuser->pwd_rat,stwpuser->uuid);
    printf("updateuser sql :%s \n",sql);
    if(mysql_query(stwp_mysql.conn, sql))
    {
        stwp_logdump_module.push("[STWP_MYSQL]  %s : %s",sql,mysql_error(stwp_mysql.conn));
        return -1;
    }
    else
    {
        return 0;

    }
     
    return 0;
 
}

int stwp_mysql_export2csv(char* sql,char* file)
{

    int ret = 0;
    int rows_cnt = 0; //记录条数
    int field_cnt= 0; //每条记录有几列
    FILE *fp=NULL;
    int j=0;
    char aline[1024*2]={0x00};
    
    fp = fopen(file,"a+");
    if(!fp)
    {
        stwp_logdump_module.push("[STWP_MYSQL]  open failed : %s",file); 
        return -1;
    }
    ret = mysql_query(stwp_mysql.conn, sql);
     if (ret)
     {
        stwp_logdump_module.push("[STWP_MYSQL]  %s : %s",sql,mysql_error(stwp_mysql.conn));
        return -1;
     }
     else
     {
        
        stwp_mysql.res = mysql_store_result(stwp_mysql.conn);//mysql_use_result(stwp_mysql.conn);
        field_cnt = mysql_num_fields(stwp_mysql.res);  //取表的列总数
        rows_cnt =  mysql_num_rows(stwp_mysql.res);  //取表内数据记录条数.如果用mysql_use_result该值不正确
    
        stwp_mysql.fields = mysql_fetch_fields(stwp_mysql.res); //取列名(即表的字段名称)

        memset(aline,0,sizeof(aline));
        for(j= 0 ; j < field_cnt;j++)
        {
            if(j>0)
                strcat(aline,",");
            strcat(aline,stwp_mysql.fields[j].name);
        }
        strcat(aline,"\r\n");
        //写头
        fwrite(aline,strlen(aline),1,fp);

        while(stwp_mysql.row = mysql_fetch_row(stwp_mysql.res))
        {    
           
            memset(aline,0,sizeof(aline));
            for(j= 0 ; j < field_cnt;j++)
            {
                if(j>0)
                    strcat(aline,",");
                strcat(aline,stwp_mysql.row[j]);
            }
            strcat(aline,"\r\n");
            //写头
            fwrite(aline,strlen(aline),1,fp);         
            
        }

    
     }   
  
    fclose(fp);
    fp = NULL;
    
   
    return 0;
   

}