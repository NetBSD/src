#include "param.h"
#include "systm.h"
#include "malloc.h"

#include "net/netisr.h"
#include "machine/isr.h"


struct isr *isr_array[NISR];

void isr_init()
{
    int i;

    for (i = 0; i < NISR; i++)
	isr_array[i] = NULL;
}

void isr_add(level, handler, arg)
     int level;
     int (*handler)();
     int arg;
{
    struct isr *new_isr;

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
    if (isr_array[level]) 
	new_isr->isr_forw = isr_array[level];
    isr_array[level] = new_isr;
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
