
/********************************************
sizes.h
copyright 1991, 1992.  Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/* $Log: sizes.h,v $
/* Revision 1.2  1993/07/02 23:57:56  jtc
/* Updated to mawk 1.1.4
/*
 * Revision 5.3  1992/12/17  02:48:01  mike
 * 1.1.2d changes for DOS
 *
 * Revision 5.2  1992/08/27  03:20:08  mike
 * patch2: increase A_HASH_PRIME
 *
 * Revision 5.1  1991/12/05  07:59:35  brennan
 * 1.1 pre-release
 *
*/

/*  sizes.h  */

#ifndef  SIZES_H
#define  SIZES_H

#if     ! HAVE_SMALL_MEMORY
#define EVAL_STACK_SIZE  256  /* initial size , can grow */
/* number of fields at startup, must be a power of 2 
   and FBANK_SZ-1 must be divisible by 3! */
#define  FBANK_SZ	256
#define  FB_SHIFT	  8   /* lg(FBANK_SZ) */
#define  NUM_FBANK	128   /* see MAX_FIELD below */

#else  /* have to be frugal with memory */

#define EVAL_STACK_SIZE   64
#define  FBANK_SZ	64
#define  FB_SHIFT	 6   /* lg(FBANK_SZ) */
#define  NUM_FBANK	16   /* see MAX_FIELD below */

#endif  

#define  MAX_SPLIT	(FBANK_SZ-1)   /* needs to be divisble by 3*/
#define  MAX_FIELD	(NUM_FBANK*FBANK_SZ - 1)

#define  MIN_SPRINTF	400


#define  BUFFSZ         4096
  /* starting buffer size for input files, grows if 
     necessary */

#if      LM_DOS
/* trade some space for IO speed */
#undef  BUFFSZ
#define BUFFSZ		8192
/* maximum input buffers that will fit in 64K */
#define  MAX_BUFFS	((int)(0x10000L/BUFFSZ) - 1)
#endif

#define  HASH_PRIME  53

#if ! HAVE_SMALL_MEMORY
#define  A_HASH_PRIME 199
#else
#define  A_HASH_PRIME  37
#endif


#define  MAX_COMPILE_ERRORS  5 /* quit if more than 4 errors */



/* AWF showed the need for this */
#define  MAIN_PAGE_SZ    4096 /* max instr in main block */
#if 0
#define  PAGE_SZ    1024  /* max instructions for other blocks */
#endif 
/* these used to be different */
#define  PAGE_SZ    4096

#endif   /* SIZES_H */
