#include "param.h"
#include "systm.h"
#include "malloc.h"

#include "net/netisr.h"
#include "machine/isr.h"

#include "vector.h"

/*
 * Justification:
 *
 * the reason the interrupts are not initialized earlier is because of
 * concerns that if it was done earlier then this might screw up the monitor.
 * this may just be lame.
 * 
 */


extern void level0intr(), level1intr(), level2intr(), level3intr(),
    level4intr(), level5intr(), level6intr(), level7intr();

struct level_intr {
    void (*level_intr)();
    int used;
} level_intr_array[NISR] = {
    { level0intr, 0 },		/* this is really the spurious interrupt */
    { level1intr, 0 },
    { level2intr, 0 }, 
    { level3intr, 0 },
    { level4intr, 0 }, 
    { level5intr, 0 },
    { level6intr, 0 },
    { level7intr, 0 }
};

     
struct isr *isr_array[NISR];

void isr_init()
{
    int i;

    for (i = 0; i < NISR; i++)
	isr_array[i] = NULL;
}

void isr_activate(level)
     int level;
{
    level_intr_array[level].used++;
    set_vector_entry(VEC_INTERRUPT_BASE + level,
		     level_intr_array[level].level_intr);
}

void isr_cleanup()
{
    int i;

    for (i = 0; i <NISR; i++) {
	if (level_intr_array[i].used) continue;
	isr_activate(i);
    }
}

void isr_add(level, handler, arg)
     int level;
     int (*handler)();
     int arg;
{
    struct isr *new_isr;
    int first_isr;

    first_isr = 0;
    if ((level < 0) || (level >= NISR))
	panic("isr_add: attempt to add handler for bad level");
    new_isr = (struct isr *)
	malloc(sizeof(struct isr), M_DEVBUF, M_NOWAIT);
    if (!new_isr)
	panic("isr_add: allocation of new 'isr' failed");
    new_isr->isr_arg = arg;
    new_isr->isr_ipl = level;
    new_isr->isr_intr = handler;
    new_isr->isr_back = NULL;
    new_isr->isr_forw = isr_array[level];
    if (!isr_array[level]) first_isr++;
    isr_array[level] = new_isr;
    if (first_isr) 
	isr_activate(level);
}

void intrhand(sr)
	int sr;
{
    struct isr *isr;
    int found;
    int ipl;

    ipl = (sr >> 8) & 7;

    isr = isr_array[ipl];
    if (!isr) {
	printf("intrhand: unexpected sr 0x%x\n", sr);
	return;
    }
    if (!ipl) {
	printf("intrhand: spurious interrupt\n");
	return;
    }
    found = 0;
    for (; isr != NULL; isr = isr->isr_forw)
	if (isr->isr_intr(isr->isr_arg)) {
	    found++;
	    break;
	}
    if (!found)
	printf("intrhand: stray interrupt, sr 0x%x\n", sr);
}

netintr()
{
#ifdef INET
	if (netisr & (1 << NETISR_IP)) {
		netisr &= ~(1 << NETISR_IP);
		ipintr();
	}
#endif
#ifdef NS
	if (netisr & (1 << NETISR_NS)) {
		netisr &= ~(1 << NETISR_NS);
		nsintr();
	}
#endif
#ifdef ISO
	if (netisr & (1 << NETISR_ISO)) {
		netisr &= ~(1 << NETISR_ISO);
		clnlintr();
	}
#endif
}

