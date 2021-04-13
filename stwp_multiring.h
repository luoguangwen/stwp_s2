#ifndef __stwp_multiring_h__
#define __stwp_multiring_h__

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define STWP_MULTIRING_X86_ALIGN_SIZE 64
#define STWP_MULTIRING_X86_ALIGN_MASK (STWP_MULTIRING_X86_ALIGN_SIZE - 1)

#define STWP_MULTIRING_COMPILER_BARRIER() do {\
	        asm volatile ("" : : : "memory");\
} while(0)

typedef struct stwp_multiring stwp_multiring_t;

#define queue_for_each(ring,item,iterator) 	for( iterator =  ring->cons.tail,item = ring->ring[iterator & ring->prod.mask];	\
	iterator <=  ring->prod.head ; \
	item = ring->ring[iterator & ring->prod.mask],iterator ++ )


struct stwp_multiring_module
{
	stwp_multiring_t *(* create)(uint32_t nem);
	int (* enqueue)(stwp_multiring_t *ring, void *item);
	int (* dequeue)(stwp_multiring_t *ring, void **item);
	int (* destroy)(void);
	
}__attribute__((packed));

//无锁循环队列
struct stwp_multiring
{
	/*stwp_multiring producer status*/
	struct producer
	{
		uint32_t watermark;
		uint32_t sp_enqueue;
		uint32_t size;
		uint32_t mask;

		volatile uint32_t head;
		volatile uint32_t tail;
	}prod __attribute__((__aligned__(64)));

	/*stwp_multiring consumer status */
	struct consumer
	{
		uint32_t sc_dequeue;	
		uint32_t size;
		uint32_t mask;

		volatile uint32_t head;
		volatile uint32_t tail;
	}cons __attribute__((__aligned__(64)));

	void *ring[0];
}__attribute__((packed));

extern struct stwp_multiring_module stwp_multiring_module;

#endif

