/*	$NetBSD: isadma.c,v 1.22 1997/03/21 00:00:21 mycroft Exp $	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/uio.h>

#include <vm/vm.h>

#include <machine/pio.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isadmavar.h>
#include <dev/isa/isadmareg.h>

/* region of physical memory known to be contiguous */
vm_offset_t isaphysmem;
static caddr_t dma_bounce[8];		/* XXX */
static char bounced[8];		/* XXX */
#define MAXDMASZ 512		/* XXX */
static u_int8_t dma_finished;

/* high byte of address is stored in this port for i-th dma channel */
static int dmapageport[2][4] = {
	{0x87, 0x83, 0x81, 0x82},
	{0x8f, 0x8b, 0x89, 0x8a}
};

static u_int8_t dmamode[4] = {
	DMA37MD_READ | DMA37MD_SINGLE,
	DMA37MD_WRITE | DMA37MD_SINGLE,
	DMA37MD_READ | DMA37MD_LOOP,
	DMA37MD_WRITE | DMA37MD_LOOP
};

int	isa_dmarangecheck	__P((vm_offset_t, u_long, int));
caddr_t	isa_allocphysmem	__P((caddr_t, unsigned, void (*)(void)));
void	isa_freephysmem		__P((caddr_t, unsigned));

/*
 * isa_dmacascade(): program 8237 DMA controller channel to accept
 * external dma control by a board.
 */
void
isa_dmacascade(chan)
	int chan;
{

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7)
		panic("isa_dmacascade: impossible request"); 
#endif

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0) {
		outb(DMA1_MODE, chan | DMA37MD_CASCADE);
		outb(DMA1_SMSK, chan | DMA37SM_CLEAR);
	} else {
		chan &= 3;

		outb(DMA2_MODE, chan | DMA37MD_CASCADE);
		outb(DMA2_SMSK, chan | DMA37SM_CLEAR);
	}
}

/*
 * isa_dmastart(): program 8237 DMA controller channel, avoid page alignment
 * problems by using a bounce buffer.
 */
void
isa_dmastart(flags, addr, nbytes, chan)
	int flags;
	caddr_t addr;
	vm_size_t nbytes;
	int chan;
{
	vm_offset_t phys;
	int waport;
	caddr_t newaddr;

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7 ||
	    ((chan & 4) ? (nbytes >= (1<<17) || nbytes & 1 || (u_int)addr & 1) :
	    (nbytes >= (1<<16))))
		panic("isa_dmastart: impossible request"); 
#endif

	if (isa_dmarangecheck((vm_offset_t) addr, nbytes, chan)) {
		if (dma_bounce[chan] == 0)
			dma_bounce[chan] =
			    /*(caddr_t)malloc(MAXDMASZ, M_TEMP, M_WAITOK);*/
			    (caddr_t) isaphysmem + NBPG*chan;
		bounced[chan] = 1;
		newaddr = dma_bounce[chan];
		*(int *) newaddr = 0;	/* XXX */
		/* copy bounce buffer on write */
		if ((flags & DMAMODE_READ) == 0)
			bcopy(addr, newaddr, nbytes);
		addr = newaddr;
	}

	/* translate to physical */
	phys = pmap_extract(pmap_kernel(), (vm_offset_t)addr);

	dma_finished &= ~(1 << chan);

	if ((chan & 4) == 0) {
		/*
		 * Program one of DMA channels 0..3.  These are
		 * byte mode channels.
		 */
		/* set dma channel mode, and reset address ff */
		outb(DMA1_MODE, chan | dmamode[flags]);
		outb(DMA1_FFC, 0);

		/* send start address */
		waport = DMA1_CHN(chan);
		outb(dmapageport[0][chan], phys>>16);
		outb(waport, phys);
		outb(waport, phys>>8);

		/* send count */
		outb(waport + 1, --nbytes);
		outb(waport + 1, nbytes>>8);

		/* unmask channel */
		outb(DMA1_SMSK, chan | DMA37SM_CLEAR);
	} else {
		chan &= 3;

		/*
		 * Program one of DMA channels 4..7.  These are
		 * word mode channels.
		 */
		/* set dma channel mode, and reset address ff */
		outb(DMA2_MODE, chan | dmamode[flags]);
		outb(DMA2_FFC, 0);

		/* send start address */
		waport = DMA2_CHN(chan);
		outb(dmapageport[1][chan], phys>>16);
		phys >>= 1;
		outb(waport, phys);
		outb(waport, phys>>8);

		/* send count */
		nbytes >>= 1;
		outb(waport + 2, --nbytes);
		outb(waport + 2, nbytes>>8);

		/* unmask channel */
		outb(DMA2_SMSK, chan | DMA37SM_CLEAR);
	}
}

void
isa_dmaabort(chan)
	int chan;
{

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7)
		panic("isa_dmaabort: impossible request");
#endif

	bounced[chan] = 0;

	/* mask channel */
	if ((chan & 4) == 0) {
		outb(DMA1_SMSK, chan | DMA37SM_SET);
	} else {
		chan &= 3;

		outb(DMA2_SMSK, chan | DMA37SM_SET);
	}
}

vm_size_t
isa_dmacount(chan)
	int chan;
{
	int waport;
	vm_size_t nbytes;

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7)
		panic("isa_dmafinished: impossible request");
#endif

	/* check that the terminal count was reached */
	if ((chan & 4) == 0) {
		/* mask channel */
		outb(DMA1_SMSK, chan | DMA37SM_SET);
		outb(DMA1_FFC, 0);

		if (!isa_dmafinished(chan)) {
			/* read count */
			waport = DMA1_CHN(chan);
			nbytes = inb(waport + 1) + 1;
			nbytes += inb(waport + 1) << 8;
		} else
			nbytes = 0;

		/* unmask channel */
		outb(DMA1_SMSK, chan | DMA37SM_CLEAR);
	} else {
		chan &= 3;

		/* mask channel */
		outb(DMA2_SMSK, chan | DMA37SM_SET);
		outb(DMA2_FFC, 0);

		if (!isa_dmafinished(chan | 4)) {
			/* read count */
			waport = DMA2_CHN(chan);
			nbytes = inb(waport + 2) + 1;
			nbytes += inb(waport + 2) << 8;
			nbytes <<= 1;
		} else
			nbytes = 0;

		/* unmask channel */
		outb(DMA2_SMSK, chan | DMA37SM_CLEAR);
	}

	return (nbytes);
}

int
isa_dmafinished(chan)
	int chan;
{

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7)
		panic("isa_dmafinished: impossible request");
#endif

	/* check that the terminal count was reached */
	if ((chan & 4) == 0)
		dma_finished |= inb(DMA1_SR) & 0x0f;
	else
		dma_finished |= (inb(DMA2_SR) & 0x0f) << 4;

	return ((dma_finished & (1 << chan)) != 0);
}

void
isa_dmadone(flags, addr, nbytes, chan)
	int flags;
	caddr_t addr;
	vm_size_t nbytes;
	int chan;
{

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7)
		panic("isa_dmadone: impossible request");
#endif

	if (!isa_dmafinished(chan))
		printf("isa_dmadone: channel %d not finished\n", chan);

	/* mask channel */
	if ((chan & 4) == 0) {
		outb(DMA1_SMSK, chan | DMA37SM_SET);
	} else {
		chan &= 3;

		outb(DMA2_SMSK, chan | DMA37SM_SET);
	}

	/* copy bounce buffer on read */
	if (bounced[chan]) {
		bcopy(dma_bounce[chan], addr, nbytes);
		bounced[chan] = 0;
	}
}

/*
 * Check for problems with the address range of a DMA transfer
 * (non-contiguous physical pages, outside of bus address space,
 * crossing DMA page boundaries).
 * Return true if special handling needed.
 */
int
isa_dmarangecheck(va, length, chan)
	vm_offset_t va;
	u_long length;
	int chan;
{
	vm_offset_t phys, priorpage = 0, endva;
	u_int dma_pgmsk = (chan & 4) ?  ~(128*1024-1) : ~(64*1024-1);

	endva = round_page(va + length);
	for (; va < endva ; va += NBPG) {
		phys = trunc_page(pmap_extract(pmap_kernel(), va));
		if (phys == 0)
			panic("isa_dmacheck: no physical page present");
		if (phys >= (1<<24)) 
			return 1;
		if (priorpage) {
			if (priorpage + NBPG != phys)
				return 1;
			/* check if crossing a DMA page boundary */
			if ((priorpage ^ phys) & dma_pgmsk)
				return 1;
		}
		priorpage = phys;
	}
	return 0;
}

/* head of queue waiting for physmem to become available */
struct buf isa_physmemq;

/* blocked waiting for resource to become free for exclusive use */
static isaphysmemflag;
/* if waited for and call requested when free (B_CALL) */
static void (*isaphysmemunblock) __P((void)); /* needs to be a list */

/*
 * Allocate contiguous physical memory for transfer, returning
 * a *virtual* address to region. May block waiting for resource.
 * (assumed to be called at splbio())
 */
caddr_t
isa_allocphysmem(ca, length, func)
	caddr_t ca;
	unsigned length;
	void (*func) __P((void));
{
	
	isaphysmemunblock = func;
	while (isaphysmemflag & B_BUSY) {
		isaphysmemflag |= B_WANTED;
		sleep((caddr_t)&isaphysmemflag, PRIBIO);
	}
	isaphysmemflag |= B_BUSY;

	return((caddr_t)isaphysmem);
}

/*
 * Free contiguous physical memory used for transfer.
 * (assumed to be called at splbio())
 */
void
isa_freephysmem(va, length)
	caddr_t va;
	unsigned length;
{

	isaphysmemflag &= ~B_BUSY;
	if (isaphysmemflag & B_WANTED) {
		isaphysmemflag &= B_WANTED;
		wakeup((caddr_t)&isaphysmemflag);
		if (isaphysmemunblock)
			(*isaphysmemunblock)();
	}
}
