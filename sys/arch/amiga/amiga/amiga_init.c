/* Authors: Markus Wild, Bryan Ford, Niklas Hallqvist 
 *          Michael L. Hitch - initial 68040 support
 *
 *	$Id: amiga_init.c,v 1.16 1994/05/22 07:22:10 chopps Exp $
 */


#include <machine/pte.h>
#include <machine/cpu.h>
#include <sys/param.h>
#include <machine/vmparam.h>

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
#include <vm/pmap.h>
#include <sys/dkbad.h>
#include <sys/reboot.h>
#include <sys/exec.h>

#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h> 

#include <amiga/amiga/cfdev.h>
#include <amiga/amiga/memlist.h>
#include <amiga/dev/ztwobusvar.h>
#include <amiga/dev/zthreebusvar.h>

#ifdef DEBUG
#include <amiga/amiga/color.h>
#define ROLLCOLOR(a) rollcolor(a)
void rollcolor (u_int);
#else
#define ROLLCOLOR(a)
#endif

extern int	machineid, mmutype;
extern u_int 	lowram;
extern u_int	Sysptmap, Sysptsize, Sysseg, Umap, proc0paddr;
extern u_int	Sysseg1;
extern u_int	virtual_avail;

extern char *esym;

/* virtual addresses specific to the AMIGA */
u_int CIAADDR, CUSTOMADDR; /* SCSIADDR;*/

u_int CHIPMEMADDR;

/* some addresses used often, for performance */
caddr_t	INTREQRaddr, INTREQWaddr;
/* pre-initialize these variables, so we can access the hardware while
   running un-mapped. */
caddr_t	CIAAbase = (caddr_t)0xbfe001, CIABbase = (caddr_t)0xbfd000;
volatile struct Custom *CUSTOMbase = (volatile struct Custom *) 0xdff000;

struct Mem_List *mem_list;

void *chipmem_start = (void *)0x400, *chipmem_end;

void *z2mem_start, *z2mem_end;		/* XXX */
int use_z2_mem = 1;			/* XXX */

/* Called by the console et al to steal chip memory during initialization */
void *
chipmem_steal(amount)
	long amount;
{
  /* steal from top of chipmem, so we don't collide with the kernel loaded
     into chipmem in the not-yet-mapped state. */
  void *ptr = chipmem_end - amount;
  if ((long)ptr & 1)
    ptr = (void *) ((long)ptr - 1);
  chipmem_end = ptr;
  if(chipmem_start > chipmem_end)
    panic("not enough chip memory");
  return(ptr);
}

/* Called to allocate 24-bit DMAable memory (ZorroII or chip).  Used by
   A2091 and GVP11 scsi driver as a dma bounce buffer.  If use_z2_mem
   is patched to 0, then only chip memory will be allocated. */

void *
alloc_z2mem(amount)			/* XXX */
	long amount;
{					/* XXX */
	if (use_z2_mem && z2mem_end && (z2mem_end - amount) >= z2mem_start) {
		z2mem_end -= amount;
		return ((void *) z2mem_end);
	}
	return ((void *) alloc_chipmem(amount));	/* XXX */
}					/* XXX */


/* This stuff is for reloading new kernels through /dev/reload.  */
static struct exec kernel_exec;
static u_char *kernel_image;
static u_long kernel_text_size, kernel_load_ofs;
static u_long kernel_load_phase;
static u_long kernel_load_endseg;
static u_long kernel_symbol_size, kernel_symbol_esym;

u_long orig_fastram_start, orig_fastram_size, orig_chipram_size;


/* this is the C-level entry function, it's called from locore.s.
   Preconditions:
   - Interrupts are disabled
   - PA == VA, we don't have to relocate addresses before enabling
     the MMU
   - Exec is no longer available (because we're loaded all over 
     low memory, no ExecBase is available anymore)
     
   It's purpose is:
   - Do the things that are done in locore.s in the hp300 version, 
     this includes allocation of kernel maps and enabling the MMU.
     
   Some of the code in here is `stolen' from Amiga MACH, and was 
   written by Bryan Ford and Niklas Hallqvist.

   Very crude 68040 support by Michael L. Hitch.
 */

u_int cache_copyback = PG_CC;		/* patchable to disable copyback cache */

void
start_c(id, fastram_start, fastram_size, chipram_size, esym_addr)
	int id;
	u_int fastram_start, fastram_size, chipram_size;
	char *esym_addr;
{
  extern char end[];
  extern void etext();
  extern u_int protorp[2];
  u_int pstart, pend, vstart, vend, avail;
  u_int Sysseg_pa, Sysptmap_pa, umap_pa;
  u_int Sysseg1_pa, Sysptmap1_pa, umap1_pa;
  u_int Sysptmap1, Umap1;
  u_int pagetable, pagetable_pa, pagetable_size, pagetable_extra;
  u_int sg_proto, pg_proto, *sg, *pg, *pg2, i;
  u_int p0_pagetable_pa, p0_u_area_pa;
  u_int tc;
  u_int hw_va;
  u_int chip_pt, cia_pt;
#if 0
  u_int custom_pt, scsi_pt;
#else
  u_int zorro2_pt;
#endif
  u_int end_loaded;
  u_int ncd;
  struct cfdev *cd;

  orig_fastram_start = fastram_start;
  orig_fastram_size = fastram_size;
  orig_chipram_size = chipram_size;

  machineid = id;

  chipmem_end = (void *)chipram_size;

  esym = esym_addr;

  /* the kernel ends at end(), plus the ConfigDev structures we placed 
     there in the loader. Correct for this now. */
  if (esym == NULL) {
    ncfdev = *(int *)end;
    cfdev = (struct cfdev *) ((int)end + 4);
    end_loaded = (u_int)end + 4 + ncfdev * sizeof(struct cfdev);
  } else {
    ncfdev = *(int *)esym;
    cfdev = (struct cfdev *) ((int)esym + 4);
    end_loaded = (u_int)esym + 4 + ncfdev * sizeof(struct cfdev);
  }

  mem_list = (struct Mem_List *) end_loaded;
  end_loaded = (u_int)&mem_list->mem_seg[mem_list->num_mem];
  
  /* Get ZorroII (16-bit) memory if there is any and it's not where the
     kernel is loaded. */
  if (mem_list->num_mem > 0 && mem_list->num_mem < 16) {
    for (i = 0; i < mem_list->num_mem; i++) {
      if ((mem_list->mem_seg[i].mem_attrib & (MEMF_FAST|MEMF_24BITDMA)) !=
        (MEMF_FAST|MEMF_24BITDMA))
        continue;
      if (mem_list->mem_seg[i].mem_start == fastram_start)
        continue;
      z2mem_start = (void *)mem_list->mem_seg[i].mem_start;
      ZTWOMEMSIZE = mem_list->mem_seg[i].mem_size;
      z2mem_end = z2mem_start + mem_list->mem_seg[i].mem_size;
      break;
    }
  }

  /* look for Z3 boards. For now, no Z3 RAM-extensions are supported, just
     I/O boards */
  for (ZTHREEAVAIL = 0, cd = cfdev, ncd = ncfdev; 
       ncd > 0; 
       ncd--, cd++)
    if ((u_int) cd->addr >= ZTHREEBASE && (u_int) cd->addr < ZTHREETOP)
      ZTHREEAVAIL += amiga_round_page (cd->size);

#if 0
  /* XXX */
  {
    extern int boothowto;
    boothowto = RB_SINGLE;
  }
#endif

  /* update these as soon as possible! */
  PAGE_SIZE  = NBPG;
  PAGE_MASK  = NBPG-1;
  PAGE_SHIFT = PG_SHIFT;

  /* assume KVA_MIN == 0 */
  vend   = fastram_size;
  avail  = vend;
  vstart = (u_int) end_loaded;
  vstart = amiga_round_page (vstart);
  pstart = vstart + fastram_start;
  pend   = vend   + fastram_start;
  avail -= vstart;
  
  if (cpu040) {
    /* allocate the kernel 1st level segment table */
    Sysseg1   = vstart;
    Sysseg1_pa = pstart;
    vstart   += AMIGA_PAGE_SIZE;
    pstart   += AMIGA_PAGE_SIZE;
    avail    -= AMIGA_PAGE_SIZE;

    /* allocate the kernel segment table */
    Sysseg    = vstart;
    Sysseg_pa = pstart;
    vstart   += AMIGA_PAGE_SIZE * 8;
    pstart   += AMIGA_PAGE_SIZE * 8;
    avail    -= AMIGA_PAGE_SIZE * 8;
  }
  else {

    /* allocate the kernel segment table */
    Sysseg    = vstart;
    Sysseg_pa = pstart;
    vstart   += AMIGA_PAGE_SIZE;
    pstart   += AMIGA_PAGE_SIZE;
    avail    -= AMIGA_PAGE_SIZE;
  }
  
  /* allocate initial page table pages */
  pagetable       = vstart;
  pagetable_pa    = pstart;
#if 0
  pagetable_extra = CHIPMEMSIZE + CIASIZE + CUSTOMSIZE + SCSISIZE;
#else
    pagetable_extra = CHIPMEMSIZE + CIASIZE + ZTWOROMSIZE;
#endif
  pagetable_extra += (ZTWOMEMSIZE) / AMIGA_PAGE_SIZE; /* XXX */
  if (ZTHREEAVAIL > 0)
    pagetable_extra += btoc (ZTHREEAVAIL);
  pagetable_size  = (Sysptsize + (pagetable_extra + NPTEPG-1)/NPTEPG) << PGSHIFT;
  vstart         += pagetable_size;
  pstart         += pagetable_size;
  avail          -= pagetable_size;
  
  /* allocate kernel page table map */
  Sysptmap    = vstart;
  Sysptmap_pa = pstart;
  vstart   += AMIGA_PAGE_SIZE;
  pstart   += AMIGA_PAGE_SIZE;
  avail    -= AMIGA_PAGE_SIZE;

  /* set Sysmap; mapped after page table pages */
  Sysmap = (typeof (Sysmap)) (pagetable_size << (cpu040 ? 11 : (SEGSHIFT - PGSHIFT)));

  /* initialize segment table and page table map */
  if (cpu040) {
    sg_proto = Sysseg_pa | SG_RW | SG_V;
    /* map all level 1 entries to the segment table */
    for (sg = (u_int *) Sysseg1_pa; 
         sg_proto < (u_int) pagetable_pa;
         sg_proto += AMIGA_040RTSIZE)
      {
        *sg++ = sg_proto;
      }

    sg_proto = pagetable_pa | SG_RW | SG_V;
    pg_proto = pagetable_pa | PG_RW | PG_CI | PG_V;
    /* map so many segs */
    for (sg = (u_int *) Sysseg_pa, pg = (u_int *) Sysptmap_pa; 
         sg_proto < (u_int) pstart;
         sg_proto += AMIGA_040PTSIZE, pg_proto += AMIGA_PAGE_SIZE)
      {
        *sg++ = sg_proto;
        if (pg_proto < pstart)
          *pg++ = pg_proto;
        else if (pg < (u_int *) (Sysptmap_pa + AMIGA_PAGE_SIZE))
          *pg++ = PG_NV;
      }
    /* and invalidate the remainder of the table */
    do
      {
        *sg++ = SG_NV;
        if (pg < (u_int *) (Sysptmap_pa + AMIGA_PAGE_SIZE))
          *pg++ = PG_NV;
      }
    while (sg < (u_int *) (Sysseg_pa + AMIGA_PAGE_SIZE * 8));
  }
  else {
    sg_proto = pagetable_pa | SG_RW | SG_V;
    pg_proto = pagetable_pa | PG_RW | PG_CI | PG_V;
    /* map so many segs */
    for (sg = (u_int *) Sysseg_pa, pg = (u_int *) Sysptmap_pa; 
         sg_proto < pstart;
         sg_proto += AMIGA_PAGE_SIZE, pg_proto += AMIGA_PAGE_SIZE)
      {
        *sg++ = sg_proto;
        *pg++ = pg_proto;
      }
    /* and invalidate the remainder of the tables (except last entry) */
    do
      {
        *sg++ = SG_NV;
        *pg++ = PG_NV;
      }
    while (sg < (u_int *) (Sysseg_pa + AMIGA_STSIZE-4));
  }

  if (cpu040) {
    /* the end of the last segment (0xFFFC0000) of KVA space is used to 
       map the u-area of the current process (u + kernel stack). */
    umap_pa  = pstart;

    sg_proto = (pstart + AMIGA_PAGE_SIZE - AMIGA_040PTSIZE) | SG_RW | SG_V;	/* use next availabe PA */
    umap_pa  = pstart;	/* remember for later map entry */
    /* enter the page into the level 2 segment table */
    for (sg = (u_int *) (Sysseg_pa + AMIGA_PAGE_SIZE * 8);
         sg_proto > (u_int) pstart;
         sg_proto -= AMIGA_040PTSIZE)
      *--sg     = sg_proto;
    /* enter the page into the page table map */
    pg_proto = pstart | PG_RW | PG_CI | PG_V;
    pg = (u_int *) (Sysptmap_pa + 1024);		/*** fix constant ***/
    *--pg = pg_proto;
    /* invalidate all pte's (will validate u-area afterwards) */
    for (pg = (u_int *) pstart; 
         pg < (u_int *) (pstart + AMIGA_PAGE_SIZE);
         )
      *pg++ = PG_NV;
    /* account for the allocated page */
    pstart   += AMIGA_PAGE_SIZE;
    vstart   += AMIGA_PAGE_SIZE;
    avail    -= AMIGA_PAGE_SIZE;

    /* record KVA at which to access current u-area PTE(s) */
    Umap = (u_int) Sysmap + AMIGA_MAX_PTSIZE - UPAGES*4;
  }
  else {
    /* the end of the last segment (0xFF000000) of KVA space is used to 
       map the u-area of the current process (u + kernel stack). */
    sg_proto = pstart | SG_RW | SG_V;	/* use next availabe PA */
    pg_proto = pstart | PG_RW | PG_CI | PG_V;
    umap_pa  = pstart;	/* remember for later map entry */
    /* enter the page into the segment table (and page table map) */
    *sg++     = sg_proto;
    *pg++     = pg_proto;
    /* invalidate all pte's (will validate u-area afterwards) */
    for (pg = (u_int *) pstart; 
         pg < (u_int *) (pstart + AMIGA_PAGE_SIZE);
         )
      *pg++ = PG_NV;
    /* account for the allocated page */
    pstart   += AMIGA_PAGE_SIZE;
    vstart   += AMIGA_PAGE_SIZE;
    avail    -= AMIGA_PAGE_SIZE;

    /* record KVA at which to access current u-area PTE(s) */
    Umap = (u_int) Sysmap + AMIGA_MAX_PTSIZE - UPAGES*4;
  }
  
  /* initialize kernel page table page(s) (assume load at VA 0) */
  pg_proto = fastram_start | PG_RO | PG_V;	/* text pages are RO */
  pg       = (u_int *) pagetable_pa;
  for (i = 0; 
       i < (u_int) etext;
       i += AMIGA_PAGE_SIZE, pg_proto += AMIGA_PAGE_SIZE)
    *pg++ = pg_proto;

  /* data, bss and dynamic tables are read/write */
  pg_proto = (pg_proto & PG_FRAME) | PG_RW | PG_V;
  if (cpu040)
    pg_proto |= cache_copyback;
  /* go till end of data allocated so far plus proc0 PT/u-area (to be allocated) */
  for (; 
       i < vstart + (UPAGES + 1)*AMIGA_PAGE_SIZE;
       i += AMIGA_PAGE_SIZE, pg_proto += AMIGA_PAGE_SIZE)
    *pg++ = pg_proto;

  /* invalidate remainder of kernel PT */
  while (pg < (u_int *) (pagetable_pa + pagetable_size))
    *pg++ = PG_NV;

  /* go back and validate internal IO PTEs at end of allocated PT space */
  pg      -= pagetable_extra;
  chip_pt  = (u_int) pg;
  pg_proto = CHIPMEMBASE | PG_RW | PG_CI | PG_V;	/* CI needed here?? */
  while (pg_proto < CHIPMEMTOP)
    {
      *pg++     = pg_proto;
      pg_proto += AMIGA_PAGE_SIZE;
    }
  if (z2mem_end) {				/* XXX */
    pg_proto = (u_int) z2mem_start | PG_RW | PG_V;	/* XXX */
    while (pg_proto < (u_int) z2mem_end)		/* XXX */
      {						/* XXX */
        *pg++ = pg_proto;			/* XXX */
        pg_proto += AMIGA_PAGE_SIZE;		/* XXX */
      }						/* XXX */
  }						/* XXX */
  pg_proto = CIABASE | PG_RW | PG_CI | PG_V;
  cia_pt  = (u_int) pg;
  while (pg_proto < CIATOP)
    {
      *pg++     = pg_proto;
      pg_proto += AMIGA_PAGE_SIZE;
    }
#if 0
  pg_proto  = CUSTOMBASE | PG_RW | PG_CI | PG_V;
  custom_pt = (u_int) pg;
  while (pg_proto < CUSTOMTOP)
    {
      *pg++     = pg_proto;
      pg_proto += AMIGA_PAGE_SIZE;
    }
  pg_proto = SCSIBASE | PG_RW | PG_CI | PG_V;
  scsi_pt  = (u_int) pg;
  while (pg_proto < SCSITOP)
    {
      *pg++     = pg_proto;
      pg_proto += AMIGA_PAGE_SIZE;
    }
#else
  pg_proto  = ZTWOROMBASE | PG_RW | PG_CI | PG_V;
  zorro2_pt = (u_int) pg;
  while (pg_proto < ZTWOROMTOP)
    {
      *pg++     = pg_proto;
      pg_proto += AMIGA_PAGE_SIZE;
    }
#endif

  /* following page tables MAY be allocated to ZORRO3 space, but they're
     then later mapped in autoconf.c */

  /* Setup page table for process 0.
  
     We set up page table access for the kernel via Usrptmap (usrpt)
     [no longer used?] and access to the u-area itself via Umap (u).
     First available page (vstart/pstart) is used for proc0 page table.
     Next UPAGES page(s) following are for u-area. */
  
  p0_pagetable_pa = pstart;
  pstart	 += AMIGA_PAGE_SIZE;
  vstart         += AMIGA_PAGE_SIZE;	/* keep VA in sync */
  avail          -= AMIGA_PAGE_SIZE;

  p0_u_area_pa	  = pstart;		/* base of u-area and end of PT */
  
  /* invalidate entire page table */
  for (pg = (u_int *) p0_pagetable_pa; pg < (u_int *) p0_u_area_pa; )
    *pg++ = PG_NV;

  /* now go back and validate u-area PTE(s) in PT and in Umap */
  pg      -= UPAGES;
  pg2      = (u_int *) (umap_pa + 4*(NPTEPG - UPAGES));
  pg_proto = p0_u_area_pa | PG_RW | PG_V;
  if (cpu040)
    pg_proto |= cache_copyback;
  for (i = 0; i < UPAGES; i++, pg_proto += AMIGA_PAGE_SIZE)
    {
      *pg++  = pg_proto;
      *pg2++ = pg_proto;
    }
  bzero ((u_char *) p0_u_area_pa, UPAGES * AMIGA_PAGE_SIZE);  /* clear process 0 u-area */

  /* save KVA of proc0 u-area */
  proc0paddr = vstart;
  pstart    += UPAGES * AMIGA_PAGE_SIZE;
  vstart    += UPAGES * AMIGA_PAGE_SIZE;
  avail     -= UPAGES * AMIGA_PAGE_SIZE;

  /* init mem sizes */
  maxmem  = pend >> PGSHIFT;
  lowram  = fastram_start >> PGSHIFT;
  physmem = fastram_size >> PGSHIFT;
  
  /* get the pmap module in sync with reality. */
  pmap_bootstrap (pstart, fastram_start);

  /* record base KVA of IO spaces which are just before Sysmap */
  CHIPMEMADDR = (u_int)Sysmap - pagetable_extra * AMIGA_PAGE_SIZE;
  if (z2mem_end) {				/* XXX */
    ZTWOMEMADDR = CHIPMEMADDR + CHIPMEMSIZE*AMIGA_PAGE_SIZE;
    CIAADDR   = ZTWOMEMADDR + ZTWOMEMSIZE;
  }						/* XXX */
  else						/* XXX */
    CIAADDR     = CHIPMEMADDR + CHIPMEMSIZE*AMIGA_PAGE_SIZE;
  ZTWOROMADDR  = (caddr_t) CIAADDR + CIASIZE*AMIGA_PAGE_SIZE;
  ZTHREEADDR  = (caddr_t) ZTWOROMADDR + ZTWOROMSIZE*AMIGA_PAGE_SIZE;
  CIAADDR    += AMIGA_PAGE_SIZE/2; /* not on 8k boundery :-( */

  /* just setup the custom chips address, other addresses (like SCSI on
     A3000, clock, etc.) moved to device drivers where they belong */
  CUSTOMADDR  = (u_int) ZTWOROMADDR - ZTWOROMBASE + CUSTOMBASE;

#if 0
  CUSTOMADDR  = CIAADDR + CIASIZE*AMIGA_PAGE_SIZE;
  SCSIADDR    = CUSTOMADDR + CUSTOMSIZE*AMIGA_PAGE_SIZE;
  CIAADDR    += AMIGA_PAGE_SIZE/2; /* not on 8k boundery :-( */
  CUSTOMADDR += AMIGA_PAGE_SIZE/2; /* not on 8k boundery */
#endif

  /* set this before copying the kernel, so the variable is updated in
     the `real' place too. protorp[0] is already preset to the CRP setting. */
  protorp[1] = Sysseg_pa;	/* + segtable address */

  /* copy over the kernel (and all now initialized variables) to fastram.
     DONT use bcopy(), this beast is much larger than 128k ! */
  {
    register u_int *lp = 0, *le = (u_int *) end_loaded, *fp = (u_int *) fastram_start;
    while (lp < le)
      *fp++ = *lp++;
  }

  /* prepare to enable the MMU */

  if (cpu040) {
    /* movel Sysseg1_pa,a0; movec a0,SRP; pflusha; movel #$0xc000,d0; movec d0,TC */
    asm volatile ("movel %0,a0; .word 0x4e7b,0x8807" : : "a" (Sysseg1_pa));
    asm volatile (".word 0xf518" : : );
    asm volatile ("movel #0xc000,d0; .word 0x4e7b,0x0003" : : );
  }
  else {
    /* create SRP */
    protorp[0] = 0x80000202;	/* nolimit + share global + 4 byte PTEs */
    /*protorp[1] = Sysseg_pa;*/	/* + segtable address */
    /* load SRP into MMU */
    asm volatile ("pmove %0@,srp" : : "a" (protorp));
    /* setup TC register. */
    tc = 0x82d08b00;	/* enable_cpr, enable_srp, pagesize=8k, A=8bits, B=11bits */
    /* load it */
    asm volatile ("pmove %0@,tc" : : "a" (&tc));
  }

  /* to make life easier in locore.s, set these addresses explicitly */
  CIAAbase = (caddr_t) CIAADDR + 0x1001;	/* CIA-A is at odd addresses ! */
  CIABbase = (caddr_t) CIAADDR;
  CUSTOMbase = (volatile struct Custom *) CUSTOMADDR;
  INTREQRaddr = (caddr_t) & custom.intreqr;
  INTREQWaddr = (caddr_t) & custom.intreq;

  /* Get our chip memory allocation system working */
  chipmem_start = (void *) (CHIPMEMADDR + (int) chipmem_start);
  chipmem_end   = (void *) (CHIPMEMADDR + (int) chipmem_end);

  if (z2mem_end) {				/* XXX */
    z2mem_end = (void *) (ZTWOMEMADDR + ZTWOMEMSIZE);
    z2mem_start = (void *) ZTWOMEMADDR;		/* XXX */
  }						/* XXX */

  i = *(int *)proc0paddr;
  *(volatile int *)proc0paddr = i;
/*  printf ("Running mapped now! Virtual_avail = $%lx\n", virtual_avail);*/

  /* Do these things here, much easier than in assembly in locore.s ;-)

     We shut off any interrupts, but allow interrupts in general (INTEN).
     This way, drivers can enable those interrupts they want to have
     enabled, without having to temper with the global interrupt 
     enable/disable bit. */

  custom.intena = 0x7fff;
  
  /* enable ints (none currently selected though) */
  custom.intena = INTF_SETCLR | INTF_INTEN;

  /* clear any possibly pending interrupts */
  custom.intreq = 0x7fff;
  /* really! (-> bloody keyboard!) */
  ciaa.icr = 0x7f;
  ciab.icr = 0x7f;

  /* return to locore.s, and let BSD take over */
}


void
rollcolor(color)
	u_int color;
{
  int s, i;

  s = splhigh();
  /* need to adjust count - too slow when cache off, too fast when cache on */
  for (i = 0; i < 400000; i++)
    CUSTOMbase->color[0] = color;
  splx (s);
}

#ifdef DEBUG
void
dump_segtable(ste)
	struct ste *ste;
{
  int i;

/* XXX need changes for 68040 */
  for (i = 0; i < (cpu040 ? AMIGA_040STSIZE : AMIGA_STSIZE)>>2; i++, ste++)
    {
      if (ste->sg_v == SG_V)
	{
          printf ("$%08lx -> ", i << (cpu040 ? SG_040ISHIFT : SG_ISHIFT));
	  printf ("$%08lx\t", (*(u_int *)ste) & SG_FRAME);
	}
    }
  printf ("\n");
}

void
dump_pagetable(voff, pte, num_pte)
	caddr_t voff;
	struct pte *pte;
	int num_pte;
{
  int i;

  while (num_pte--)
    {
      if (pte->pg_v == PG_V)
	{
          printf ("$%08lx -> ", voff);
	  printf ("$%08lx\t", (*(u_int *)pte) & PG_FRAME);
	}
    }
  printf ("\n");
}

u_int
vmtophys(ste, vm)
	u_int *ste, vm;
{
  u_int *s;

  s = (u_int *)((*(ste + (vm >> SEGSHIFT))) & SG_FRAME);
  s += (vm & (cpu040 ? SG_040PMASK : SG_PMASK)) >> PGSHIFT;

  return (*s & (-AMIGA_PAGE_SIZE)) | (vm & (AMIGA_PAGE_SIZE-1));
}

#endif


/*** Kernel reloading code ***/
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
	    + mem_list->num_mem * sizeof(struct Mem_Seg);
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
    + mem_list->num_mem*sizeof(struct Mem_Seg) + 4);
}

#undef __LDPGSZ
#define __LDPGSZ 8192 /* XXX ??? */

int
kernel_reload_write(uio)
	struct uio *uio;
{
  struct iovec *iov;
  int error;

  iov = uio->uio_iov;
  
  if (kernel_image == 0)
    {
      /* We have to get at least the whole exec header
	 in the first write.  */
      if (iov->iov_len < sizeof(kernel_exec))
	return EFAULT;		/* XXX */

      /* Pull in the exec header and check it.  */
      error = uiomove(&kernel_exec, sizeof(kernel_exec), uio);
      if (error == 0)
	{
#if 0 /* XXX??? */
	  if (kernel_exec.a_magic != NMAGIC)
	    return EFAULT;	/* XXX */
#endif
	  printf("loading kernel %d+%d+%d+%d\n", kernel_exec.a_text,
	    kernel_exec.a_data, kernel_exec.a_bss,
	    esym == NULL ? 0 : kernel_exec.a_syms);

	  /* Looks good - allocate memory for a kernel image.  */
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
	}
    }
  else
    {
      int c;

      /* Continue loading in the kernel image.  */
      c = MIN(iov->iov_len, kernel_load_endseg - kernel_load_ofs);
      c = MIN(c, MAXPHYS);
      error = uiomove(kernel_image + kernel_load_ofs, (int)c, uio);

      if (error == 0)
	{
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

	  if (kernel_load_ofs == kernel_load_endseg)
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
		kernel_load_endseg = kernel_load_ofs + kernel_exec.a_syms + 8;
		*((u_long *)(kernel_image + kernel_load_ofs)) =
		  kernel_exec.a_syms;
		kernel_load_ofs += 4;
		kernel_load_phase = 3;
		break;
	      }
	      /* Fall Through */
	    case 2:		/* done loading kernel */

	      /* Put the finishing touches on the kernel image.  */
	      kernel_image_magic_copy(kernel_image + kernel_load_ofs);

	      /* Get everything in a clean state for rebooting.  */
	      bootsync();

	      /* Start the new kernel with code in locore.s.  */
	      kernel_reload(kernel_image,
			    (kernel_load_ofs + kernel_image_magic_size()),
			    kernel_exec.a_entry,
			    orig_fastram_start, orig_fastram_size,
			    orig_chipram_size, kernel_symbol_esym);
	      /* Not reached */
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
	}
    }
  return error;
}

