#ifndef __stwp_util_h__
#define __stwp_util_h__

typedef enum STWP_MSGTYPE{
    STWP_PROCRUN = 0,
    STWP_PROCACCESS,
    STWP_USERACCESS,
    STWP_RELOAD,
    STWP_PAM,
    STWP_ATTRIBUTEPRO,
    STWP_SENSESACCESS,
    STWP_USBACCESS,
}stwp_msgtype_t;

                                                                                      
struct stwp_common_msgnode{                                                 
    stwp_msgtype_t type;                                                    
}__attribute__((packed));                                                             
                                                                                      
struct stwp_procrun_msgnode
{
    stwp_msgtype_t type;                                                    
    int result;                                                                       
    int proc_type;                                                                    
    char proc_name[1024];                                                             
    char proc_hash[64];                                                               
    uint32_t fsize;                                                                   
    uint32_t atime;                                                                   
    uint32_t mtime;                                                                   
    uint32_t ctime;                                                                   
}__attribute__((packed));

struct stwp_weekpwd_node
{
    struct list_head node;

    char username[512];
    char password[512];
};


extern int stwp_run_once ();


extern char* stwp_util_get_tname_bytype( int nType);
extern int stwp_util_gen_salt(char* salt);
extern long long stwp_util_get_time();
extern int stwp_util_get_uuid(char buf[37]);
extern int stwp_util_get_encpwd(char* pwd,char salt[16],char* enc_pwd);


#endif

