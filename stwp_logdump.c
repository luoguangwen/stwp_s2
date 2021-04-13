#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>


#include "stwp_multiring.h"
#include "stwp_logdump.h"

static volatile int stwp_logdump_run_flag;

static pthread_mutex_t stwp_logdump_lock;
static pthread_cond_t stwp_logdump_cond;

static struct stwp_multiring *stwp_logdump_ring;


static int stwp_logdump_module_inside_inside_dump(char *errmsg)
{
    FILE *fp;
    struct stat _stat;
    const char *tag = "a+";
    char name[1024], st[128] = {0};
    time_t t = time(NULL);

    ctime_r(&t, st);
    st[strlen(st) - 1] = '\0';

    memset(name, 0x0, sizeof(name));
    sprintf(name, "%s%s", STWP_LOGDUMP_CONTENT, STWP_LOGDUMP_FILE);

    memset(&_stat, 0x0, sizeof(_stat));
    if (!stat(name, &_stat) && _stat.st_size > STWP_LOGDUMP_MAX_SIZE)
        tag = "w+";

    fp = fopen(name, tag);
    if (fp == NULL)
    {   
        fprintf(stderr, "[%s] --- fopen failed %s.\n",st, STWP_LOGDUMP_FILE);
        goto out;
    }

    fprintf(fp, "[%s] --- DUMP: %s\n", st, errmsg);
    fclose(fp);

out:
    return 0;
}


static void * stwp_logdump_module_inside_dump(void *arg)
{
    /*wait for running*/
    int ret;
    sigset_t set;

    sigfillset(&set);
    if (pthread_sigmask(SIG_SETMASK, &set, NULL) != 0)
    {
        fprintf(stderr, "pthread_sigmask error in stwp_logdump_module_inside_dump\n");	
        _exit(-1);
    }

    pthread_cond_wait(&stwp_logdump_cond, &stwp_logdump_lock);
    pthread_mutex_unlock(&stwp_logdump_lock);

    char *msg = NULL;
    while (stwp_logdump_run_flag)
    {
        ret = stwp_multiring_module.dequeue(stwp_logdump_ring, (void **)&msg);		
        if (ret)
        {
            usleep(1000);	
            continue;
        }
        if (msg)
        {
            stwp_logdump_module_inside_inside_dump(msg);
            free(msg);	
        }
    }

    return NULL;
}


static int stwp_logdump_module_init(void)
{
    int ret = 0;
    pthread_t stwp_logdump_pid;

    stwp_logdump_run_flag = 1;

    pthread_mutex_init(&stwp_logdump_lock, NULL);
    pthread_cond_init(&stwp_logdump_cond, NULL);

    stwp_logdump_ring = stwp_multiring_module.create(STWP_LOGDUMP_RING_HEIGH);
    if (!stwp_logdump_ring)
    {
        ret = -1;	
        goto out;
    }

    /*make sure getting lock before run*/
    pthread_mutex_lock(&stwp_logdump_lock);
    ret = pthread_create(&stwp_logdump_pid, NULL, stwp_logdump_module_inside_dump, NULL);
    if (ret != 0)
    {
        ret = -1;
        pthread_mutex_unlock(&stwp_logdump_lock);
        goto out;
    }    
    pthread_detach(stwp_logdump_pid);

out:
    return ret;
}


static int stwp_logdump_module_run(void)
{
    pthread_mutex_lock(&stwp_logdump_lock);
    pthread_cond_signal(&stwp_logdump_cond);
    pthread_mutex_unlock(&stwp_logdump_lock);

    return 0;
}

static int stwp_logdump_module_stop(void)
{
    stwp_logdump_run_flag = 0;

    return 0;
}

static int stwp_logdump_module_push(char *msg, ...)
{
    int ret = 0;
    char *buff = NULL;
    va_list list;
    va_start(list, msg);

    buff = (char *)malloc(2048);
    if (!buff)
    {
        ret = -1;	
        goto out;
    }

    memset(buff, 0x0, 2048);
    vsprintf(buff, msg, list);

    ret = stwp_multiring_module.enqueue(stwp_logdump_ring, buff);
    if (ret)
    {
        ret = -1;	
        free(buff);
    }

out:
    va_end(list);

    return ret;
}


struct stwp_logdump_module stwp_logdump_module = 
{
    .init   = stwp_logdump_module_init,
    .run    = stwp_logdump_module_run,
    .stop   = stwp_logdump_module_stop,
    .push   = stwp_logdump_module_push
};



