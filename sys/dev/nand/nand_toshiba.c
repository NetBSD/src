/*	$NetBSD: nand_toshiba.c,v 1.1 2017/11/09 21:50:15 jmcneill Exp $	*/

/*-
 * Copyright (c) 2012-2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hoka.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Device specific functions for legacy Toshiba NAND chips
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nand_toshiba.c,v 1.1 2017/11/09 21:50:15 jmcneill Exp $");

#include "nand.h"
#include "onfi.h"

enum {
	NAND_TOSHIBA_PAGEMASK = 0x3,
	NAND_TOSHIBA_OOBMASK = 0x1 << 2,
	NAND_TOSHIBA_BLOCKMASK = 0x3 << 4,
	NAND_TOSHIBA_BITSMASK = 0x1 << 6
};

enum {
	NAND_TOSHIBA_PLANENUMMASK = 0x3 << 2
};

int
nand_read_parameters_toshiba(device_t self, struct nand_chip * const chip)
{
	uint8_t mfgrid;
	uint8_t devid;
	uint8_t params1;
	uint8_t params2;
	uint8_t params3;

	nand_select(self, true);
	nand_command(self, ONFI_READ_ID);
	nand_address(self, 0x00);
	nand_read_1(self, &mfgrid);
	nand_read_1(self, &devid);
	nand_read_1(self, &params1);
	nand_read_1(self, &params2);
	nand_read_1(self, &params3);
	nand_select(self, false);

	aprint_debug_dev(self,
	    "ID Definition table: 0x%2.x 0x%2.x 0x%2.x 0x%2.x 0x%2.x\n",
	    mfgrid, devid, params1, params2, params3);

	if (devid == 0xdc) {
		/* From the documentation */
		chip->nc_addr_cycles_column = 2;
		chip->nc_addr_cycles_row = 3;
		chip->nc_lun_blocks = 2048;

		switch (params2 & NAND_TOSHIBA_PAGEMASK) {
		case 0x0:
			chip->nc_page_size = 1024;
			break;
		case 0x1:
			chip->nc_page_size = 2048;
			break;
		case 0x2:
			chip->nc_page_size = 4096;
			break;
		case 0x3:
			chip->nc_page_size = 8192;
			break;
		default:
			KASSERTMSG(false, "ID Data parsing bug detected!");
		}

		chip->nc_spare_size =
		    (8 << __SHIFTOUT(params2, NAND_TOSHIBA_OOBMASK)) *
		    (chip->nc_page_size >> 9);

		switch ((params2 & NAND_TOSHIBA_BLOCKMASK) >> 4) {
		case 0x0:
			chip->nc_block_size = 64 * 1024;
			break;
		case 0x1:
			chip->nc_block_size = 128 * 1024;
			break;
		case 0x2:
			chip->nc_block_size = 256 * 1024;
			break;
		case 0x3:
			chip->nc_block_size = 512 * 1024;
			break;
		default:
			KASSERTMSG(false, "ID Data parsing bug detected!");
		}

		switch ((params2 & NAND_TOSHIBA_BITSMASK) >> 6) {
		case 0x0:
			/* its an 8bit chip */
			break;
		case 0x1:
			chip->nc_flags |= NC_BUSWIDTH_16;
			break;
		default:
			KASSERTMSG(false, "ID Data parsing bug detected!");
		}

		switch ((params3 & NAND_TOSHIBA_PLANENUMMASK) >> 2) {
		case 0x0:
			chip->nc_num_luns = 1;
			break;
		case 0x1:
			chip->nc_num_luns = 2;
			break;
		case 0x2:
			chip->nc_num_luns = 4;
			break;
		case 0x3:
			chip->nc_num_luns = 8;
			break;
		default:
			KASSERTMSG(false, "ID Data parsing bug detected!");
		}

		chip->nc_size = (uint64_t)chip->nc_lun_blocks * 
		    chip->nc_block_size;
	} else {
		return 1;
	}

	return 0;
}
