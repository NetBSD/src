/*	$NetBSD: amiga_init.c,v 1.25 1995/02/12 19:18:33 chopps Exp $	*/

/* Authors: Markus Wild, Bryan Ford, Niklas Hallqvist 
 *          Michael L. Hitch - initial 68040 support
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <vm/vm.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/msgbuf.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/dkbad.h>
#include <sys/reboot.h>
#include <sys/exec.h>
#include <vm/pmap.h>
#include <machine/vmparam.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <amiga/amiga/cc.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cfdev.h>
#include <amiga/amiga/memlist.h>
#include <amiga/dev/zbusvar.h>

extern int	machineid, mmutype;
extern u_int 	lowram;
extern u_int	Sysptmap, Sysptsize, Sysseg, Umap, proc0paddr;
extern u_int	Sysseg1;
extern u_int	virtual_avail;

extern char *esym;

#ifdef GRF_AGA
extern u_long aga_enable;
#endif

#ifdef MACHINE_NONCONTIG
extern u_long noncontig_enable;
#endif

/*
 * some addresses used in locore
 */
vm_offset_t INTREQRaddr;
vm_offset_t INTREQWaddr;

/*
 * these are used by the extended spl?() macros.
 */
volatile unsigned short *amiga_intena_read, *amiga_intena_write;

/*
 * the number of pages in our hw mapping and the start address
 */
vm_offset_t amigahwaddr;
u_int namigahwpg;

static vm_offset_t z2mem_start;		/* XXX */
static vm_offset_t z2mem_end;		/* XXX */
int use_z2_mem = 1;			/* XXX */

u_long boot_fphystart, boot_fphysize, boot_cphysize;

static u_long boot_flags;

void *
chipmem_steal(amount)
	long amount;
{
	/*
	 * steal from top of chipmem, so we don't collide with 
	 * the kernel loaded into chipmem in the not-yet-mapped state.
	 */
	vm_offset_t p = chipmem_end - amount;
	if (p & 1)
		p = p - 1;
	chipmem_end = p;
	if(chipmem_start > chipmem_end)
		panic("not enough chip memory");
	return((void *)p);
}

/*
 * XXX
 * used by certain drivers currently to allocate zorro II memory
 * for bounce buffers, if use_z2_mem is NULL, chipmem will be
 * returned instead.
 * XXX
 */
void *
alloc_z2mem(amount)
	long amount;
{
	if (use_z2_mem && z2mem_end && (z2mem_end - amount) >= z2mem_start) {
		z2mem_end -= amount;
		return ((void *)z2mem_end);
	}
	return (alloc_chipmem(amount));
}


/*
 * this is the C-level entry function, it's called from locore.s.
 * Preconditions:
 *	Interrupts are disabled
 *	PA == VA, we don't have to relocate addresses before enabling
 *		the MMU
 * 	Exec is no longer available (because we're loaded all over 
 *		low memory, no ExecBase is available anymore)
 *
 * It's purpose is:
 *	Do the things that are done in locore.s in the hp300 version, 
 *		this includes allocation of kernel maps and enabling the MMU.
 * 
 * Some of the code in here is `stolen' from Amiga MACH, and was 
 * written by Bryan Ford and Niklas Hallqvist.
 * 
 * Very crude 68040 support by Michael L. Hitch.
 */

void
start_c(id, fphystart, fphysize, cphysize, esym_addr, flags)
	int id;
	u_int fphystart, fphysize, cphysize;
	char *esym_addr;
	u_int flags;
{
	extern char end[];
	extern void etext();
	extern u_int protorp[2];
	struct cfdev *cd;
	u_int pstart, pend, vstart, vend, avail;
	u_int pt, ptpa, ptsize, ptextra;
	u_int Sysseg_pa, Sysptmap_pa, umap_pa, Sysseg1_pa, Sysptmap1_pa;
	u_int umap1_pa, Sysptmap1, Umap1, sg_proto, pg_proto;
	u_int p0_ptpa, p0_u_area_pa, tc, end_loaded, ncd, i;
	u_int *sg, *pg, *pg2;

	boot_fphystart = fphystart;
	boot_fphysize = fphysize;
	boot_cphysize = cphysize;

	machineid = id;
	chipmem_end = cphysize;
	esym = esym_addr;
	boot_flags = flags;
#ifdef GRF_AGA
	if (flags & 1)
		aga_enable |= 1;
#endif
#ifdef MACHINE_NONCONTIG
	if (flags & (3 << 1))
		noncontig_enable = (flags >> 1) & 3;
#endif

	/* 
	 * the kernel ends at end(), plus the cfdev structures we placed 
	 * there in the loader. Correct for this now.
	 */
	if (esym == NULL) {
		ncfdev = *(int *)end;
		cfdev = (struct cfdev *) ((int)end + 4);
		end_loaded = (u_int)end + 4 + ncfdev * sizeof(struct cfdev);
	} else {
		ncfdev = *(int *)esym;
		cfdev = (struct cfdev *) ((int)esym + 4);
		end_loaded = (u_int)esym + 4 + ncfdev * sizeof(struct cfdev);
	}

	memlist = (struct boot_memlist *)end_loaded;
	end_loaded = (u_int)&memlist->m_seg[memlist->m_nseg];

	/*
	 * Get ZorroII (16-bit) memory if there is any and it's not where the
	 * kernel is loaded.
	 */
	if (memlist->m_nseg > 0 && memlist->m_nseg < 16 && use_z2_mem) {
		struct boot_memseg *sp, *esp;

		sp = memlist->m_seg;
		esp = sp + memlist->m_nseg;
		for (; sp < esp; i++, sp++) {
			if ((sp->ms_attrib & (MEMF_FAST | MEMF_24BITDMA)) 
			    != (MEMF_FAST|MEMF_24BITDMA))
				continue;
			if (sp->ms_start == fphystart)
				continue;
			z2mem_end = sp->ms_start + sp->ms_size;
			z2mem_start = z2mem_end - MAXPHYS * use_z2_mem;
			NZTWOMEMPG = (z2mem_end - z2mem_start) / NBPG;
			if ((z2mem_end - z2mem_start) > sp->ms_size) {
				NZTWOMEMPG = sp->ms_size / NBPG;
				z2mem_start = z2mem_end - sp->ms_size;
			}
			break;
		}
	}

	/*
	 * Scan ConfigDev list and get size of Zorro I/O boards that are
	 * outside the Zorro II I/O area.
	 */
	for (ZBUSAVAIL = 0, cd = cfdev, ncd = ncfdev; ncd > 0; ncd--, cd++) {
		if ((cd->rom.type & 0xe0) == 0x80 ||
		    ((cd->rom.type & 0xe0) == 0xc0 && !isztwopa(cd->addr)))
			ZBUSAVAIL += amiga_round_page(cd->size);
	}

	/*
	 * update these as soon as possible!
	 */
	PAGE_SIZE  = NBPG;
	PAGE_MASK  = NBPG-1;
	PAGE_SHIFT = PG_SHIFT;

	/*
	 * assume KVA_MIN == 0
	 */
	vend   = fphysize;
	avail  = vend;
	vstart = (u_int) end_loaded;
	vstart = amiga_round_page (vstart);
	pstart = vstart + fphystart;
	pend   = vend   + fphystart;
	avail -= vstart;

#ifdef M68040
	if (cpu040) {
		/*
		 * allocate the kernel 1st level segment table
		 */
		Sysseg1_pa = pstart;
		Sysseg1 = vstart;
		vstart += NBPG;
		pstart += NBPG;
		avail -= NBPG;

		/*
		 * allocate the kernel segment table
		 */
		Sysseg_pa = pstart;
		Sysseg = vstart;
		vstart += AMIGA_040RTSIZE / 4 * AMIGA_040STSIZE;
		pstart += AMIGA_040RTSIZE / 4 * AMIGA_040STSIZE;
		avail -= AMIGA_040RTSIZE / 4 * AMIGA_040STSIZE;
	} else
#endif
	{
		/*
		 * allocate the kernel segment table
		 */
		Sysseg = vstart;
		Sysseg_pa = pstart;
		vstart += NBPG;
		pstart += NBPG;
		avail -= NBPG;
	}

	/*
	 * allocate initial page table pages
	 */
	pt = vstart;
	ptpa = pstart;
	ptextra = NCHIPMEMPG + NCIAPG + NZTWOROMPG + NZTWOMEMPG + btoc(ZBUSAVAIL);
	ptsize = (Sysptsize + howmany(ptextra, NPTEPG)) << PGSHIFT;

	vstart += ptsize;
	pstart += ptsize;
	avail -= ptsize;

	/*
	 * allocate kernel page table map
	 */
	Sysptmap = vstart;
	Sysptmap_pa = pstart;
	vstart += NBPG;
	pstart += NBPG;
	avail -= NBPG;

	/*
	 * set Sysmap; mapped after page table pages
	 */
	Sysmap = (u_int *)(ptsize * NPTEPG);

	/*
	 * initialize segment table and page table map
	 */
#ifdef M68040
	if (cpu040) {
		sg_proto = Sysseg_pa | SG_RW | SG_V;
		/*
		 * map all level 1 entries to the segment table
		 */
		sg = (u_int *)Sysseg1_pa;
		while (sg_proto < ptpa) {
			*sg++ = sg_proto;
			sg_proto += AMIGA_040RTSIZE;
		}
		sg_proto = ptpa | SG_RW | SG_V;
		pg_proto = ptpa | PG_RW | PG_CI | PG_V;
		/*
		 * map so many segs
		 */
		sg = (u_int *)Sysseg_pa;
		pg = (u_int *)Sysptmap_pa;
		while (sg_proto < pstart) {
			*sg++ = sg_proto;
			if (pg_proto < pstart)
				*pg++ = pg_proto;
			else if (pg < (u_int *) (Sysptmap_pa + NBPG))
				*pg++ = PG_NV;
			sg_proto += AMIGA_040PTSIZE;
			pg_proto += NBPG;
		}
		/*
		 * invalidate the remainder of the table
		 */
		do {
			*sg++ = SG_NV;
			if (pg < (u_int *)(Sysptmap_pa + NBPG))
				*pg++ = PG_NV;
		} while (sg < (u_int *)(Sysseg_pa + AMIGA_040RTSIZE / 4 * AMIGA_040STSIZE));
		/* the end of the last segment (0xFFFC0000) 
		 * of KVA space is used to map the u-area of
		 * the current process (u + kernel stack).
		 */
		umap_pa  = pstart;
		/*
		 * use next available slot
		 */
		sg_proto = (pstart + NBPG - AMIGA_040PTSIZE) | SG_RW | SG_V;
		umap_pa  = pstart;	/* remember for later map entry */
		/*
		 * enter the page into the level 2 segment table
		 */
		sg = (u_int *)(Sysseg_pa + AMIGA_040RTSIZE / 4 * AMIGA_040STSIZE);
		while (sg_proto > pstart) {
			*--sg = sg_proto;
			sg_proto -= AMIGA_040PTSIZE;
		}
		/*
		 * enter the page into the page table map
		 */
		pg_proto = pstart | PG_RW | PG_CI | PG_V;
		pg = (u_int *) (Sysptmap_pa + 1024);	/*** fix constant ***/
		*--pg = pg_proto;
	} else
#endif /* M68040 */
	{
		sg_proto = ptpa | SG_RW | SG_V;
		pg_proto = ptpa | PG_RW | PG_CI | PG_V;
		/*
		 * map so many segs
		 */
		sg = (u_int *)Sysseg_pa;
		pg = (u_int *)Sysptmap_pa;
		while (sg_proto < pstart) {
			*sg++ = sg_proto;
			*pg++ = pg_proto;
			sg_proto += NBPG;
			pg_proto += NBPG;
		}
		/* 
		 * invalidate the remainder of the tables
		 * (except last entry)
		 */
		do {
			*sg++ = SG_NV;
			*pg++ = PG_NV;
		} while (sg < (u_int *)(Sysseg_pa + AMIGA_STSIZE - 4));

		/*
		 * the end of the last segment (0xFF000000) 
		 * of KVA space is used to map the u-area of
		 * the current process (u + kernel stack).
		 */
		sg_proto = pstart | SG_RW | SG_V; /* use next availabe PA */
		pg_proto = pstart | PG_RW | PG_CI | PG_V;
		umap_pa  = pstart;	/* remember for later map entry */
		/*
		 * enter the page into the segment table 
		 * (and page table map)
		 */
		*sg++     = sg_proto;
		*pg++     = pg_proto;
	}
	/*
	 * invalidate all pte's (will validate u-area afterwards)
	 */
	for (pg = (u_int *) pstart; pg < (u_int *) (pstart + NBPG); )
		*pg++ = PG_NV;
	/*
	 * account for the allocated page
	 */
	pstart   += NBPG;
	vstart   += NBPG;
	avail    -= NBPG;

	/*
	 * record KVA at which to access current u-area PTE(s)
	 */
	Umap = (u_int)Sysmap + AMIGA_MAX_PTSIZE - UPAGES * 4;

	/*
	 * initialize kernel page table page(s) (assume load at VA 0)
	 */
	pg_proto = fphystart | PG_RO | PG_V;	/* text pages are RO */
	pg       = (u_int *) ptpa;
	for (i = 0; i < (u_int) etext; i += NBPG, pg_proto += NBPG)
		*pg++ = pg_proto;

	/* 
	 * data, bss and dynamic tables are read/write
	 */
	pg_proto = (pg_proto & PG_FRAME) | PG_RW | PG_V;
#ifdef M68040
	if (cpu040)
		pg_proto |= PG_CCB;
#endif
	/*
	 * go till end of data allocated so far
	 * plus proc0 PT/u-area (to be allocated)
	 */
	for (; i < vstart + (UPAGES + 1)*NBPG; i += NBPG, pg_proto += NBPG)
		*pg++ = pg_proto;

	/*
	 * invalidate remainder of kernel PT
	 */
	while (pg < (u_int *) (ptpa + ptsize))
		*pg++ = PG_NV;

	/*
	 * go back and validate internal IO PTEs 
	 * at end of allocated PT space
	 */
	pg      -= ptextra;
	pg_proto = CHIPMEMBASE | PG_RW | PG_CI | PG_V;	/* CI needed here?? */
	while (pg_proto < CHIPMEMTOP) {
		*pg++     = pg_proto;
		pg_proto += NBPG;
	}
	if (z2mem_end) {					/* XXX */
		pg_proto = z2mem_start | PG_RW | PG_V;		/* XXX */
		while (pg_proto < z2mem_end) {			/* XXX */
			*pg++ = pg_proto;			/* XXX */
			pg_proto += NBPG;			/* XXX */
		}						/* XXX */
	}							/* XXX */
	pg_proto = CIABASE | PG_RW | PG_CI | PG_V;
	while (pg_proto < CIATOP) {
		*pg++     = pg_proto;
		pg_proto += NBPG;
	}
	pg_proto  = ZTWOROMBASE | PG_RW | PG_CI | PG_V;
	while (pg_proto < ZTWOROMTOP) {
		*pg++     = pg_proto;
		pg_proto += NBPG;
	}

	/*
	 *[ following page tables MAY be allocated to ZORRO3 space,
	 * but they're then later mapped in autoconf.c ]
	 */
	/*
	 * Setup page table for process 0.
	 * We set up page table access for the kernel via Usrptmap (usrpt)
	 * [no longer used?] and access to the u-area itself via Umap (u).
	 * First available page (vstart/pstart) is used for proc0 page table.
	 * Next UPAGES page(s) following are for u-area.
	 */

	p0_ptpa = pstart;
	pstart += NBPG;
	vstart += NBPG;
	avail -= NBPG;

	p0_u_area_pa = pstart;		/* base of u-area and end of PT */

	/*
	 * invalidate entire page table
	 */
	for (pg = (u_int *) p0_ptpa; pg < (u_int *) p0_u_area_pa; )
		*pg++ = PG_NV;

	/*
	 * now go back and validate u-area PTE(s) in PT and in Umap
	 */
	pg -= UPAGES;
	pg2 = (u_int *) (umap_pa + 4*(NPTEPG - UPAGES));
	pg_proto = p0_u_area_pa | PG_RW | PG_V;
#ifdef M68040
	if (cpu040)
		pg_proto |= PG_CCB;
#endif
	for (i = 0; i < UPAGES; i++, pg_proto += NBPG) {
		*pg++  = pg_proto;
		*pg2++ = pg_proto;
	}
	bzero ((u_char *) p0_u_area_pa, UPAGES * NBPG);

	/*
	 * save KVA of proc0 u-area
	 */
	proc0paddr = vstart;
	pstart += UPAGES * NBPG;
	vstart += UPAGES * NBPG;
	avail -= UPAGES * NBPG;

	/*
	 * init mem sizes
	 */
	maxmem  = pend >> PGSHIFT;
	lowram  = fphystart >> PGSHIFT;
	physmem = fphysize >> PGSHIFT;

	/*
	 * get the pmap module in sync with reality.
	 */
	pmap_bootstrap(pstart, fphystart);

	/*
	 * record base KVA of IO spaces which are just before Sysmap
	 */
	CHIPMEMADDR = (u_int)Sysmap - ptextra * NBPG;
	if (z2mem_end == 0)
		CIAADDR = CHIPMEMADDR + NCHIPMEMPG * NBPG;
	else {
		ZTWOMEMADDR = CHIPMEMADDR + NCHIPMEMPG * NBPG;
		CIAADDR   = ZTWOMEMADDR + NZTWOMEMPG * NBPG;
	}
	ZTWOROMADDR  = CIAADDR + NCIAPG * NBPG;
	ZBUSADDR = ZTWOROMADDR + NZTWOROMPG * NBPG;
	CIAADDR += NBPG/2;		 /* not on 8k boundery :-( */
	CUSTOMADDR  = ZTWOROMADDR - ZTWOROMBASE + CUSTOMBASE;
	/*
	 * some nice variables for pmap to use
	 */
	amigahwaddr = CHIPMEMADDR;
	namigahwpg = NCHIPMEMPG + NCIAPG + NZTWOROMPG + NZTWOMEMPG;

	/*
	 * set this before copying the kernel, so the variable is updated in
	 * the `real' place too. protorp[0] is already preset to the 
	 * CRP setting.
	 */
	protorp[1] = Sysseg_pa;		/* + segtable address */

	/*
	 * copy over the kernel (and all now initialized variables) 
	 * to fastram.  DONT use bcopy(), this beast is much larger 
	 * than 128k !
	 */
	{
		register u_int *lp, *le, *fp;

		lp = 0;
		le = (u_int *)end_loaded;
		fp = (u_int *)fphystart;
		while (lp < le)
			*fp++ = *lp++;
	}

	/*
	 * prepare to enable the MMU
	 */
#ifdef M68040
	if (cpu040) {
		/*
		 * movel Sysseg1_pa,a0;
		 * movec a0,SRP;
		 * pflusha;
		 * movel #$0xc000,d0;
		 * movec d0,TC
		 */
		asm volatile ("movel %0,a0; .word 0x4e7b,0x8807"
		    : : "a" (Sysseg1_pa));
		asm volatile (".word 0xf518" : : );
		asm volatile ("movel #0xc000,d0; .word 0x4e7b,0x0003" : : );
	} else
#endif
	{

		/*
		 * setup and load SRP
		 * nolimit, share global, 4 byte PTE's
		 */
		protorp[0] = 0x80000202;
		asm volatile ("pmove %0@,srp" : : "a" (protorp));
		/*
		 * setup and load TC register.
		 * enable_cpr, enable_srp, pagesize=8k,
		 * A = 8 bits, B = 11 bits
		 */
		tc = 0x82d08b00;
		asm volatile ("pmove %0@,tc" : : "a" (&tc));
	}

	/*
	 * to make life easier in locore.s, set these addresses explicitly
	 */
	CIAAbase = CIAADDR + 0x1001;	/* CIA-A at odd addresses ! */
	CIABbase = CIAADDR;
	CUSTOMbase = CUSTOMADDR;
	INTREQRaddr = (vm_offset_t)&custom.intreqr;
	INTREQWaddr = (vm_offset_t)&custom.intreq;

	/*
	 * Get our chip memory allocation system working
	 */
	chipmem_start = CHIPMEMADDR + chipmem_start;
	chipmem_end   = CHIPMEMADDR + chipmem_end;

	if (z2mem_end) {
		z2mem_end = ZTWOMEMADDR + NZTWOMEMPG * NBPG;
		z2mem_start = ZTWOMEMADDR;
	}

	i = *(int *)proc0paddr;
	*(volatile int *)proc0paddr = i;

	/*
	 * disable all interupts but enable allow them to be enabled 
	 * by specific driver code (global int enable bit)
	 */
	custom.intena = 0x7fff;				/* disable ints */
	custom.intena = INTF_SETCLR | INTF_INTEN;	/* but allow them */
	custom.intreq = 0x7fff;			/* clear any current */
	ciaa.icr = 0x7f;			/* and keyboard */
	ciab.icr = 0x7f;			/* and again */

	/*
	 * remember address of read and write intena register for use
	 * by extended spl?() macros.
	 */
	amiga_intena_read  = &custom.intenar;
	amiga_intena_write = &custom.intena;

	/*
	 * This is needed for 3000's with superkick ROM's. Bit 7 of
	 * 0xde0002 enables the ROM if set. If this isn't set the machine
	 * has to be powercycled in order for it to boot again. ICKA! RFH 
	 */
	if (is_a3000()) {
		volatile unsigned char *a3000_magic_reset;

		a3000_magic_reset = (unsigned char *)ztwomap(0xde0002);

		/* Turn SuperKick ROM (V36) back on */
		*a3000_magic_reset |= 0x80;
	}

}


void
rollcolor(color)
	int color;
{
	int s, i;

	s = splhigh();
	/*
	 * need to adjust count -
	 * too slow when cache off, too fast when cache on
	 */
	for (i = 0; i < 400000; i++)
		((volatile struct Custom *)CUSTOMbase)->color[0] = color;
	splx(s);
}

#ifdef DEBUG
void
dump_segtable(stp)
	u_int *stp;
{
	u_int *s, *es;
	int shift, i;

	s = stp;
#ifdef M68040
	if (cpu040) {
		es = s + (AMIGA_040STSIZE >> 2);
		shift = SG4_ISHIFT;
	} else
#endif
	{
		es = s + (AMIGA_STSIZE >> 2);
		shift = SG_ISHIFT;
	}

	/* 
	 * XXX need changes for 68040 
	 */
	for (i = 0; s < es; s++, i++)
		if (*s & SG_V)
			printf("$%08lx: $%08lx\t", i << shift, *s & SG_FRAME);
	printf("\n");
}

void
dump_pagetable(ptp, i, n)
	u_int *ptp, i, n;
{
	u_int *p, *ep;

	p = ptp + i;
	ep = p + n;
	for (; p < ep; p++, i++)
		if (*p & PG_V)
			printf("$%08lx -> $%08lx\t", i, *p & PG_FRAME);
	printf("\n");
}

u_int
vmtophys(ste, vm)
	u_int *ste, vm;
{
	ste = (u_int *) (*(ste + (vm >> SEGSHIFT)) & SG_FRAME);
#ifdef M68040
	if (cpu040)
		ste += (vm & SG4_PMASK) >> PGSHIFT;
	else
#endif
		ste += (vm & SG_PMASK) >> PGSHIFT;
	return((*ste & -NBPG) | (vm & (NBPG - 1)));
}

#endif


/*
 * Kernel reloading code 
 */

static struct exec kernel_exec;
static u_char *kernel_image;
static u_long kernel_text_size, kernel_load_ofs;
static u_long kernel_load_phase;
static u_long kernel_load_endseg;
static u_long kernel_symbol_size, kernel_symbol_esym;

/* This supports the /dev/reload device, major 2, minor 20,
   hooked into mem.c.  Author: Bryan Ford.  */

/*
 * This is called below to find out how much magic storage
 * will be needed after a kernel image to be reloaded.
 */
static int
kernel_image_magic_size()
{
	int sz;

	/* 4 + cfdev's + Mem_Seg's + 4 */
	sz = 8 + ncfdev * sizeof(struct cfdev) 
	    + memlist->m_nseg * sizeof(struct boot_memseg);
	return(sz);
}

/* This actually copies the magic information.  */
static void
kernel_image_magic_copy(dest)
	u_char *dest;
{
	*((int*)dest) = ncfdev;
	dest += 4;
	bcopy(cfdev, dest, ncfdev * sizeof(struct cfdev)
	    + memlist->m_nseg * sizeof(struct boot_memseg) + 4);
}

#undef __LDPGSZ
#define __LDPGSZ 8192 /* XXX ??? */

int
kernel_reload_write(uio)
	struct uio *uio;
{
	extern int eclockfreq;
	struct iovec *iov;
	int error, c;

	iov = uio->uio_iov;

	if (kernel_image == 0) {
		/*
		 * We have to get at least the whole exec header
		 * in the first write.
		 */
		if (iov->iov_len < sizeof(kernel_exec))
			return EFAULT;		/* XXX */

		/*
		 * Pull in the exec header and check it.
		 */
		if (error = uiomove(&kernel_exec, sizeof(kernel_exec), uio))
			return(error);
		printf("loading kernel %d+%d+%d+%d\n", kernel_exec.a_text,
			kernel_exec.a_data, kernel_exec.a_bss,
			esym == NULL ? 0 : kernel_exec.a_syms);
		/*
		 * Looks good - allocate memory for a kernel image.
		 */
		kernel_text_size = (kernel_exec.a_text
			+ __LDPGSZ - 1) & (-__LDPGSZ);
		if (esym != NULL) {
			kernel_symbol_size = kernel_exec.a_syms;
			kernel_symbol_size += 16 * (kernel_symbol_size / 12);
		}
		kernel_image = malloc(kernel_text_size + kernel_exec.a_data
			+ kernel_exec.a_bss
			+ kernel_symbol_size
			+ kernel_image_magic_size(),
			M_TEMP, M_WAITOK);
		kernel_load_ofs = 0;
		kernel_load_phase = 0;
		kernel_load_endseg = kernel_exec.a_text;
		return(0);
	} 
	/*
	 * Continue loading in the kernel image.
	 */
	c = min(iov->iov_len, kernel_load_endseg - kernel_load_ofs);
	c = min(c, MAXPHYS);
	if (error = uiomove(kernel_image + kernel_load_ofs, (int)c, uio))
		return(error);
	kernel_load_ofs += c;

	/*
	 * Fun and games to handle loading symbols - the length of the
	 * string table isn't know until after the symbol table has
	 * been loaded.  We have to load the kernel text, data, and
	 * the symbol table, then get the size of the strings.  A
	 * new kernel image is then allocated and the data currently
	 * loaded moved to the new image.  Then continue reading the
	 * string table.  This has problems if there isn't enough
	 * room to allocate space for the two copies of the kernel
	 * image.  So the approach I took is to guess at the size
	 * of the symbol strings.  If the guess is wrong, the symbol
	 * table is ignored.
	 */

	if (kernel_load_ofs != kernel_load_endseg)
		return(0);

	switch (kernel_load_phase) {
	case 0:		/* done loading kernel text */
		kernel_load_ofs = kernel_text_size;
		kernel_load_endseg = kernel_load_ofs + kernel_exec.a_data;
		kernel_load_phase = 1;
		break;
	case 1:		/* done loading kernel data */
		for(c = 0; c < kernel_exec.a_bss; c++)
			kernel_image[kernel_load_ofs + c] = 0;
		kernel_load_ofs += kernel_exec.a_bss;
		if (esym) {
			kernel_load_endseg = kernel_load_ofs
			    + kernel_exec.a_syms + 8;
			*((u_long *)(kernel_image + kernel_load_ofs)) =
			    kernel_exec.a_syms;
			kernel_load_ofs += 4;
			kernel_load_phase = 3;
			break;
		}
		/*FALLTHROUGH*/
	case 2:		/* done loading kernel */

		/*
		 * Put the finishing touches on the kernel image.
		 */
		kernel_image_magic_copy(kernel_image + kernel_load_ofs);
		bootsync();
		/*
		 * Start the new kernel with code in locore.s.
		 */
		kernel_reload(kernel_image,
		    kernel_load_ofs + kernel_image_magic_size(),
		    kernel_exec.a_entry, boot_fphystart, boot_fphysize,
		    boot_cphysize, kernel_symbol_esym, eclockfreq,
		    boot_flags);
		/*NOTREACHED*/
	case 3:		/* done loading kernel symbol table */
		c = *((u_long *)(kernel_image + kernel_load_ofs - 4));
		if (c > 16 * (kernel_exec.a_syms / 12))
			c = 16 * (kernel_exec.a_syms / 12);
		kernel_load_endseg += c - 4;
		kernel_symbol_esym = kernel_load_endseg;
#ifdef notyet
		kernel_image_copy = kernel_image;
		kernel_image = malloc(kernel_load_ofs + c
		    + kernel_image_magic_size(), M_TEMP, M_WAITOK);
		if (kernel_image == NULL)
			panic("kernel_reload failed second malloc");
		for (c = 0; c < kernel_load_ofs; c += MAXPHYS)
			bcopy(kernel_image_copy + c, kernel_image + c,
			    (kernel_load_ofs - c) > MAXPHYS ? MAXPHYS :
			    kernel_load_ofs - c);
#endif
		kernel_load_phase = 2;
	}
	return(0);
}
