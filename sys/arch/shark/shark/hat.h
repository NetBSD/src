/*	$NetBSD: hat.h,v 1.1.2.2 2002/02/28 04:11:55 nathanw Exp $	*/

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
 * hat.h
 *
 * interface to high-availability timer on SHARK
 *
 * Created      : 19/05/97
 */

/* interface to high-availability timer.
   DO NOT CALL ANY OF THESE FUNCTIONS FROM CODE CALLED BY THE FIQ!
   (e.g. hatFn)
*/

/* hatClkOff(): turn off HAT clock.  

   returns: 0 => success  -1 => error
*/
int hatClkOff(void);

/* hatClkOn(): turn on HAT clock.

   count   = count for ISA TIMER 2, which generates HAT clock
   hatFn   = function to be called on every HAT FIQ.
   arg     = argument to pass to hatFn.
             if (arg == 0)
              then the FIQ handler will pass the contents of 
                   sequoia register 001H, including the PMI source bits
              otherwise the FIQ handler will not load the register and
                        will pass arg directly
   stack   = stack for hatFn
   wedgeFn = function called when the HAT wedges and needs to be restarted.
             the argument is the total number of FIQs received by the system.
             (wraps at 32-bits).

   returns: 0 => success  -1 => error
*/
int hatClkOn(int count, void (*hatFn)(int), int arg,
	     unsigned char *stack, void (*wedgeFn)(int));

/* hatClkAdjust: adjusts the speed of the HAT.

   count = new count for ISA TIMER 2

   returns: 0 => success  -1 => error
*/
int hatClkAdjust(int count);

/* hatUnwedge: call this function periodically (slower than the frequency
               of the HAT) to unwedge the HAT, if necessary.  
*/
void hatUnwedge();

/* HAT_MIN_FREQ: should be a power of 2.  Call hatUnwedge() no more
                 often than HAT_MIN_FREQ.  HMF should be a power of 2.
*/
#define HAT_MIN_FREQ 64
