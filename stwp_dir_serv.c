#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "stwp_list.h"
#include "stwp_util.h"
#include "stwp_logdump.h"
#include "cJSON.h"
#include "stwp_p2.h"


//给UI2回应应答，带错误码
int sendback_to_ui2(int type,int socket_fd,int errcode)
{
    char *outdata;
    cJSON *json;
   
    int ret = 0;
    json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "type", type);
    cJSON_AddNumberToObject(json, "errcode", errcode);
    cJSON_AddNumberToObject(json, "count", 0);
    cJSON_AddStringToObject(json,"data"," ");
    outdata=cJSON_Print(json);
    int nlen = strlen(outdata);
    send(socket_fd,&nlen,sizeof(int),0);
    printf("send back :%s \n",outdata);
    ret = send(socket_fd,outdata,nlen,0);
    free(outdata);
    cJSON_Delete(json);
    
    return 0;
}




int stwp_getdir_in(char *in_file, int depth,char* pjson,int* nlen)
{

	DIR * dir = NULL;
	struct dirent *dirent = NULL;
	struct stat stat;
	size_t file_l, name_l;

	int  mode=0;
	char *name = NULL, *suffix = NULL;
    char file[1024] = {0};
    char buff[1024] = {0x00};
    int btrip_flag =0;

    if(nlen==NULL)
        return -1;

    strncpy(file, in_file, strlen(in_file));

	file_l = strlen(file);
  
   
	
	dir = opendir(file);
    if (!dir)
    {
        goto out;
    }
      
	while ((dirent = readdir(dir)) != NULL) {
		name = dirent->d_name;
		if (!strcmp(name, ".") || !strcmp(name, ".."))
			continue;
		name_l = strlen(name);
		suffix = name + name_l - 3;

		if(file_l != 1)
			strcat(file, "/");
		strcat(file, name);
		if (lstat(file, &stat) == -1) {
			memset(file + file_l, 0x0, name_l + 1);
			continue;
		}
        btrip_flag =1;
        memset(buff,0,1024);
        sprintf(buff,"{\"name\":\"%s\",\n",file);
        if(pjson)
            strcat(pjson,buff);
        else
            *nlen += strlen(buff);
		mode = stat.st_mode;
        if (S_ISDIR(mode)){
            //是否是一个目录
            memset(buff,0,1024);
            sprintf(buff,"\"type\":\"folder\",\n\"sub\":[\n");
            if(pjson)
                strcat(pjson,buff);
            else
                *nlen += strlen(buff);
            if(depth >0 )
                stwp_getdir_in(file,depth-1,pjson,nlen);
            memset(buff,0,1024);
            sprintf(buff,"]\n");
            if(pjson)
                strcat(pjson,buff);
             else
                *nlen += strlen(buff);
            
        }
		else  {
             //是否是一个常规文件.
            memset(buff,0,1024);
            sprintf(buff,"\"type\":\"file\",\n\"sub\":[]\n");
            if(pjson)
                strcat(pjson,buff);
            else
                *nlen += strlen(buff);
		}
        /*
        else if (S_ISREG(mode)){

        }
        else if(S_ISLNK(mode)){
            //是否是一个连接.
            memset(buff,0,1024);
            sprintf(buff,"\"type\":\"link\",\n\"sub\":[]\n");
            if(pjson)
                strcat(pjson,buff);
            else
                *nlen += strlen(buff);
        }
        else if(S_ISCHR(mode)){
            //是否是一个字符设备.
            memset(buff,0,1024);
            sprintf(buff,"\"type\":\"chrdev\",\n\"sub\":[]\n");
            if(pjson)
                strcat(pjson,buff);
            else
                *nlen += strlen(buff);
        }
        else if(S_ISBLK(mode)){
            //是否是一个块设备
               memset(buff,0,1024);
            sprintf(buff,"\"type\":\"blkdev\",\n\"sub\":[]\n");
            if(pjson)
                strcat(pjson,buff);
            else
                *nlen += strlen(buff);
        }
        else if(S_ISFIFO(mode)){
            //是否 是一个FIFO文件.
               memset(buff,0,1024);
            sprintf(buff,"\"type\":\"fifo\",\n\"sub\":[]\n");
            if(pjson)
                strcat(pjson,buff);
            else
                *nlen += strlen(buff);
        }
        else if(S_ISSOCK(mode)){
            //是否是一个SOCKET文件 
               memset(buff,0,1024);
            sprintf(buff,"\"type\":\"sock\",\n\"sub\":[]\n");
            if(pjson)
                strcat(pjson,buff);
            else
                *nlen += strlen(buff);
        }
		else 
        {
            memset(buff,0,1024);
            sprintf(buff,"\"type\":\"unknown\",\n\"sub\":[]\n");
            if(pjson)
                strcat(pjson,buff);
            else
                *nlen += strlen(buff);
        }*/
        memset(buff,0,1024);
        sprintf(buff,"},\n");
        if(pjson)
            strcat(pjson,buff);
        else
            *nlen += strlen(buff);

		file[file_l] = '\0';
	}

out:
	if(dir)
		closedir(dir);
    //在return前把最后一个“,”删除掉，则满足JSON格式
    //printf("#*#*\n");
    if(  btrip_flag && pjson )
        pjson[strlen(pjson)-2] = ' ';
	return 0;	
			
}

/**
 *  遍历目录，生成JSON文件 
 *  @param s_start_dir : 起始目录
 *  @param depth : 遍历深度
 *  @param pjson : JSON串，传入NULL获取总长度
 *  @param nlen  : 
 */
int stwp_getdir(char *s_start,int depth,char* pjson,int *nlen)
{
    int res = 0;
	char dfile[1024] = {0};
	struct stat stat;
    char buff[1024] = {0x00};
    
    if(nlen==NULL)
        return -1;
 	strncpy(dfile, s_start, strlen(s_start));
	if(strcmp(dfile, "/") && (dfile[strlen(dfile) - 1] == '/'))
		dfile[strlen(dfile) - 1] = '\0';

	if (-1 == lstat(dfile, &stat)) {
      	return -1;
	}
    
    memset(buff,0,1024);
    sprintf(buff,"{\"name\":\"%s\",\n",dfile);
    if(pjson)
        strcat(pjson,buff);
    else
        *nlen += strlen(buff);
   
    if (S_ISDIR(stat.st_mode)) {
        memset(buff,0,1024);
        sprintf(buff,"\"type\":\"folder\",\n\"sub\":[\n");
        if(pjson)
            strcat(pjson,buff);
        else
            *nlen += strlen(buff);

        if(depth >0 )
            stwp_getdir_in(dfile,depth-1,pjson,nlen);
        memset(buff,0,1024);
        sprintf(buff,"]\n");
        if(pjson)
            strcat(pjson,buff);
        else
            *nlen += strlen(buff);
      
    }
    else{
         memset(buff,0,1024);
        sprintf(buff,"\"type\":\"file\",\n\"sub\":[]\n");
       
        if(pjson)
            strcat(pjson,buff);
        else
            *nlen += strlen(buff);
       
       
    }

    
    memset(buff,0,1024);
    sprintf(buff,"}\n");
    if(pjson)
        strcat(pjson,buff);
    else
        *nlen += strlen(buff);
  
   // printf(pjson);
 
	return 0;
  
}


/**
 * 处理客户端请求线程函数
 */
int  doDirServer(cJSON *root_json,int socket_fd)
{
   

    cJSON *json_key_dir= NULL,*json_key_depth=NULL;
    char * pjson = NULL;
    int nlen_json = 0;
    int errcode=STWP_VALUE_TYPE_ErrData;

  
    json_key_dir = cJSON_GetObjectItem(root_json, "dir");
    if (json_key_dir == NULL){
        stwp_logdump_module.push("[stwp_dirserve] get json dir failed ,error=:%s\n", cJSON_GetErrorPtr());
        goto error;
    }

    json_key_depth = cJSON_GetObjectItem(root_json, "depth");
    if (json_key_depth == NULL){
        stwp_logdump_module.push("[stwp_dirserve] get json depth failed ,error=:%s\n", cJSON_GetErrorPtr());
        goto error;
    }

    printf("----->dir:%s\n", json_key_dir->valuestring);
    printf("----->depth:%d\n", json_key_depth->valueint);
    //char buff[4096*200] = {0x00};
    nlen_json = 0 ;
    pjson=NULL;
    stwp_getdir(json_key_dir->valuestring,json_key_depth->valueint,NULL,&nlen_json);
    printf("json string length = %d \n",nlen_json);
    // stwp_getdir(json_key_dir->valuestring,json_key_depth->valueint,buff,4096*200);
    char * pTmp = "{\"type\":700,\"errcode\":0,\"count\": 1,\"data\":";
    char * pTmp1 = "{\"type\":700,\"errcode\":0,\"count\": 0,\"data\":{}}";
    char * pTmp2= "}";
    int nheaderlen = strlen(pTmp);
    int ntaillen = strlen(pTmp2);
    pjson= (char*)malloc(nlen_json+ nheaderlen + ntaillen + 2);

    if(pjson)
    {
        memset(pjson,0,nlen_json+2);
        if( nlen_json == 0 )
            sprintf(pjson,"%s",pTmp1);
           
        else
        {
            sprintf(pjson,"%s",pTmp);
            stwp_getdir(json_key_dir->valuestring,json_key_depth->valueint,pjson+nheaderlen,&nlen_json);   
            strcat(pjson,pTmp2);
        }
           
        //printf(pjson);  
        
        int nlen = strlen(pjson);
        printf("response data len = %d \n", nlen);
        send(socket_fd,&nlen,sizeof(int),0);
       // printf("send back :%s \n",pjson);
        send(socket_fd,pjson,nlen,0);
/*
        cJSON* pdata = cJSON_Parse(pjson);
        if (NULL == pdata)
        {
            free(pjson);
            pjson=NULL;;
            goto error;
        }


        sendback_to_ui2(STWP_VALUE_TYPE_DirServer,socket_fd,0,pdata);  

        //cJSON_Delete(pdata);      
      */ 
        free(pjson);
        pjson=NULL;
        return 0;

    }
error:
    sendback_to_ui2(STWP_VALUE_TYPE_DirServer,socket_fd,errcode );
    return -1;

  
}


