#include <sys/types.h>
#include <a.out.h>
#include <stdio.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <libraries/configregs.h>
#include <libraries/expansionbase.h>

#include <inline/exec.h>
#include <inline/expansion.h>

struct ExpansionBase *ExpansionBase;

#undef __LDPGSZ
#define __LDPGSZ 8192

void get_mem_config (void **fastmem_start, u_long *fastmem_size, u_long *chipmem_size);

int 
main (int argc, char *argv[])
{
  struct exec e;
  int fd;

  if (argc >= 2)
    {
      if ((fd = open (argv[1], 0)) >= 0)
        {
          if (read (fd, &e, sizeof (e)) == sizeof (e))
            {
              if (e.a_magic == NMAGIC)
                {
                  u_char *kernel;
		  int text_size;
		  struct ConfigDev *cd;
		  int num_cd;
		  
		  ExpansionBase= (struct ExpansionBase *) OpenLibrary ("expansion.library", 0);
		  if (! ExpansionBase)	/* not supposed to fail... */
		    abort();
		  for (cd = 0, num_cd = 0; cd = FindConfigDev (cd, -1, -1); num_cd++) ;

		  text_size = (e.a_text + __LDPGSZ - 1) & (-__LDPGSZ);
		  kernel = (u_char *) malloc (text_size + e.a_data + e.a_bss 
				              + num_cd*sizeof(*cd) + 4);

                  if (kernel)
                    {
		      if (read (fd, kernel, e.a_text) == e.a_text 
			  && read (fd, kernel + text_size, e.a_data) == e.a_data)
			{
			  int *knum_cd;
			  struct ConfigDev *kcd;
			  void *fastmem_start;
			  u_long fastmem_size, chipmem_size;
			  
			  get_mem_config (&fastmem_start, &fastmem_size, &chipmem_size);
			  
			  if (argc == 3 && !strcmp (argv[2], "-k"))
			    {
			      fastmem_start += 4*1024*1024;
			      fastmem_size  -= 4*1024*1024;
			    }
			  
			  printf ("Using %dM FASTMEM at 0x%x, %dM CHIPMEM\n",
				  fastmem_size>>20, fastmem_start, chipmem_size>>20);
			  /* give them a chance to read the information... */
			  sleep(2);

			  bzero (kernel + text_size + e.a_data, e.a_bss);
			  knum_cd = (int *) (kernel + text_size + e.a_data + e.a_bss);
			  *knum_cd = num_cd;
			  if (num_cd)
			    for (kcd = (struct ConfigDev *) (knum_cd+1);
			         cd = FindConfigDev (cd, -1, -1);
			         *kcd++ = *cd) ;
			  startit (kernel, 
				   text_size + e.a_data + e.a_bss + num_cd*sizeof(*cd) + 4,
				   e.a_entry, fastmem_start,
				   fastmem_size, chipmem_size);
			}
		      else
			fprintf (stderr, "Executable corrupt!\n");
                    }
                  else
		    fprintf (stderr, "Out of memory! (%d)\n", text_size + e.a_data + e.a_bss 
				   + num_cd*sizeof(*cd) + 4);
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
    fprintf (stderr, "%0 some-vmunix\n", argv[0]);
}


void
get_mem_config (void **fastmem_start, u_long *fastmem_size, u_long *chipmem_size)
{
  extern struct ExecBase *SysBase;  
  struct MemHeader *mh, *nmh;
  
  *fastmem_size = 0;
  *chipmem_size = 0;
  
  /* walk thru the exec memory list */
  Forbid ();
  for (mh  = (struct MemHeader *) SysBase->MemList.lh_Head;
       nmh = (struct MemHeader *) mh->mh_Node.ln_Succ;
       mh  = nmh)
    {
      if (mh->mh_Attributes & MEMF_CHIP)
        {
	  /* there should hardly be more than one entry for chip mem, but
	     handle it the same nevertheless */
	  if ((u_int)mh->mh_Upper - (u_int)mh->mh_Lower > *chipmem_size)
	    {
	      *chipmem_size = (u_int)mh->mh_Upper - (u_int)mh->mh_Lower;
	      /* round to multiple of 512K */
	      *chipmem_size = (*chipmem_size + 512*1024 - 1) & -(512*1024);
	      
	      /* chipmem always starts at 0, so don't remember start
	         address */
	    }
        }
      else
	{
	  if ((u_int)mh->mh_Upper - (u_int)mh->mh_Lower > *fastmem_size)
	    {
	      u_int start = (u_int) mh->mh_Lower;
	      u_int end = (u_int) mh->mh_Upper;
	      
	      /* some heuristics.. */
	      start &= -__LDPGSZ;
	      /* get the mem back stolen by incore kickstart on A3000 with
	         V36 bootrom. */
	      if (end == 0x07f80000)
	        end = 0x08000000;

	      *fastmem_size = end - start;
	      *fastmem_start = (void *)start;
	    }
	}
    }
  Permit();
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

	movel	a3@(4),a1		| loaded kernel
	movel	a3@(8),d2		| length of loaded kernel
	movel	a3@(12),a2		| entry point
	movel	a3@(16),a0		| fastmem-start
	movel	a3@(20),d0		| fastmem-size
	movel	a3@(24),d1		| chipmem-size
	subl	a4,a4			| target, load to 0

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
	moveb	a1@+,a4@+
	subl	#1,d2
	bcc	L0


	jmp	a2@


| A do-nothing MMU root pointer (includes the following long as well)

nullrp:	.long	0x7fff0001
zero:	.long	0


");

