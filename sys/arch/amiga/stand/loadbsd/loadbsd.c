/*
 *	$Id: loadbsd.c,v 1.11 1994/06/13 08:13:16 chopps Exp $
 */

#include <sys/types.h>
#include <a.out.h>
#include <stdio.h>
#include <unistd.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <libraries/configregs.h>
#include <libraries/expansionbase.h>
#include <graphics/gfxbase.h>

#include <inline/exec.h>
#include <inline/expansion.h>
#include <inline/graphics.h>

/* Get definitions for boothowto */
#include "reboot.h"

static char usage[] =
"
NAME
\t%s - loads NetBSD from amiga dos.
SYNOPSIS
\t%s some-vmunix [-a] [-b] [-k] [-m memory] [-p] [-t] [-V]
OPTIONS
\t-a  Boot up to multiuser mode.
\t-b  Ask for which root device.
\t    Its possible to have multiple roots and choose between them.
\t-k  Reserve the first 4M of fast mem [Some one else
\t    is going to have to answer what that it is used for].
\t-m  Tweak amount of available memory, for finding minimum amount
\t    of memory required to run. Sets fastmem size to specified
\t    size in Kbytes.
\t-p  Use highest priority fastmem segement instead of the largest
\t    segment. The higher priority segment is usually faster
\t    (i.e. 32 bit memory), but some people have smaller amounts
\t    of 32 bit memory.
\t-t  This is a *test* option.  It prints out the memory
\t    list information being passed to the kernel and also
\t    exits without actually starting NetBSD.
\t-S  Include kernel symbol table.
\t-D  Enter debugger
\t-V  Version of loadbsd program.
\t-c  Set machine type.
HISTORY
      This version supports Kernel version 720 +
";

struct ExpansionBase *ExpansionBase;
struct GfxBase *GfxBase;

#undef __LDPGSZ
#define __LDPGSZ 8192

#define MAX_MEM_SEG	16

/*
 * Kernel parameter passing version
 *	1:	first version of loadbsd
 *	2:	needs esym location passed in a4
 */
#define KERNEL_PARAMETER_VERSION	2

/*
 *	Version history:
 *	1.x	Kernel parameter passing version check.
 *	2.0	Added symbol table end address and symbol table support.
 *	2.1	03/23/94 - Round up end of fastram segment.
 *		Check fastram segment size for minimum of 2M.
 *		Use largest segment of highest priority if -p option.
 *		Print out fastram size in KB if not a multiple of MB.
 *	2.2	03/24/94 - Zero out all unused registers.
 *		Started version history comment.
 *	2.3	04/26/94 - Added -D option to enter debugger on boot.
 *	2.4	04/30/94 - Cpuid includes base machine type.
 *		Also check if CPU is capable of running NetBSD.
 *	2.5	05/17/94 - Add check for "A3000 bonus".
 *	2.6	06/05/94 - Added -c option to override machine type.
 */

struct MEM_LIST {
	u_long	num_mem;
	struct MEM_SEG {
		u_long	mem_start;
		u_long	mem_size;
		u_short	mem_attrib;
		short	mem_prio;
	} mem_seg[MAX_MEM_SEG];
} mem_list, *kmem_list;

int k_opt;
int a_opt;
int b_opt;
int p_opt;
int t_opt;
int m_opt;
int S_opt;
int D_opt;

u_long cpuid;

extern char *optarg;
extern int optind;

void get_mem_config (void **fastmem_start, u_long *fastmem_size, u_long *chipmem_size);
void get_cpuid (void);
void Usage (char *program_name);
void Version (void);

static const char _version[] = "$VER: LoadBSD 2.6 (5.6.94)";

int
main (int argc, char *argv[])
{
  struct exec e;
  int fd;
  int boothowto = RB_SINGLE;

  if (argc >= 2)
    {
      if ((fd = open (argv[1], 0)) >= 0)
	{
	  if (read (fd, &e, sizeof (e)) == sizeof (e))
	    {
	      if (e.a_magic == NMAGIC)
		{
		  u_char *kernel;
		  int kernel_size;
		  int text_size;
		  struct ConfigDev *cd;
		  int num_cd;
		  void *fastmem_start;
		  u_long fastmem_size, chipmem_size;
		  int i;
		  u_short *kern_vers;
		  char *esym;
		  int string_size;

		  GfxBase = (struct GfxBase *) OpenLibrary ("graphics.library", 0);
		  if (! GfxBase)	/* not supposed to fail... */
		    abort();
		  ExpansionBase= (struct ExpansionBase *) OpenLibrary ("expansion.library", 0);
		  if (! ExpansionBase)	/* not supposed to fail... */
		    abort();
		  optind = 2;
		  while ((i = getopt (argc, argv, "kabptVm:SDc:")) != EOF)
		    switch (i) {
		    case 'k':
		      k_opt = 1;
		      break;
		    case 'a':
		      a_opt = 1;
		      break;
		    case 'b':
		      b_opt = 1;
		      break;
		    case 'p':
		      p_opt = 1;
		      break;
		    case 't':
		      t_opt = 1;
		      break;
		    case 'm':
		      m_opt = atoi (optarg) * 1024;
		      break;
		    case 'V':
		      Version();
		      break;
		    case 'S':
		      S_opt = 1;
		      break;
		    case 'D':
		      D_opt = 1;
		      break;
		    case 'c':
		      cpuid = atoi (optarg) << 16;
		      break;
		    default:
		      Usage(argv[0]);
		      fprintf(stderr,"Unrecognized option \n");
		      exit(-1);
		    }

		  for (cd = 0, num_cd = 0; cd = FindConfigDev (cd, -1, -1); num_cd++) ;
		  get_mem_config (&fastmem_start, &fastmem_size, &chipmem_size);
		  get_cpuid ();

		  text_size = (e.a_text + __LDPGSZ - 1) & (-__LDPGSZ);
		  esym = NULL;
		  kernel_size = text_size + e.a_data + e.a_bss
		      + num_cd*sizeof(*cd) + 4
		      + mem_list.num_mem*sizeof(struct MEM_SEG) + 4;
		  /*
		   * get symbol table size & string size
		   * (should check kernel version to see if it will handle it)
		   */
		  if (S_opt && e.a_syms) {
		    S_opt = 0;			/* prepare for failure */
		    if (lseek(fd, e.a_text + e.a_data + e.a_syms, SEEK_CUR) > 0) {
		      if (read (fd, &string_size, 4) == 4) {
			if (lseek(fd, sizeof(e), SEEK_SET) < 0) {
		      	  printf ("Error repositioning to text\n");
		      	  exit(0);		/* Give up! */
			}
			kernel_size += e.a_syms + 4 + string_size;
			S_opt = 1;		/* sucess!  Keep -S option */
		      }
		    }
		  }

		  kernel = (u_char *) malloc (kernel_size);

		  if (t_opt)
		    for (i = 0; i < mem_list.num_mem; ++i) {
		      printf ("mem segment %d: start=%08lx size=%08lx attribute=%04lx pri=%d\n",
			i + 1, mem_list.mem_seg[i].mem_start,
			mem_list.mem_seg[i].mem_size,
			mem_list.mem_seg[i].mem_attrib,
			mem_list.mem_seg[i].mem_prio);
		    }

		  if (kernel)
		    {
		      if (read (fd, kernel, e.a_text) == e.a_text
			  && read (fd, kernel + text_size, e.a_data) == e.a_data)
			{
			  int *knum_cd;
			  struct ConfigDev *kcd;
			  int mem_ix;

			  if (k_opt)
			    {
			      fastmem_start += 4*1024*1024;
			      fastmem_size  -= 4*1024*1024;
			    }

			  if (m_opt && m_opt <= fastmem_size)
			    {
			      fastmem_size = m_opt;
			    }

			  if (a_opt)
			    {
			      printf("Autobooting...");
			      boothowto = RB_AUTOBOOT;
			    }

			  if (b_opt)
			    {
			      printf("Askboot...");
			      boothowto |= RB_ASKNAME;
			    }

			  if (D_opt)
			    {
			      boothowto |= RB_KDB;
			    }

			  printf ("Using %d%c FASTMEM at 0x%x, %dM CHIPMEM\n",
				(fastmem_size & 0xfffff) ? fastmem_size>>10 :
				fastmem_size>>20,
				(fastmem_size & 0xfffff) ? 'K' : 'M',
				  fastmem_start, chipmem_size>>20);
			  kern_vers = (u_short *) (kernel + e.a_entry - 2);
			  if (*kern_vers > KERNEL_PARAMETER_VERSION &&
			      *kern_vers != 0x4e73)
			    {
			      printf ("This kernel requires a newer version of loadbsd: %d\n", *kern_vers);
			      exit (0);
			    }
			  if ((cpuid & AFB_68020) == 0)
			    {
			      printf ("Hmmm... You don't seem to have a CPU capable\n");
			      printf ("        of running NetBSD.  You need a 68020\n");
			      printf ("        or better\n");
			      exit (0);
			    }
			  /* give them a chance to read the information... */
			  sleep(2);

			  bzero (kernel + text_size + e.a_data, e.a_bss);
			  /*
			   * If symbols wanted (and kernel can handle them),
			   * load symbol table & strings and set esym to end.
			   */
			  knum_cd = (int *) (kernel + text_size + e.a_data + e.a_bss);
			  if (*kern_vers != 0x4e73 && *kern_vers > 1 && S_opt && e.a_syms) {
			    *knum_cd++ = e.a_syms;
			    read(fd, (char *)knum_cd, e.a_syms);
			    knum_cd = (int *)((char *)knum_cd + e.a_syms);
			    read(fd, (char *)knum_cd, string_size);
			    knum_cd = (int*)((char *)knum_cd + string_size);
			    esym = (char *) (text_size + e.a_data + e.a_bss
			      + e.a_syms + 4 + string_size);
			  }
			  *knum_cd = num_cd;
			  for (kcd = (struct ConfigDev *) (knum_cd+1);
			       cd = FindConfigDev (cd, -1, -1);
			       *kcd++ = *cd)
				;
			  kmem_list = (struct MEM_LIST *)kcd;
			  kmem_list->num_mem = mem_list.num_mem;
			  for (mem_ix = 0; mem_ix < mem_list.num_mem; mem_ix++)
			  	kmem_list->mem_seg[mem_ix] = mem_list.mem_seg[mem_ix];
			  if (t_opt)		/* if test option */
			    exit (0);		/*   don't start kernel */
			  /* AGA startup - may need more */
			  LoadView (NULL);
			  startit (kernel, kernel_size,
				   e.a_entry, fastmem_start,
				   fastmem_size, chipmem_size,
				   boothowto, esym, cpuid );
			}
		      else
			fprintf (stderr, "Executable corrupt!\n");
		    }
		  else
		    fprintf (stderr, "Out of memory! (%d)\n", text_size + e.a_data + e.a_bss
				   + num_cd*sizeof(*cd) + 4
				   + mem_list.num_mem*sizeof(struct MEM_SEG) + 4);
		}
	      else
		fprintf (stderr, "Unsupported executable: %o\n", e.a_magic);
	    }
	  else
	    fprintf (stderr, "Can't read header of %s\n", argv[1]);

	  close (fd);
	}
      else
	perror ("open");
    }
  else
    Usage(argv[0]);
  Version();
}/* main() */

void
get_mem_config (void **fastmem_start, u_long *fastmem_size, u_long *chipmem_size)
{
  extern struct ExecBase *SysBase;
  struct MemHeader *mh, *nmh;
  int num_mem = 0;
  u_int seg_size;
  u_int seg_start;
  u_int seg_end;
  char mem_pri = -128;

  *fastmem_size = 0;
  *chipmem_size = 0;

  /* walk thru the exec memory list */
  Forbid ();
  for (mh  = (struct MemHeader *) SysBase->MemList.lh_Head;
       nmh = (struct MemHeader *) mh->mh_Node.ln_Succ;
       mh  = nmh, num_mem++)
    {
      mem_list.mem_seg[num_mem].mem_attrib = mh->mh_Attributes;
      mem_list.mem_seg[num_mem].mem_prio = mh->mh_Node.ln_Pri;
      seg_start = (u_int)mh->mh_Lower;
      seg_end = (u_int)mh->mh_Upper;
      seg_size = seg_end - seg_start;
      mem_list.mem_seg[num_mem].mem_size = seg_size;
      mem_list.mem_seg[num_mem].mem_start = seg_start;

      if (mh->mh_Attributes & MEMF_CHIP)
	{
	  /* there should hardly be more than one entry for chip mem, but
	     handle it the same nevertheless */
	  /* chipmem always starts at 0, so include vector area */
	  mem_list.mem_seg[num_mem].mem_start = seg_start = 0;
	  /* round to multiple of 512K */
	  seg_size = (seg_size + 512*1024 - 1) & -(512*1024);
	  mem_list.mem_seg[num_mem].mem_size = seg_size;
	  if (seg_size > *chipmem_size)
	    {
	      *chipmem_size = seg_size;
	    }
	}
      else
	{
	  /* some heuristics.. */
	  seg_start &= -__LDPGSZ;
	  seg_end = (seg_end + __LDPGSZ - 1) & -__LDPGSZ;
	  /* get the mem back stolen by incore kickstart on A3000 with
	     V36 bootrom. */
	  if (seg_end == 0x07f80000)
	    seg_end = 0x08000000;

	  /* or by zkick on a A2000.  */
	  if (seg_start == 0x280000
	    && strcmp (mh->mh_Node.ln_Name, "zkick memory") == 0)
	      seg_start = 0x200000;

	  seg_size = seg_end - seg_start;
	  mem_list.mem_seg[num_mem].mem_start = seg_start;
	  mem_list.mem_seg[num_mem].mem_size = seg_size;
	  /*
	   *  If this segment is smaller than 2M,
	   *  don't use it to load the kernel
	   */
	  if (seg_size < 2 * 1024 * 1024)
	    continue;
/* if p_opt is set, select memory by priority instead of size */
	  if ((!p_opt && seg_size > *fastmem_size) ||
	    (p_opt && mem_pri <= mh->mh_Node.ln_Pri && seg_size > *fastmem_size))
	    {
	      *fastmem_size = seg_size;
	      *fastmem_start = (void *)seg_start;
	      mem_pri = mh->mh_Node.ln_Pri;
	    }
	}
    }
  mem_list.num_mem = num_mem;
  Permit();
}

/*
 * Try to determine the machine ID by searching the resident module list
 * for modules only present on specific machines.  (Thanks, Bill!)
 */

void
get_cpuid ()
{
	extern struct ExecBase *SysBase;
	u_long *rl;
	struct Resident *rm;

	cpuid |= SysBase->AttnFlags;	/* get FPU and CPU flags */
	if (cpuid & 0xffff0000) {
		switch (cpuid >> 16) {
		case 500:
		case 600:
		case 1000:
		case 1200:
		case 2000:
		case 3000:
		case 4000:
			return;
		default:
			printf ("Machine type Amiga %d is not valid\n",
			    cpuid >> 16);
			exit (1);
		}
	}
	rl = (u_long *) SysBase->ResModules;
	if (rl == NULL)
		return;

	while (*rl) {
		rm = (struct Resident *) *rl;
		if (strcmp (rm->rt_Name, "A4000 Bonus") == 0 ||
		    strcmp (rm->rt_Name, "A1000 Bonus") == 0) {
			cpuid |= 4000 << 16;
			break;
		}
		if (strcmp (rm->rt_Name, "A3000 bonus") == 0 ||
		    strcmp (rm->rt_Name, "A3000 Bonus") == 0) {
			cpuid |= 3000 << 16;
			break;
		}
		if (strcmp (rm->rt_Name, "card.resource") == 0) {
			cpuid |= 1200 << 16;	/* or A600 :-) */
			break;
		}
		++rl;
	}
	if (*rl == 0)		/* Nothing found, it's probably an A2000 or A500 */
		cpuid |= 2000 << 16;
}


asm ("
	.set	ABSEXECBASE,4

	.text
	.globl	_startit

_startit:
	movel	sp,a3
	movel	4:w,a6
	lea	pc@(start_super-.+2),a5
	jmp	a6@(-0x1e)		| supervisor-call

start_super:
	movew	#0x2700,sr

	| the BSD kernel wants values into the following registers:
	| a0:  fastmem-start
	| d0:  fastmem-size
	| d1:  chipmem-size
	| d5:  AttnFlags (cpuid)
	| d7:  boothowto
	| a4:  esym location
	| All other registers zeroed for possible future requirements.

	movel	a3@(4),a1		| loaded kernel
	movel	a3@(8),d2		| length of loaded kernel
	movel	a3@(12),sp@-		| entry point [save on stack for rts]
	movel	a3@(16),a0		| fastmem-start
	movel	a3@(20),d0		| fastmem-size
	movel	a3@(24),d1		| chipmem-size
	movel	a3@(28),d7		| boothowto
	movel	a3@(32),a4		| esym
	movel	a3@(36),d5		| cpuid
	subl	a5,a5			| target, load to 0

	btst	#3,(ABSEXECBASE)@(0x129) | AFB_68040,SysBase->AttnFlags
	beq	not040

| Turn off 68040 MMU

	.word 0x4e7b,0xd003		| movec a5,tc
	.word 0x4e7b,0xd806		| movec a5,urp
	.word 0x4e7b,0xd807		| movec a5,srp
	.word 0x4e7b,0xd004		| movec a5,itt0
	.word 0x4e7b,0xd005		| movec a5,itt1
	.word 0x4e7b,0xd006		| movec a5,dtt0
	.word 0x4e7b,0xd007		| movec a5,dtt1
	bra	nott

not040:
	lea	pc@(zero-.+2),a3
	pmove	a3@,tc			| Turn off MMU
	lea	pc@(nullrp-.+2),a3
	pmove	a3@,crp			| Turn off MMU some more
	pmove	a3@,srp			| Really, really, turn off MMU

| Turn off 68030 TT registers

	btst	#2,(ABSEXECBASE)@(0x129) | AFB_68030,SysBase->AttnFlags
	beq	nott			| Skip TT registers if not 68030
	lea	pc@(zero-.+2),a3
	.word 0xf013,0x0800		| pmove a3@,tt0 (gas only knows about 68851 ops..)
	.word 0xf013,0x0c00		| pmove a3@,tt1 (gas only knows about 68851 ops..)

nott:

	movew	#(1<<9),0xdff096	| disable DMA

L0:
	moveb	a1@+,a5@+
	subl	#1,d2
	bcc	L0


	moveq	#0,d2			| zero out unused registers
	moveq	#0,d3			| (might make future compatibility
	moveq	#0,d4			|  a little easier, since all registers
	moveq	#0,d6			|  would have known contents)
	movel	d6,a1
	movel	d6,a2
	movel	d6,a3
	movel	d6,a5
	movel	d6,a6
	rts				| return to kernel entry point


| A do-nothing MMU root pointer (includes the following long as well)

nullrp:	.long	0x7fff0001
zero:	.long	0


");

void Usage(char *program_name)
{
   fprintf(stderr,usage,program_name,program_name);
}

void Version()
{
  fprintf(stderr,"%s\n",_version + 6);
}
