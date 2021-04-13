#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>



#include <sys/types.h>
#include <sys/stat.h>
#include "stwp_list.h"
#include "stwp_logdump.h"
#include "stwp_util.h"
#include "stwp_session.h"
#define INIT_FLAG 0x5AA5


int g_init_flag;
stwp_session g_ui2_session;

#define INIT_G_STRUCT     if(g_init_flag != INIT_FLAG)\
    {\
        stwp_session_init();\
        g_init_flag = INIT_FLAG;\
    }


void stwp_session_init(void)
{
    g_ui2_session.sess_status = -1;
    g_ui2_session.user_priv = -1;
    memset(g_ui2_session.cur_user,0,sizeof(g_ui2_session.cur_user));
}
int stwp_session_setstatus( int sts)
{
    INIT_G_STRUCT;
    g_ui2_session.sess_status = sts;
    return 0;
}
int stwp_session_getstatus( )
{
    INIT_G_STRUCT;
    return g_ui2_session.sess_status;
}
int stwp_session_setuser(char * user)
{
    INIT_G_STRUCT;
    if(!user)
        return -1;
    memset(g_ui2_session.cur_user,0,sizeof(g_ui2_session.cur_user));
    strncpy(g_ui2_session.cur_user,user,sizeof(g_ui2_session.cur_user));
    return 0;    
}
char* stwp_session_getuser()
{
    INIT_G_STRUCT;
    return g_ui2_session.cur_user;    
}

int stwp_session_setpriv(int priv)
{
    INIT_G_STRUCT;
    g_ui2_session.user_priv = priv;
    return 0;
}
int stwp_session_getpriv()
{
    INIT_G_STRUCT;
    return g_ui2_session.user_priv ;
}

int stwp_session_set(stwp_session* sess)
{
    if(!sess)
        return -1;
    stwp_session_getstatus(sess->sess_status);
    stwp_session_setuser(sess->cur_user);
    stwp_session_setpriv(sess->user_priv);
    return 0;
}