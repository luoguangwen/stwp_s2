#ifndef __stwp_logdump_h__
#define __stwp_logdump_h__

#define STWP_LOGDUMP_FILE "dump.log"
#ifdef LOGFILE
#define STWP_LOGDUMP_CONTENT LOGFILE 
#else
#define STWP_LOGDUMP_CONTENT "./log/"
#endif

#define STWP_LOGDUMP_RING_HEIGH (4 << 10)

#define STWP_LOGDUMP_MAX_SIZE (50 << 20)

struct stwp_logdump_module
{
	int (*init)(void);
	int (*run)(void);
	int (*stop)(void);
	int (*push)(char *msg, ...);
};

extern struct stwp_logdump_module stwp_logdump_module;


#endif
