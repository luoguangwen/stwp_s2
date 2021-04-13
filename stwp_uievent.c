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
#include "cJSON.h"
#include "stwp_p2.h"

static stwp_uievent_node_t uievent;
volatile int stwp_uievent_stop_flag;

static void * stwp_uievent_module_event_loop(void *arg)
{
    stwp_uievent_conn_t *item, *nxt;
   // stwp_data_node_t *pnode;

    while (stwp_uievent_stop_flag)
    {
        if (list_empty(&uievent.head))
        {
            usleep(100000); 
            continue;
        }
        list_for_each_entry_safe(item, nxt, &uievent.head, node)
        {
            
           // stwp_logdump_module.push("This is a message from Front QT Process !");

            printf("Process client[%d]'s data: %s \n",item->client_fd,item->p_begin);
            handle_ui_request(item->p_begin,item->client_fd);


            list_del(&item->node);
            free(item);
        }
    }

    return NULL;
}

static int stwp_uievent_module_inside_init(void)
{
  //  #define LOCAL_UNIX_SOCKET
    
    int fd, len;
    pthread_t tid;
    
    socklen_t addrlen;
   
    stwp_uievent_stop_flag = 1;
    memset(&uievent, 0x0, sizeof(stwp_uievent_node_t));

#ifdef LOCAL_UNIX_SOCKET
    struct sockaddr_un addr;
    addrlen = sizeof(struct sockaddr_un);    

    addr.sun_family = AF_UNIX;
    len = sizeof(STWP_UIEVENT_LISTEN_PATH) - 1;
    strncpy(addr.sun_path, STWP_UIEVENT_LISTEN_PATH, len);
    addr.sun_path[len] = '\0';


    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == fd)
        return -1;   

    if (!access(STWP_UIEVENT_LISTEN_PATH, F_OK))
        unlink(STWP_UIEVENT_LISTEN_PATH);
    if (bind(fd, (struct sockaddr *)&addr, addrlen))
        goto err0;
    listen(fd, 0);
#else
    struct sockaddr_in addr;
    addrlen = sizeof(struct sockaddr);
    bzero(&addr,sizeof(addr)); //把一段内存区的内容全部设置为0
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(STWP_UIEVENT_PORT);
      
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd)
        return -1;    
    //把socket和socket地址结构联系起来
    if( bind(fd,(struct sockaddr*)&addr,addrlen))
    {
        printf("Server Bind Port : %d Failed!", STWP_UIEVENT_PORT); 
        return -1;
    }
 
    //server_socket用于监听
    if ( listen(fd, 20) )
    {
        printf("Server Listen Failed!"); 
        return -1;
    }
#endif
    

    
    uievent.fd = fd;
     printf("server listen:socket_event.fd = %d  \n",uievent.fd);
    INIT_LIST_HEAD(&uievent.head);
    pthread_rwlock_init(&uievent.lock, NULL);
    /*init thread*/
    pthread_create(&tid, NULL, stwp_uievent_module_event_loop, NULL);

    return 0;

err0:

    close(fd);
    uievent.fd = -1;
    return -1;
}


static int stwp_uievent_module_rcvdata(int fd, stwp_uievent_conn_t *pconn)
{
    char p_reserved[4] = {0};
    int ret = -1, rz = 0, datalen = 0, reserved = 0;

    rz = recv(fd, &datalen, sizeof(int), 0);
   
    if(rz == 0) //断开连接了
        return rz;
    printf("recv len = %d\n",datalen);
    if (rz != sizeof(int))
    {
        rz = -1;
        goto err0;
    }
    

    rz = recv(fd, pconn->p_begin, datalen, 0);
    if (rz != datalen)
    {
        printf("rz = %d\n",rz);
        rz = -1; 
        memset(pconn->p_begin, 0x0, rz);
       
    }
  

err0:
    return rz;
}


static int stwp_uievent_module_inside_evwait(void)
{
    socklen_t addrlen;
    struct sockaddr addr;
    struct pollfd fds[1024];
    stwp_uievent_conn_t *pconn, *p;
    int i, ret, nfds = 0, maxsize = 1024, connfd = 0;

    for (i = 0; i < maxsize; i++)
        fds[i].fd = -1;

    fds[0].fd = uievent.fd;
    fds[0].events = POLLIN;
    nfds = 0;
  

    while(stwp_uievent_stop_flag)
    {
        int nr;
        nr = poll(fds, nfds + 1, 0);
	
        if(nr == 0)
        {
            usleep(1000);
            continue;
        }
        else if(nr < 0)
        {
            usleep(1000); 
            continue;
        }
       
        i = 0;
        if(fds[0].revents & POLLIN)
        {
             printf("new connect is arriving %d,fds[0].fd=%d \n",nr,fds[0].fd);
            connfd = accept(uievent.fd, (struct sockaddr*)&addr, &addrlen);
            for (i = nfds + 1; i < maxsize; i++)
            {
                if (fds[i].fd < 0)
                {
                    fds[i].fd = connfd;
                    fds[i].events =  POLLIN;
                    break;
                }
            }
            if (i > nfds)
                nfds = i;

            if (--nr <= 0)
                continue;
        }
		//跳过 fds[0].fd,这是accept监听的
        for(i = 1; i <= nfds; i++)
        {
      
            if (fds[i].fd < 0)
                continue;
            if(POLLIN != (fds[i].revents & POLLIN))
                continue;
            if(fds[i].revents == 0)
                continue;
            //printf("******************************\n");
            //printf("nfds = %d\n",nfds);
            //printf("POLLIN=%08X,POLLHUP=%08X,POLLERR=%08X \n",POLLIN,POLLHUP,POLLERR);
            //printf("fds[%d].fd = %d\n",i,fds[i].fd);
            //printf("fds[%d].events = %08x\n",i,fds[i].events);
            //printf("fds[%d].revents = %08x\n",i,fds[i].revents);
            pconn = (stwp_uievent_conn_t *)malloc(sizeof(stwp_uievent_conn_t) + STWP_UIEVENT_CACHE_SIZE);
            if (pconn) 
            {
                memset(pconn, 0x0, sizeof(stwp_uievent_conn_t) + STWP_UIEVENT_CACHE_SIZE); 
                p = pconn;
                p++;
                INIT_LIST_HEAD(&pconn->node);
                pconn->client_fd = fds[i].fd;
                pconn->p_begin = (char *)p;
                pconn->p_end = pconn->p_begin + STWP_UIEVENT_CACHE_SIZE;
                /*read*/
				ret = stwp_uievent_module_rcvdata(fds[i].fd, pconn);
                if (ret > 0)
                {
					printf("Receive data from client %d len%d= %s \n ",fds[i].fd,ret,pconn->p_begin);
                    pthread_rwlock_wrlock(&uievent.lock);
                    list_add_tail(&pconn->node, &uievent.head);
                    pthread_rwlock_unlock(&uievent.lock);
                }
				else if (0== ret )
                {//断开       
                    printf("client %d is disconnected\n",fds[i].fd);           
                    fds[i].fd = -1;

                }
                else
                {
                    /* code */
                }
            
            }

            
            if (--nr <= 0)
                continue;
        }
    }

    return 0;
}

struct stwp_uievent_module  stwp_uievent_module = {
    .init       = stwp_uievent_module_inside_init,
    .evwait     = stwp_uievent_module_inside_evwait,
};
