/* $Id: ar531x_board.c,v 1.1.8.2 2006/04/22 11:37:42 simonb Exp $ */
/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * This code was written by Garrett D'Amore for the Champaign-Urbana
 * Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ar531x_board.c,v 1.1.8.2 2006/04/22 11:37:42 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <mips/atheros/include/ar531xreg.h>
#include <mips/atheros/include/ar531xvar.h>

struct ar531x_board_info *
ar531x_board_info(void)
{
	static struct ar531x_board_info	*infop = NULL;
	static struct ar531x_board_info	info;
	uint32_t			ptr, end;
	uint32_t			fctl;

	if (infop)
		return infop;

	/* configure flash bank 0 */
	fctl = REGVAL(AR531X_FLASHCTL_BASE + AR531X_FLASHCTL_0) & 
	    AR531X_FLASHCTL_MW_MASK;

	fctl |=
	    AR531X_FLASHCTL_E |
	    AR531X_FLASHCTL_AC_8M |
	    (1 << AR531X_FLASHCTL_IDCY_SHIFT) |
	    (7 << AR531X_FLASHCTL_WST1_SHIFT) |
	    (7 << AR531X_FLASHCTL_WST2_SHIFT);

	REGVAL(AR531X_FLASHCTL_BASE + AR531X_FLASHCTL_0) = fctl;

	REGVAL(AR531X_FLASHCTL_BASE + AR531X_FLASHCTL_1) &=
	    ~(AR531X_FLASHCTL_E | AR531X_FLASHCTL_AC_MASK);

	REGVAL(AR531X_FLASHCTL_BASE + AR531X_FLASHCTL_2) &=
	    ~(AR531X_FLASHCTL_E | AR531X_FLASHCTL_AC_MASK);

	/* start looking at the last 4K of flash */
	ptr = AR531X_FLASH_END - 0x1000;
	end = ptr - (500 * 1024);

	while (ptr > end) {

		if (REGVAL(ptr) == AR531X_BOARD_MAGIC) {
			memcpy(&info, (char *)MIPS_PHYS_TO_KSEG1(ptr),
			    sizeof (info));
			infop = &info;

			return (infop);
		}

		ptr -= 0x1000;
	}

	return NULL;
}
