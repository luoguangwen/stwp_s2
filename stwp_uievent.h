#ifndef _stwp_uievent_h_
#define _stwp_uievent_h_

#define STWP_UIEVENT_UUID_SIZE  64 
#define STWP_UIEVENT_BUFF_SIZE  1024
#define STWP_UIEVENT_CACHE_SIZE  10240
#define STWP_UIEVENT_LISTEN_PATH "/tmp/.stwp_channel" 
#define STWP_UIEVENT_PORT 8008

typedef struct stwp_uievent_node_s {
    int fd;
    pthread_rwlock_t lock;

    struct list_head head;
} stwp_uievent_node_t;

typedef struct stwp_uievent_conn_s {
    struct list_head node;
    int client_fd;
    char *p_begin;
    char *p_end;
    char p_rdcache[0];
} stwp_uievent_conn_t;

struct stwp_uievent_module
{
    int (* init) (void);
    int (* evwait) (void);
};


extern struct stwp_uievent_module stwp_uievent_module;

#endif
