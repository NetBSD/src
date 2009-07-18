/* $NetBSD: ar5315_board.c,v 1.1.62.1 2009/07/18 14:52:54 yamt Exp $ */

/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
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

/*
 * This file provides code to locate board-specific configuration and radio
 * information data in flash for the AR5315.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ar5315_board.c,v 1.1.62.1 2009/07/18 14:52:54 yamt Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include "opt_memsize.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>

#include <dev/cons.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <ah_soc.h>	/* XXX really doesn't belong in hal */

#include <mips/atheros/include/ar5315reg.h>
#include <mips/atheros/include/ar531xvar.h>
#include <mips/atheros/include/arbusvar.h>

#include <machine/locore.h>
#include "com.h"

/*
 * Locate the Board Configuration data using heuristics.
 * Search backward from the (aliased) end of flash looking
 * for the signature string that marks the start of the data.
 * We search at most 500KB.
 */
const struct ar531x_boarddata *
ar531x_board_info(void)
{
	static const struct ar531x_boarddata *board = NULL;
	const uint8_t *ptr, *end;

	if (board == NULL) {

		/* search backward in the flash looking for the signature */
		ptr = (const uint8_t *) MIPS_PHYS_TO_KSEG1(AR5315_CONFIG_END
		    - 0x1000);
		end = (const uint8_t *)AR5315_CONFIG_BASE;
		/* XXX validate end */
		for (; ptr > end; ptr -= 0x1000)
			if (*(const uint32_t *)ptr == AR531X_BD_MAGIC) {
				board = (const struct ar531x_boarddata *) ptr;
				break;
			}
	}
	return board;
}

/*
 * Locate the radio configuration data; it is located relative to the
 * board configuration data.
 */
const void *
ar531x_radio_info(void)
{
	static const void *radio = NULL;
	const struct ar531x_boarddata *board;
	const uint8_t *baddr, *ptr, *end;

	if (radio)
		goto done;

	board = ar531x_board_info();
	if (board == NULL)
		return NULL;
	baddr = (const uint8_t *) board;
	end = (const uint8_t *)MIPS_PHYS_TO_KSEG1(AR5315_RADIO_END);

	for (ptr = baddr + 0x1000; ptr < end; ptr += 0x1000)
		if (*(const uint32_t *)ptr != 0xffffffffU) {
			radio = ptr;
			goto done;
		}

	/* AR2316 moves radio data */
	for (ptr = baddr + 0xf8; ptr < end; ptr += 0x1000)
		if (*(const uint32_t *)ptr != 0xffffffffU) {
			radio = ptr;
			goto done;
		}

done:
	return radio;
}

/*
 * Locate board and radio configuration data in flash.
 */
int
ar531x_board_config(struct ar531x_config *config)
{

	config->board = ar531x_board_info();
	if (config->board == NULL)
		return ENOENT;

	config->radio = ar531x_radio_info();
	if (config->radio == NULL)
		return ENOENT;		/* XXX distinct code */

	return 0;
}
