#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>

#include "stwp_multiring.h"

static int stwp_multiring_atomic32_cmpset(volatile uint32_t *dst, uint32_t exp, uint32_t src)
{
    uint8_t res;

    asm volatile(
            "lock ; "
            "cmpxchgl %[src], %[dst];"
            "sete %[res];"
            : [res] "=a" (res), 
              [dst] "=m" (*dst)
            : [src] "r" (src), 
              "a" (exp),
              "m" (*dst)
            : "memory"); 
    return res;
}

static int stwp_multiring_module_inside_align_pow2(uint32_t num)
{
	int offset = 0;
	uint32_t mark1, mark2;
	do
	{
		offset++;
		mark1 = (1 << offset);
		mark2 = (1 << (offset + 1));
	}while(num >= mark1);

	return (1 << offset); 
}

/**
 *@Destription: calloc structure stwp_multiring_t
 *@Return npool/success null/false
 */
static stwp_multiring_t *stwp_multiring_module_inside_init_npool(uint32_t nem)
{
	int i, tnem;
	struct stwp_multiring *npool;

	tnem = stwp_multiring_module_inside_align_pow2(nem);

	npool = (struct stwp_multiring *)malloc(sizeof(struct stwp_multiring)
			+ sizeof(void *) * tnem);
	if(npool)/*calloc success and initialize*/
	{
		for(i = 0; i < tnem; i++)
		{
			npool->ring[i] = NULL; 
		}
		npool->prod.watermark = tnem;
		npool->prod.size = npool->cons.size = tnem;
		npool->prod.mask = npool->cons.mask = tnem - 1;
		npool->prod.head = npool->cons.head = 0;
		npool->prod.tail = npool->cons.tail = 0;
	}
	return npool;
}

static stwp_multiring_t *stwp_multiring_module_create(uint32_t nem)
{
	struct stwp_multiring *npool;

	npool = stwp_multiring_module_inside_init_npool(nem);
	if(!npool)
		goto out;

out:
	return npool;
}

/*
 *@Destription multi-cores safe
 *@Return 0/success -1/false
 */
static int stwp_multiring_module_enqueue(struct stwp_multiring * ring, void * item)
{
	int success, ret;
	uint32_t prod_head, prod_next;
	uint32_t cons_tail, free_entries;
	uint32_t mask = ring->prod.mask;

	do{
		prod_head = ring->prod.head;
		cons_tail = ring->cons.tail;
		free_entries = (cons_tail - prod_head + mask);
		if(free_entries <= 0)
		{
			ret = -1;
			goto out;
		}

		prod_next = prod_head + 1;
		success = stwp_multiring_atomic32_cmpset(&ring->prod.head, prod_head, prod_next);
	}while(unlikely(success == 0));

	ring->ring[prod_head & mask] = item;
	STWP_MULTIRING_COMPILER_BARRIER();

	while(unlikely(ring->prod.tail != prod_head));
	ring->prod.tail = prod_next;

	ret = 0;
out:
	return ret;
}

/*
 *@Destription multi-cores safe
 *@Return 0/success -1/false
 */
static int stwp_multiring_module_dequeue(struct stwp_multiring *ring, void **item)
{
	int success, ret;
	uint32_t cons_head, prod_tail;	
	uint32_t cons_next, busy_entries;
	uint32_t mask = ring->prod.mask;

	do
	{
		prod_tail = ring->prod.tail;		
		cons_head = ring->cons.head;
		busy_entries = (prod_tail - cons_head);
		if(busy_entries <= 0)	
		{
			ret = -1;
			goto out;
		}

		cons_next = cons_head + 1;
		success = stwp_multiring_atomic32_cmpset(&ring->cons.head,\
			cons_head, cons_next);

	}while(unlikely(success == 0));

	*item = ring->ring[cons_head & mask];
	STWP_MULTIRING_COMPILER_BARRIER();

	while(unlikely(ring->cons.tail != cons_head));
	ring->cons.tail = cons_next;

	ret = 0;
out:	
	return ret;
}



static int stwp_multiring_module_destroy(void)
{
	return 0;
}


struct stwp_multiring_module stwp_multiring_module = 
{
	.create		= stwp_multiring_module_create,
	.enqueue	= stwp_multiring_module_enqueue,
	.dequeue	= stwp_multiring_module_dequeue,
	.destroy	= stwp_multiring_module_destroy
};
