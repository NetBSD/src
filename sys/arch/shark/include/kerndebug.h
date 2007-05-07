/*	$NetBSD: kerndebug.h,v 1.3.26.1 2007/05/07 10:55:02 yamt Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
**++
**  FACILITY:
**
**	kerndebug.h
**	
**
**  ABSTRACT:
**
**      This  header provides generic debugging capabilities using printf.
**      All debugging can be compiled out by not defining the
**      KERNEL_DEBUG macro. In addition the amount of debug output is 
**      defined by individual variables controlled by each subsystem 
**      using this utility. Finally note that the two middle bytes of
**      the kern debug flags (bits 16 to 23) are free for individual 
**      subsystems to use as they please (eg. define switches for 
**      individual functions etc). 
**     
**  AUTHORS:
**
**      John Court
**
**  CREATION DATE:      2-Feb-1992
**
**  MODIFICATION HISTORY:
**
**--
*/
#ifndef _KERNDEBUG_H_
#define _KERNDEBUG_H_

#define KERN_DEBUG_INFO		0x00000001
#define KERN_DEBUG_WARNING      0x00000002
#define KERN_DEBUG_ERROR        0x00000010
#define KERN_DEBUG_SMP          0x00000020
#define KERN_DEBUG_PANIC	0x40000000
#define KERN_REAL_PANIC		0x80000000
#define KERN_DEBUG_ALL          KERN_DEBUG_INFO | KERN_DEBUG_WARNING | \
				KERN_DEBUG_ERROR | KERN_DEBUG_PANIC
/*
** Define the type for debugging flag subsystem variables
*/
typedef unsigned int Kern_Debug_Flags;
/*
** Set up source line location macro for extra debugging and panics
*/
#ifdef  __FILE__ 
#define KERN_DEBUG_LOC ":%s:%d:=\n\t",__FILE__,__LINE__  
#else 
#define KERN_DEBUG_LOC ":__FILE__ not supported :=\n\t" 
#endif

/*
** This is real nasty in that it requires several printf's but is 
** unavoidable due to the differences between 
** preprocessors supporting standard ANSI C and others.
** 
** NOTE: The format of calls to this macro must be 
**
**       KERN_DEBUG((Kern_Debug_Flags)CntrlVar, KERN_DEBUG_xxxx, 
**                  (normal printf arguments));
**     
**       pay special attention to the extra set of () around the 
**       final argument.
**
*/
#ifdef KERNEL_DEBUG
#define KERN_DEBUG(CntrlVar,Level,Output) \
{ \
	if ( (CntrlVar) & (Level) ) \
	{ \
		if ( (CntrlVar) & (Level) & KERN_DEBUG_PANIC ) \
		{ \
		      printf ("KERNEL:DEBUG PANIC"); \
		      printf (KERN_DEBUG_LOC); \
		      printf Output; \
		      panic("KERN_DEBUG Panicking"); \
		} \
		else \
	        { \
		      printf Output; \
		} \
	} \
}
#else  /* else KERNEL_DEBUG not defined */
#define KERN_DEBUG(CntrlVar,Level,Output) 
#endif /* end else KERNEL_DEBUG not defined */

#endif /* _KERNDEBUG_H_ */








