#ifndef _STWP_P2_H_
#define _STWP_P2_H_

//P2:  Server2和UI2之间的通讯协议

#define MINI_LENGTH 64
#define MID_LENGTH 128
#define PATH_LENGTH 256
#define MAX_LENGTH 1024
extern int handle_ui_request(char *json_from_ui, int socket_fd);



typedef int(*pfunc_req_handler)(cJSON *json,int socket_fd);

typedef struct _DISPATCH_LIST{
    int req_type;  //请求类型
    pfunc_req_handler handler; //处理函数
    int priv; //函数执行权限
}st_cmd_table;

//UI2 发给S2的查询类JSON数据字段
typedef struct StwpQuery_T
{
    int type;
    int page;
    char uuid[MINI_LENGTH];
    char ip[MINI_LENGTH];
    char key[PATH_LENGTH];
    char key_time[MINI_LENGTH];
    char keystr[PATH_LENGTH];
}stwp_p2_query;
//


typedef  struct stwp_UI2_node_s
{
    int fd;
    int cnnfd;
    pthread_rwlock_t lock;
    struct list_head head;
}stwp_ui2_node_t;

typedef struct stwp_UI2_data_s
{
    struct list_head node;

    char *p_begin;
    char *p_end;
    char p_rdcache[0];
}stwp_ui2_data_t;

struct STWP_TYPE_NAME{
    int  type;
    char name[MID_LENGTH];
};
#if 1
//管理中心审计类型定义（对应数据库的op_type）
/*用户管理*/
#define  STWP_AUDIT_TYPE_Login      1001
#define  STWP_AUDIT_TYPE_Logout     1002
#define  STWP_AUDIT_TYPE_ChangePwd  1003
#define  STWP_AUDIT_TYPE_CreateUser 1004
#define  STWP_AUDIT_TYPE_ModifyUser  1005
#define  STWP_AUDIT_TYPE_DeleteUser  1006
/*主机管理*/
#define  STWP_AUDIT_TYPE_ModifyHost 2001
#define  STWP_AUDIT_TYPE_DeleteHost 2002
#define  STWP_AUDIT_TYPE_AddHost    2003
/*策略管理*/
#define  STWP_AUDIT_TYPE_CreatePolicy   3001
#define  STWP_AUDIT_TYPE_ModifyPolicy   3002
#define  STWP_AUDIT_TYPE_DeletePolicy   3003
#define  STWP_AUDIT_TYPE_CreateTask     3004
#define  STWP_AUDIT_TYPE_DeleteTask     3005
/*告警管理*/
#define  STWP_AUDIT_TYPE_ApproveWarning 4001
/*白名单管理*/
#define  STWP_AUDIT_TYPE_ExportWhiteList  5001


#endif
//数据库读操作的操作码

//查询应用白名单
#define     STWP_VALUE_TYPE_SelectAppList               220

//查询boot白名单        
#define     STWP_VALUE_TYPE_SelectBootList              221

//查询配置白名单        
#define     STWP_VALUE_TYPE_SelectConifgLis             222

//查询动态度量白名单
#define     STWP_VALUE_TYPE_SelectDynamicList           223

//查询表数量
#define     STWP_VALUE_TYPE_SelectAll                   228

//查询告警信息
#define     STWP_VALUE_TYPE_SelectAWarning              230

//查询告警信息数量
#define     STWP_VALUE_TYPE_SelectAWarningCount         231

//查询控制模式
#define     STWP_VALUE_TYPE_SelectCotrlMode             235

//查询安全模式
#define     STWP_VALUE_TYPE_SelectSecurityMode          236

//查询审计信息
#define     STWP_VALUE_TYPE_SelectAudit                 237

//查询审计信息数量
#define     STWP_VALUE_TYPE_SelectAuditCount            238

//查询注册主机信息
#define     STWP_VALUE_TYPE_SelectHost                  251

//查询主机的策略，在策略任务表中查指定主机的策略
#define     STWP_VALUE_TYPE_SelectHostPolicy            252

//查询所有主机的IP，NAME，UUID
#define     STWP_VALUE_TYPE_SelectALLHost               253

//按组uuid查询主机
#define     STWP_VALUE_TYPE_SelectHostByUuid            254

//查询组	
#define 	STWP_VALUE_TYPE_SelectGroup                 255

//查询不属于任何组的所有主机
#define     STWP_VALUE_TYPE_SelectNoGroupHost           256


//查询用户信息
#define     STWP_VALUE_TYPE_SelectUser                  271

//查询center审计
#define     STWP_VALUE_TYPE_SelectCenterAudit            291

//查询策略表，无条件分页查
#define     STWP_VALUE_TYPE_SelectPolicy                300

//查询策略任务表，无条件分页查所有
#define     STWP_VALUE_TYPE_SelectPolicyTask            301

//查询主机目录
#define     STWP_VALUE_TYPE_SelectHostDir               310

//查询滚动信息
#define     STWP_VALUE_TYPE_SelectScrollInfo            320

//查询安全模式比例，返回比值即可
#define     STWP_VALUE_TYPE_SelectSecurityStatus        330

//查询控制模式比例
#define     STWP_VALUE_TYPE_SelectCotrlStatus           330

//查询在线比例
#define     STWP_VALUE_TYPE_SelectOnlineStatus          330

//查询异常启动信息
#define     STWP_VALUE_TYPE_SelectErrorInfo             330

//查询统计数据
#define     STWP_VALU_TYPE_CollectQuery                 330
//查询异常启动信息历史数据
#define     STWP_VALUE_TYPE_SelectHistory               331




/*写数据库操作*/

//安全模式设置,0表示审计模式，1表示控制模式
#define     STWP_VALUE_TYPE_SetSecurityStatus           401

//控制模式设置，0本地，1远程
#define     STWP_VALUE_TYPE_SetCotrlMode                402

//全盘扫描
#define     STWP_VALUE_TYPE_ScanALL                     404

//boot白名单添加
#define     STWP_VALUE_TYPE_ScanBoot                    408

//白名单增量扫描
#define     STWP_VALUE_TYPE_ScanApp                     410

//配置文件添加
#define     STWP_VALUE_TYPE_ScanConfig                  420

//动态度量添加
#define     STWP_VALUE_TYPE_ScanDynamic                 440

//主机删除 
#define     STWP_VALUE_TYPE_DeletHost                   460

//主机修改	461	
#define     STWP_VALUE_TYPE_ModifyHost                  461

//主机添加	462	
#define     STWP_VALUE_TYPE_AddHost                     462

//添加组	470	
#define     STWP_VALUE_TYPE_AddGroup            470

//修改组	471	
#define     STWP_VALUE_TYPE_ModifyGroup             471

//删除组	472	
#define     STWP_VALUE_TYPE_DelGroup            472

//批量向组内加主机	473	
#define     STWP_VALUE_TYPE_AddHostToGroup          473

//批量删除组内主机	474	
#define     STWP_VALUE_TYPE_RemoveHostFromGroup         474


//用户添加，0管理，1审计，2操作
#define     STWP_VALUE_TYPE_AddUser                     480

//用户删除
#define     STWP_VALUE_TYPE_DeleteUser                  481

//用户修改
#define     STWP_VALUE_TYPE_ModifyUser                  482

//策略添加      
#define     STWP_VALUE_TYPE_AddPolicy                   490

//策略删除
#define     STWP_VALUE_TYPE_DelectPolicy                491

//策略修改   
#define     STWP_VALUE_TYPE_ModifyPolicy                492

//策略配置 ，生成策略任务  
#define     STWP_VALUE_TYPE_ConfigPolicy               493

//删除策略任务
#define    STWP_VALUE_TYPE_DeletePolicyTask             494

//告警信息审批
#define    STWP_VALUE_TYPE_WarningToPolicyTask          495



//导出应用白名单	
#define  	STWP_VALUE_TYPE_ExportAppList               501

//导出boot白名单
#define 	STWP_VALUE_TYPE_ExportBootList              502	

//导出配置白名单	
#define 	STWP_VALUE_TYPE_ExportConifgList            503	

//导出动态度量白名单	504	
#define 	STWP_VALUE_TYPE_ExportDynamicList           504

//导出主机审计记录	505	
#define 	STWP_VALUE_TYPE_ExportHostAudit             505

//导出中心审计记录	506	
#define 	STWP_VALUE_TYPE_ExportCenterAudit           506

//导出告警信息	507	    
#define 	STWP_VALUE_TYPE_ExportWarning               507

//导出信息	500	
#define 	STWP_VALUE_TYPE_Export                      500

//登录验证	601	
#define 	STWP_VALUE_TYPE_VerifyUser                  601

//修改密码	602	
#define 	STWP_VALUE_TYPE_ChangePassword              602


#define     STWP_VALUE_TYPE_DirServer                   700


/*错误码*/

//成功
#define     STWP_VALUE_TYPE_ErrSucceed                1000

//用户名已存在
#define     STWP_VALUE_TYPE_ErrUserName               1001

//用户名不存在
#define     STWP_VALUE_TYPE_UserNotExist              1002

//数据解析错误
#define     STWP_VALUE_TYPE_ErrData                   -1

//用户的密码错误 
#define     STWP_VALUE_TYPE_ErrPWD                     1003

//用记的密码重复使用
#define     STWP_VALUE_TYPE_ErrPWDRepeat            1004

//用户密码错误次数超限被锁定
#define     STWP_VALUE_TYPE_UserLocked                  1005


//对像已存在
#define   STWP_VALUE_TYPE_Existed                    1010

//对像在使用中
#define   STWP_VALUE_TYPE_Using                      1011

#endif