#ifndef _STWP_CONFIG_H_
#define _STWP_CONFIG_H_

#define PORT 8000  //监听端口号

#define     TABLE_AUDIT_        "stwp_audit_"
#define     TABLE_CENTER        "stwp_audit_center"
#define     TABLE_KEY           "stwp_exchangekey"
#define     TABLE_HOSTS         "stwp_hosts"
#define     TABLE_GROUP         "stwp_group"
#define     TABLE_PERMISSIONS   "stwp_permissions"          
#define     TABLE_POLICIES      "stwp_policies"             
#define     TABLE_TASK          "stwp_policies_task"        
#define     TABLE_USER          "stwp_users"                
#define     TABLE_WARNING       "stwp_warning"    
#define     TABLE_SCROLL        "stwp_warning_recent"          
#define     TABLE_WARNING_      "stwp_warning_"           
#define     TABLE_APP_          "stwp_whitelist_app_"     
#define     TABLE_BOOT_         "stwp_whitelist_boot_"    
#define     TABLE_CONFIG_       "stwp_whitelist_config_" 
#define     TABLE_DYNAMIC_       "stwp_whitelist_dynamic_"
#define     SCHEMA_GET          "select count(*) from information_schema.`COLUMNS` where TABLE_SCHEMA='stwp'and TABLE_NAME="


#endif 