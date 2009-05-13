/*	$NetBSD: sysconf.c,v 1.10.92.1 2009/05/13 17:18:13 jym Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysconf.c,v 1.10.92.1 2009/05/13 17:18:13 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <machine/sysconf.h>
#include <pmax/pmax/pmaxtype.h>


#include "opt_dec_3100.h"
#ifdef DEC_3100
  void	dec_3100_init(void);
#else
# define dec_3100_init		platform_not_configured
#endif

#include "opt_dec_3max.h"
#ifdef DEC_3MAX
  void	dec_3max_init(void);
#else
# define dec_3max_init	platform_not_configured
#endif


#include "opt_dec_3min.h"
#ifdef DEC_3MIN
  void	dec_3min_init(void);
#else
# define dec_3min_init	platform_not_configured
#endif


#include "opt_dec_maxine.h"
#ifdef DEC_MAXINE
  void	dec_maxine_init(void);
#else
# define dec_maxine_init	platform_not_configured
#endif

#include "opt_dec_3maxplus.h"
#ifdef DEC_3MAXPLUS
  void	dec_3maxplus_init(void);
#else
# define dec_3maxplus_init	platform_not_configured
#endif

#include "opt_dec_5100.h"
#ifdef DEC_5100
  void	dec_5100_init(void);
#else
# define dec_5100_init	platform_not_configured
#endif

#include "opt_dec_5400.h"
#ifdef DEC_5400
  void	dec_5400_init(void);
#else
# define dec_5400_init	platform_not_configured
#endif

#include "opt_dec_5500.h"
#ifdef DEC_5500
  void	dec_5500_init(void);
#else
# define dec_5500_init	platform_not_configured
#endif


#include "opt_dec_5800.h"
#ifdef DEC_5800
  void	dec_5800_init(void);
#else
# define dec_5800_init	platform_not_configured
#endif


struct sysinit sysinit[] = {
	sys_notsupp("???"),			     /*	 0: ??? */
	sys_init(dec_3100_init,"DEC_3100"),	     /*	 1: PMAX */
	sys_init(dec_3max_init,"DEC_3MAX"),	     /*	 2: 3MAX */
	sys_init(dec_3min_init,"DEC_3MIN"),	     /*	 3: 3MIN */
	sys_init(dec_3maxplus_init,"DEC_3MAXPLUS"),  /*	 4: 3MAXPLUS */
	sys_notsupp("DEC_5800"),		     /*	 5: 5800 */
	sys_notsupp("DEC_5400"),		     /*	 6: 5400 */
	sys_init(dec_maxine_init,"DEC_MAXINE"),	     /*	 7: MAXINE */
	sys_notsupp("???"),			     /*	 8: ??? */
	sys_notsupp("???"),			     /*	 9: ??? */
	sys_notsupp("???"),			     /*	 10: ??? */
	sys_notsupp("DEC_5500"),		     /*	 11: 5500 */
	sys_init(dec_5100_init,"DEC_5100"),	     /*	 12: 5100 */
};
int nsysinit = (sizeof(sysinit) / sizeof(sysinit[0]));


void
platform_not_configured(void)
{
	printf("\n");
	printf("Support for system type %d is not present in this kernel.\n",
	    systype);
	printf("Please build a kernel with \"options %s\" and reboot.\n",
	    sysinit[systype].option);
	printf("\n");
	panic("platform not configured");
}

void
platform_not_supported(void)
{
	const char *typestr;

	if (systype >= nsysinit)
		typestr = "???";
	else
		typestr = sysinit[systype].option;

	printf("\n");
	printf("NetBSD does not yet support system type %d (%s).\n", systype,
	     typestr);
	printf("\n");
	panic("platform not supported");
}
