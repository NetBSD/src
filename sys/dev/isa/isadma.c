/*	$NetBSD: isadma.c,v 1.24 1997/05/28 20:02:39 mycroft Exp $	*/

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
static char dma_bounced[8];		/* XXX */
#define MAXDMASZ 512		/* XXX */
static u_int8_t dma_finished;
static vm_size_t dma_length[8];

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

inline void
isa_dmaunmask(chan)
	int chan;
{
	int ochan = chan & 3;

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7)
		panic("isa_dmacascade: impossible request"); 
#endif

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0)
		outb(DMA1_SMSK, ochan | DMA37SM_CLEAR);
	else
		outb(DMA2_SMSK, ochan | DMA37SM_CLEAR);
}

inline void
isa_dmamask(chan)
	int chan;
{
	int ochan = chan & 3;

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7)
		panic("isa_dmacascade: impossible request"); 
#endif

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0) {
		outb(DMA1_SMSK, ochan | DMA37SM_SET);
		outb(DMA1_FFC, 0);
	} else {
		outb(DMA2_SMSK, ochan | DMA37SM_SET);
		outb(DMA2_FFC, 0);
	}
}

/*
 * isa_dmacascade(): program 8237 DMA controller channel to accept
 * external dma control by a board.
 */
void
isa_dmacascade(chan)
	int chan;
{
	int ochan = chan & 3;

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7)
		panic("isa_dmacascade: impossible request"); 
#endif

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0)
		outb(DMA1_MODE, ochan | DMA37MD_CASCADE);
	else
		outb(DMA2_MODE, ochan | DMA37MD_CASCADE);

	isa_dmaunmask(chan);
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
	int ochan = chan & 3;

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
		dma_bounced[chan] = 1;
		newaddr = dma_bounce[chan];
		*(int *) newaddr = 0;	/* XXX */
		/* copy bounce buffer on write */
		if ((flags & DMAMODE_READ) == 0)
			bcopy(addr, newaddr, nbytes);
		addr = newaddr;
	}

	dma_length[chan] = nbytes;

	/* translate to physical */
	phys = pmap_extract(pmap_kernel(), (vm_offset_t)addr);

	isa_dmamask(chan);
	dma_finished &= ~(1 << chan);

	if ((chan & 4) == 0) {
		/* set dma channel mode */
		outb(DMA1_MODE, ochan | dmamode[flags]);

		/* send start address */
		waport = DMA1_CHN(ochan);
		outb(dmapageport[0][ochan], phys>>16);
		outb(waport, phys);
		outb(waport, phys>>8);

		/* send count */
		outb(waport + 1, --nbytes);
		outb(waport + 1, nbytes>>8);
	} else {
		/* set dma channel mode */
		outb(DMA2_MODE, ochan | dmamode[flags]);

		/* send start address */
		waport = DMA2_CHN(ochan);
		outb(dmapageport[1][ochan], phys>>16);
		phys >>= 1;
		outb(waport, phys);
		outb(waport, phys>>8);

		/* send count */
		nbytes >>= 1;
		outb(waport + 2, --nbytes);
		outb(waport + 2, nbytes>>8);
	}

	isa_dmaunmask(chan);
}

void
isa_dmaabort(chan)
	int chan;
{

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7)
		panic("isa_dmaabort: impossible request");
#endif

	isa_dmamask(chan);

	dma_bounced[chan] = 0;
}

vm_size_t
isa_dmacount(chan)
	int chan;
{
	int waport;
	vm_size_t nbytes;
	int ochan = chan & 3;

#ifdef ISADMA_DEBUG
	if (chan < 0 || chan > 7)
		panic("isa_dmacount: impossible request");
#endif

	isa_dmamask(chan);

	/*
	 * We have to shift the byte count by 1.  If we're in auto-initialize
	 * mode, the count may have wrapped around to the initial value.  We
	 * can't use the TC bit to check for this case, so instead we compare
	 * against the original byte count.
	 * If we're not in auto-initialize mode, then the count will wrap to
	 * -1, so we also handle that case.
	 */
	if ((chan & 4) == 0) {
		waport = DMA1_CHN(ochan);
		nbytes = inb(waport + 1) + 1;
		nbytes += inb(waport + 1) << 8;
		nbytes &= 0xffff;
	} else {
		waport = DMA2_CHN(ochan);
		nbytes = inb(waport + 2) + 1;
		nbytes += inb(waport + 2) << 8;
		nbytes <<= 1;
		nbytes &= 0x1ffff;
	}

	if (nbytes == dma_length[chan])
		nbytes = 0;

	isa_dmaunmask(chan);
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

	isa_dmamask(chan);

	if (!isa_dmafinished(chan))
		printf("isa_dmadone: channel %d not finished\n", chan);

	/* copy bounce buffer on read */
	if (dma_bounced[chan]) {
		bcopy(dma_bounce[chan], addr, nbytes);
		dma_bounced[chan] = 0;
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
