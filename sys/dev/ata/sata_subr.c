/*	$NetBSD: sata_subr.c,v 1.1.14.2 2008/01/21 09:42:37 yamt Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of Wasabi Systems, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Common functions for Serial ATA.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sata_subr.c,v 1.1.14.2 2008/01/21 09:42:37 yamt Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <dev/ata/satareg.h>
#include <dev/ata/satavar.h>

/*
 * sata_speed:
 *
 *	Return a string describing the port speed reported by
 *	the port's SStatus register.
 */
const char *
sata_speed(uint32_t sstatus)
{
	static const char *sata_speedtab[] = {
		"no negotiated speed",
		"1.5Gb/s",
		"3.0Gb/s",
		"<unknown 3>",
		"<unknown 4>",
		"<unknown 5>",
		"<unknown 6>",
		"<unknown 7>",
		"<unknown 8>",
		"<unknown 9>",
		"<unknown 10>",
		"<unknown 11>",
		"<unknown 12>",
		"<unknown 13>",
		"<unknown 14>",
		"<unknown 15>",
	};

	return (sata_speedtab[(sstatus & SStatus_SPD_mask) >>
			      SStatus_SPD_shift]);
}

/*
 * reset the PHY and bring it online
 */
uint32_t
sata_reset_interface(struct ata_channel *chp, bus_space_tag_t sata_t,
    bus_space_handle_t scontrol_r, bus_space_handle_t sstatus_r)
{
	uint32_t scontrol, sstatus;
	int i;

	/* bring the PHYs online.
	 * The work-around for errata #1 for the 31244 says that we must
	 * write 0 to the port first to be sure of correctly initializing
	 * the device. It doesn't hurt for other devices.
	 */
	bus_space_write_4(sata_t, scontrol_r, 0, 0);
	scontrol = SControl_IPM_NONE | SControl_SPD_ANY | SControl_DET_INIT;
	bus_space_write_4 (sata_t, scontrol_r, 0, scontrol);

	tsleep(chp, PRIBIO, "sataup", mstohz(50));
	scontrol &= ~SControl_DET_INIT;
	bus_space_write_4(sata_t, scontrol_r, 0, scontrol);

	tsleep(chp, PRIBIO, "sataup", mstohz(50));
	/* wait up to 1s for device to come up */
	for (i = 0; i < 100; i++) {
		sstatus = bus_space_read_4(sata_t, sstatus_r, 0);
		if ((sstatus & SStatus_DET_mask) == SStatus_DET_DEV)
			break;
		tsleep(chp, PRIBIO, "sataup", mstohz(10));
	}

	switch (sstatus & SStatus_DET_mask) {
	case SStatus_DET_NODEV:
		/* No Device; be silent.  */
		break;

	case SStatus_DET_DEV_NE:
		aprint_error("%s port %d: device connected, but "
		    "communication not established\n",
		    chp->ch_atac->atac_dev.dv_xname, chp->ch_channel);
		break;

	case SStatus_DET_OFFLINE:
		aprint_error("%s port %d: PHY offline\n",
		    chp->ch_atac->atac_dev.dv_xname, chp->ch_channel);
		break;

	case SStatus_DET_DEV:
		aprint_normal("%s port %d: device present, speed: %s\n",
		    chp->ch_atac->atac_dev.dv_xname, chp->ch_channel,
		    sata_speed(sstatus));
		break;
	default:
		aprint_error("%s port %d: unknown SStatus: 0x%08x\n",
		    chp->ch_atac->atac_dev.dv_xname, chp->ch_channel,
		    sstatus);
	}
	return(sstatus & SStatus_DET_mask);
}
