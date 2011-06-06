/*	$NetBSD: sysconf.c,v 1.1.8.2 2011/06/06 09:05:17 jruoho Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sysconf.c,v 1.1.8.2 2011/06/06 09:05:17 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <machine/sysconf.h>
#include <emips/emips/emipstype.h>

#include "opt_xilinx_ml40x.h"
#ifdef XILINX_ML40X
  void	xilinx_ml40x_init (void);
#else
# define xilinx_ml40x_init	platform_not_configured
#endif

#include "opt_xs_bee3.h"
#ifdef XS_BEE3
  void	xs_bee3_init (void);
#else
# define xs_bee3_init	platform_not_configured
#endif

/* Platform-specific configuration functions
 */
struct sysinit sysinit[] = {
	sys_notsupp(NULL),			     			/*	 0: ??? */
	sys_init(xilinx_ml40x_init,"XILINX_ML40x"), /*   1: Xilinx ML50x (XUP), same as ML40x */
	sys_notsupp(NULL),			     			/*	 2: ??? */
	sys_notsupp(NULL),			     			/*	 3: ??? */
	sys_notsupp(NULL),			     			/*	 4: ??? */
	sys_notsupp(NULL),			     			/*	 5: ??? */
	sys_notsupp(NULL),			     			/*	 6: ??? */
	sys_notsupp(NULL),			     			/*	 7: ??? */
	sys_init(xilinx_ml40x_init,"XILINX_ML40x"), /*   8: Xilinx ML401/2 */
	sys_init(xs_bee3_init,"XS_BEE3"),	        /*   9: BeCube BE3 */
};
int nsysinit = (sizeof(sysinit) / sizeof(sysinit[0]));


void
platform_not_configured()
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
platform_not_supported()
{
	const char *typestr = NULL;

	if (systype < nsysinit)
		typestr = sysinit[systype].option;

	printf("\n");
	printf("NetBSD does not (yet?) support system type %d (%s).\n", systype,
	     (typestr) ? typestr : "???");
	printf("\n");
	panic("platform not supported");
}

void noop(void)
{
}
