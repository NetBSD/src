/*	$NetBSD: twa.c,v 1.3 2006/05/25 01:37:08 wrstuden Exp $ */
/*	$wasabi: twa.c,v 1.25 2006/05/01 15:16:59 simonb Exp $	*/
/*
 * Copyright (c) 2004-2006 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Your Wasabi Systems License Agreement specifies the terms and
 * conditions for use and redistribution.
 */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jordan Rhody of Wasabi Systems, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*-
 * Copyright (c) 2003-04 3ware, Inc.
 * Copyright (c) 2000 Michael Smith
 * Copyright (c) 2000 BSDi
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$FreeBSD: src/sys/dev/twa/twa.c,v 1.2 2004/04/02 15:09:57 des Exp $
 */

/*
 * 3ware driver for 9000 series storage controllers.
 *
 * Author: Vinod Kashyap
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: twa.c,v 1.3 2006/05/25 01:37:08 wrstuden Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/bswap.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/endian.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/disk.h>
#include <sys/syslog.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/twareg.h>
#include <dev/pci/twavar.h>
#include <dev/pci/twaio.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/scsi_spc.h>

#include <dev/ldvar.h>

#include "locators.h"

#define	PCI_CBIO	0x10

static int	twa_fetch_aen(struct twa_softc *);
static void	twa_aen_callback(struct twa_request *);
static int	twa_find_aen(struct twa_softc *sc, u_int16_t);
static uint16_t	twa_enqueue_aen(struct twa_softc *sc,
			struct twa_command_header *);

static void	twa_attach(struct device *, struct device *, void *);
static void	twa_shutdown(void *);
static int	twa_init_connection(struct twa_softc *, u_int16_t, u_int32_t,
				    u_int16_t, u_int16_t, u_int16_t, u_int16_t, u_int16_t *,
					u_int16_t *, u_int16_t *, u_int16_t *, u_int32_t *);
static int	twa_intr(void *);
static int 	twa_match(struct device *, struct cfdata *, void *);
static int	twa_reset(struct twa_softc *);

static int	twa_print(void *, const char *);
static int	twa_soft_reset(struct twa_softc *);

static int	twa_check_ctlr_state(struct twa_softc *, u_int32_t);
static int	twa_get_param(struct twa_softc *, int, int, size_t,
				void (* callback)(struct twa_request *),
				struct twa_param_9k **);
static int 	twa_set_param(struct twa_softc *, int, int, int, void *,
				void (* callback)(struct twa_request *));
static void	twa_describe_controller(struct twa_softc *);
static int	twa_wait_status(struct twa_softc *, u_int32_t, u_int32_t);
static int	twa_done(struct twa_softc *);
#if 0
static int	twa_flash_firmware(struct twa_softc *sc);
static int	twa_hard_reset(struct twa_softc *sc);
#endif

extern struct	cfdriver twa_cd;
extern uint32_t twa_fw_img_size;
extern uint8_t	twa_fw_img[];

CFATTACH_DECL(twa, sizeof(struct twa_softc),
    twa_match, twa_attach, NULL, NULL);

/* AEN messages. */
static const struct twa_message	twa_aen_table[] = {
	{0x0000, "AEN queue empty"},
	{0x0001, "Controller reset occurred"},
	{0x0002, "Degraded unit detected"},
	{0x0003, "Controller error occured"},
	{0x0004, "Background rebuild failed"},
	{0x0005, "Background rebuild done"},
	{0x0006, "Incomplete unit detected"},
	{0x0007, "Background initialize done"},
	{0x0008, "Unclean shutdown detected"},
	{0x0009, "Drive timeout detected"},
	{0x000A, "Drive error detected"},
	{0x000B, "Rebuild started"},
	{0x000C, "Background initialize started"},
	{0x000D, "Entire logical unit was deleted"},
	{0x000E, "Background initialize failed"},
	{0x000F, "SMART attribute exceeded threshold"},
	{0x0010, "Power supply reported AC under range"},
	{0x0011, "Power supply reported DC out of range"},
	{0x0012, "Power supply reported a malfunction"},
	{0x0013, "Power supply predicted malfunction"},
	{0x0014, "Battery charge is below threshold"},
	{0x0015, "Fan speed is below threshold"},
	{0x0016, "Temperature sensor is above threshold"},
	{0x0017, "Power supply was removed"},
	{0x0018, "Power supply was inserted"},
	{0x0019, "Drive was removed from a bay"},
	{0x001A, "Drive was inserted into a bay"},
	{0x001B, "Drive bay cover door was opened"},
	{0x001C, "Drive bay cover door was closed"},
	{0x001D, "Product case was opened"},
	{0x0020, "Prepare for shutdown (power-off)"},
	{0x0021, "Downgrade UDMA mode to lower speed"},
	{0x0022, "Upgrade UDMA mode to higher speed"},
	{0x0023, "Sector repair completed"},
	{0x0024, "Sbuf memory test failed"},
	{0x0025, "Error flushing cached write data to disk"},
	{0x0026, "Drive reported data ECC error"},
	{0x0027, "DCB has checksum error"},
	{0x0028, "DCB version is unsupported"},
	{0x0029, "Background verify started"},
	{0x002A, "Background verify failed"},
	{0x002B, "Background verify done"},
	{0x002C, "Bad sector overwritten during rebuild"},
	{0x002E, "Replace failed because replacement drive too small"},
	{0x002F, "Verify failed because array was never initialized"},
	{0x0030, "Unsupported ATA drive"},
	{0x0031, "Synchronize host/controller time"},
	{0x0032, "Spare capacity is inadequate for some units"},
	{0x0033, "Background migration started"},
	{0x0034, "Background migration failed"},
	{0x0035, "Background migration done"},
	{0x0036, "Verify detected and fixed data/parity mismatch"},
	{0x0037, "SO-DIMM incompatible"},
	{0x0038, "SO-DIMM not detected"},
	{0x0039, "Corrected Sbuf ECC error"},
	{0x003A, "Drive power on reset detected"},
	{0x003B, "Background rebuild paused"},
	{0x003C, "Background initialize paused"},
	{0x003D, "Background verify paused"},
	{0x003E, "Background migration paused"},
	{0x003F, "Corrupt flash file system detected"},
	{0x0040, "Flash file system repaired"},
	{0x0041, "Unit number assignments were lost"},
	{0x0042, "Error during read of primary DCB"},
	{0x0043, "Latent error found in backup DCB"},
	{0x0044, "Battery voltage is normal"},
	{0x0045, "Battery voltage is low"},
	{0x0046, "Battery voltage is high"},
	{0x0047, "Battery voltage is too low"},
	{0x0048, "Battery voltage is too high"},
	{0x0049, "Battery temperature is normal"},
	{0x004A, "Battery temperature is low"},
	{0x004B, "Battery temperature is high"},
	{0x004C, "Battery temperature is too low"},
	{0x004D, "Battery temperature is too high"},
	{0x004E, "Battery capacity test started"},
	{0x004F, "Cache synchronization skipped"},
	{0x0050, "Battery capacity test completed"},
	{0x0051, "Battery health check started"},
	{0x0052, "Battery health check completed"},
	{0x0053, "Need to do a capacity test"},
	{0x0054, "Charge termination voltage is at high level"},
	{0x0055, "Battery charging started"},
	{0x0056, "Battery charging completed"},
	{0x0057, "Battery charging fault"},
	{0x0058, "Battery capacity is below warning level"},
	{0x0059, "Battery capacity is below error level"},
	{0x005A, "Battery is present"}, 
	{0x005B, "Battery is not present"},
	{0x005C, "Battery is weak"},
	{0x005D, "Battery health check failed"},
	{0x005E, "Cache synchronized after power fail"},
	{0x005F, "Cache synchronization failed; some data lost"},
	{0x0060, "Bad cache meta data checksum"},
	{0x0061, "Bad cache meta data signature"},
	{0x0062, "Cache meta data restore failed"},
	{0x0063, "BBU not found after power fail"},
	{0x00FC, "Recovered/finished array membership update"},
	{0x00FD, "Handler lockup"},
	{0x00FE, "Retrying PCI transfer"},
	{0x00FF, "AEN queue is full"},
	{0xFFFFFFFF, (char *)NULL}
};

/* AEN severity table. */
static const char	*twa_aen_severity_table[] = {
	"None",
	"ERROR",
	"WARNING",
	"INFO",
	"DEBUG",
	(char *)NULL
};

/* Error messages. */
static const struct twa_message	twa_error_table[] = {
	{0x0100, "SGL entry contains zero data"},
	{0x0101, "Invalid command opcode"},
	{0x0102, "SGL entry has unaligned address"},
	{0x0103, "SGL size does not match command"},
	{0x0104, "SGL entry has illegal length"},
	{0x0105, "Command packet is not aligned"},
	{0x0106, "Invalid request ID"},
	{0x0107, "Duplicate request ID"},
	{0x0108, "ID not locked"},
	{0x0109, "LBA out of range"},
	{0x010A, "Logical unit not supported"},
	{0x010B, "Parameter table does not exist"},
	{0x010C, "Parameter index does not exist"},
	{0x010D, "Invalid field in CDB"},
	{0x010E, "Specified port has invalid drive"},
	{0x010F, "Parameter item size mismatch"},
	{0x0110, "Failed memory allocation"},
	{0x0111, "Memory request too large"},
	{0x0112, "Out of memory segments"},
	{0x0113, "Invalid address to deallocate"},
	{0x0114, "Out of memory"},
	{0x0115, "Out of heap"},
	{0x0120, "Double degrade"},
	{0x0121, "Drive not degraded"},
	{0x0122, "Reconstruct error"},
	{0x0123, "Replace not accepted"},
	{0x0124, "Replace drive capacity too small"},
	{0x0125, "Sector count not allowed"},
	{0x0126, "No spares left"},
	{0x0127, "Reconstruct error"},
	{0x0128, "Unit is offline"},
	{0x0129, "Cannot update status to DCB"},
	{0x0130, "Invalid stripe handle"},
	{0x0131, "Handle that was not locked"},
	{0x0132, "Handle that was not empy"},
	{0x0133, "Handle has different owner"},
	{0x0140, "IPR has parent"},
	{0x0150, "Illegal Pbuf address alignment"},
	{0x0151, "Illegal Pbuf transfer length"},
	{0x0152, "Illegal Sbuf address alignment"},
	{0x0153, "Illegal Sbuf transfer length"},
	{0x0160, "Command packet too large"},
	{0x0161, "SGL exceeds maximum length"},
	{0x0162, "SGL has too many entries"},
	{0x0170, "Insufficient resources for rebuilder"},
	{0x0171, "Verify error (data != parity)"},
	{0x0180, "Requested segment not in directory of this DCB"},
	{0x0181, "DCB segment has unsupported version"},
	{0x0182, "DCB segment has checksum error"},
	{0x0183, "DCB support (settings) segment invalid"},
	{0x0184, "DCB UDB (unit descriptor block) segment invalid"},
	{0x0185, "DCB GUID (globally unique identifier) segment invalid"},
	{0x01A0, "Could not clear Sbuf"},
	{0x01C0, "Flash identify failed"},
	{0x01C1, "Flash out of bounds"},
	{0x01C2, "Flash verify error"},
	{0x01C3, "Flash file object not found"},
	{0x01C4, "Flash file already present"},
	{0x01C5, "Flash file system full"},
	{0x01C6, "Flash file not present"},
	{0x01C7, "Flash file size error"},
	{0x01C8, "Bad flash file checksum"},
	{0x01CA, "Corrupt flash file system detected"},
	{0x01D0, "Invalid field in parameter list"},
	{0x01D1, "Parameter list length error"},
	{0x01D2, "Parameter item is not changeable"},
	{0x01D3, "Parameter item is not saveable"},
	{0x0200, "UDMA CRC error"},
	{0x0201, "Internal CRC error"},
	{0x0202, "Data ECC error"},
	{0x0203, "ADP level 1 error"},
	{0x0204, "Port timeout"},
	{0x0205, "Drive power on reset"},
	{0x0206, "ADP level 2 error"},
	{0x0207, "Soft reset failed"},
	{0x0208, "Drive not ready"},
	{0x0209, "Unclassified port error"},
	{0x020A, "Drive aborted command"},
	{0x0210, "Internal CRC error"},
	{0x0211, "Host PCI bus abort"},
	{0x0212, "Host PCI parity error"},
	{0x0213, "Port handler error"},
	{0x0214, "Token interrupt count error"},
	{0x0215, "Timeout waiting for PCI transfer"},
	{0x0216, "Corrected buffer ECC"},
	{0x0217, "Uncorrected buffer ECC"},
	{0x0230, "Unsupported command during flash recovery"},
	{0x0231, "Next image buffer expected"},
	{0x0232, "Binary image architecture incompatible"},
	{0x0233, "Binary image has no signature"},
	{0x0234, "Binary image has bad checksum"},
	{0x0235, "Image downloaded overflowed buffer"},
	{0x0240, "I2C device not found"},
	{0x0241, "I2C transaction aborted"},
	{0x0242, "SO-DIMM parameter(s) incompatible using defaults"},
	{0x0243, "SO-DIMM unsupported"},
	{0x0248, "SPI transfer status error"},
	{0x0249, "SPI transfer timeout error"},
	{0x0250, "Invalid unit descriptor size in CreateUnit"},
	{0x0251, "Unit descriptor size exceeds data buffer in CreateUnit"},
	{0x0252, "Invalid value in CreateUnit descriptor"},
	{0x0253, "Inadequate disk space to support descriptor in CreateUnit"},
	{0x0254, "Unable to create data channel for this unit descriptor"},
	{0x0255, "CreateUnit descriptor specifies a drive already in use"},
       {0x0256, "Unable to write configuration to all disks during CreateUnit"},
	{0x0257, "CreateUnit does not support this descriptor version"},
	{0x0258, "Invalid subunit for RAID 0 or 5 in CreateUnit"},
	{0x0259, "Too many descriptors in CreateUnit"},
	{0x025A, "Invalid configuration specified in CreateUnit descriptor"},
	{0x025B, "Invalid LBA offset specified in CreateUnit descriptor"},
	{0x025C, "Invalid stripelet size specified in CreateUnit descriptor"},
	{0x0260, "SMART attribute exceeded threshold"},
	{0xFFFFFFFF, (char *)NULL}
};

struct twa_pci_identity {
	uint32_t	vendor_id;
	uint32_t	product_id;
	const char	*name;
};

static const struct twa_pci_identity pci_twa_products[] = {
	{ PCI_VENDOR_3WARE,
	  PCI_PRODUCT_3WARE_9000,
	  "3ware 9000 series",
	},
	{ PCI_VENDOR_3WARE,
	  PCI_PRODUCT_3WARE_9550,
	  "3ware 9550SX series",
	},
	{ 0,
	  0,
	  NULL,
	},
};


static inline void
twa_outl(struct twa_softc *sc, int off, u_int32_t val)
{
	bus_space_write_4(sc->twa_bus_iot, sc->twa_bus_ioh, off, val);
	bus_space_barrier(sc->twa_bus_iot, sc->twa_bus_ioh, off, 4,
	    BUS_SPACE_BARRIER_WRITE);
}


static inline u_int32_t	twa_inl(struct twa_softc *sc, int off)
{
	bus_space_barrier(sc->twa_bus_iot, sc->twa_bus_ioh, off, 4,
	    BUS_SPACE_BARRIER_WRITE | BUS_SPACE_BARRIER_READ);
	return (bus_space_read_4(sc->twa_bus_iot, sc->twa_bus_ioh, off));
}

void
twa_request_wait_handler(struct twa_request *tr)
{
	wakeup(tr);
}


static int
twa_match(struct device *parent, struct cfdata *cfdata, void *aux)
{
	int i;
	struct pci_attach_args *pa = aux;
	const struct twa_pci_identity *entry = 0;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_3WARE) {
		for (i = 0; (pci_twa_products[i].product_id); i++) {
			entry = &pci_twa_products[i];
			if (entry->product_id == PCI_PRODUCT(pa->pa_id)) {
				aprint_normal("%s: (rev. 0x%02x)\n",
				    entry->name, PCI_REVISION(pa->pa_class));
				return (1);
			}
		}
	}
	return (0);
}


static const char *
twa_find_msg_string(const struct twa_message *table, u_int16_t code)
{
	int	i;

	for (i = 0; table[i].message != NULL; i++)
		if (table[i].code == code)
			return(table[i].message);

	return(table[i].message);
}


void
twa_release_request(struct twa_request *tr)
{
	int s;
	struct twa_softc *sc;

	sc = tr->tr_sc;

	if ((tr->tr_flags & TWA_CMD_AEN) == 0) {
		s = splbio();
		TAILQ_INSERT_TAIL(&tr->tr_sc->twa_free, tr, tr_link);
		splx(s);
		if (__predict_false((tr->tr_sc->twa_sc_flags &
		    TWA_STATE_REQUEST_WAIT) != 0)) {
			tr->tr_sc->twa_sc_flags &= ~TWA_STATE_REQUEST_WAIT;
			wakeup(&sc->twa_free);
		}
	} else
		tr->tr_flags &= ~TWA_CMD_AEN_BUSY;
}


static void
twa_unmap_request(struct twa_request *tr)
{
	struct twa_softc	*sc = tr->tr_sc;
	u_int8_t		cmd_status;

	/* If the command involved data, unmap that too. */
	if (tr->tr_data != NULL) {
		if (tr->tr_cmd_pkt_type & TWA_CMD_PKT_TYPE_9K)
			cmd_status = tr->tr_command->command.cmd_pkt_9k.status;
		else
			cmd_status =
			      tr->tr_command->command.cmd_pkt_7k.generic.status;

		if (tr->tr_flags & TWA_CMD_DATA_OUT) {
			bus_dmamap_sync(tr->tr_sc->twa_dma_tag, tr->tr_dma_map,
				0, tr->tr_length, BUS_DMASYNC_POSTREAD);
			/*
			 * If we are using a bounce buffer, and we are reading
			 * data, copy the real data in.
			 */
			if (tr->tr_flags & TWA_CMD_DATA_COPY_NEEDED)
				if (cmd_status == 0)
					memcpy(tr->tr_real_data, tr->tr_data,
						tr->tr_real_length);
		}
		if (tr->tr_flags & TWA_CMD_DATA_IN)
			bus_dmamap_sync(tr->tr_sc->twa_dma_tag, tr->tr_dma_map,
				0, tr->tr_length, BUS_DMASYNC_POSTWRITE);

		bus_dmamap_unload(sc->twa_dma_tag, tr->tr_dma_map);
	}

	/* Free alignment buffer if it was used. */
	if (tr->tr_flags & TWA_CMD_DATA_COPY_NEEDED) {
		uvm_km_free(kmem_map, (vaddr_t)tr->tr_data,
		    tr->tr_length, UVM_KMF_WIRED);
		tr->tr_data = tr->tr_real_data;
		tr->tr_length = tr->tr_real_length;
	}
}


/*
 * Function name:	twa_wait_request
 * Description:		Sends down a firmware cmd, and waits for the completion,
 *			but NOT in a tight loop.
 *
 * Input:		tr	-- ptr to request pkt
 *			timeout -- max # of seconds to wait before giving up
 * Output:		None
 * Return value:	0	-- success
 *			non-zero-- failure
 */
static int
twa_wait_request(struct twa_request *tr, u_int32_t timeout)
{
	time_t	end_time;
	struct timeval	t1;
	int	s, error;

	tr->tr_flags |= TWA_CMD_SLEEP_ON_REQUEST;
	tr->tr_callback = twa_request_wait_handler;
	tr->tr_status = TWA_CMD_BUSY;

	if ((error = twa_map_request(tr)))
		return (error);

	microtime(&t1);
	end_time = t1.tv_usec +
		(timeout * 1000 * 100);

	while (tr->tr_status != TWA_CMD_COMPLETE) {
		if ((error = tr->tr_error))
			return(error);
		if ((error = tsleep(tr, PRIBIO, "twawait", timeout * hz)) == 0)
		{
			error = (tr->tr_status != TWA_CMD_COMPLETE);
			break;
		}
		if (error == EWOULDBLOCK) {
			/*
			 * We will reset the controller only if the request has
			 * already been submitted, so as to not lose the
			 * request packet.  If a busy request timed out, the
			 * reset will take care of freeing resources.  If a
			 * pending request timed out, we will free resources
			 * for that request, right here.  So, the caller is
			 * expected to NOT cleanup when ETIMEDOUT is returned.
			 */
			if (tr->tr_status != TWA_CMD_PENDING &&
			    tr->tr_status != TWA_CMD_COMPLETE)
				twa_reset(tr->tr_sc);
			else {
				/* Request was never submitted.  Clean up. */
				s = splbio();
				TAILQ_REMOVE(&tr->tr_sc->twa_pending, tr, tr_link);
				splx(s);

				twa_unmap_request(tr);
				if (tr->tr_data)
					free(tr->tr_data, M_DEVBUF);

				twa_release_request(tr);
			}
			return(ETIMEDOUT);
		}
		/*
		 * Either the request got completed, or we were woken up by a
		 * signal.  Calculate the new timeout, in case it was the latter.
		 */
		microtime(&t1);

		timeout = (end_time - t1.tv_usec) / (1000 * 100);
	}
	twa_unmap_request(tr);
	return(error);
}


/*
 * Function name:	twa_immediate_request
 * Description:		Sends down a firmware cmd, and waits for the completion
 *			in a tight loop.
 *
 * Input:		tr	-- ptr to request pkt
 *			timeout -- max # of seconds to wait before giving up
 * Output:		None
 * Return value:	0	-- success
 *			non-zero-- failure
 */
static int
twa_immediate_request(struct twa_request *tr, u_int32_t timeout)
{
	struct timeval t1;
	int	s = 0, error = 0;

	if ((error = twa_map_request(tr))) {
		return(error);
	}

	timeout = (timeout * 10000 * 10);

	microtime(&t1);

	timeout += t1.tv_usec;

	do {
		if ((error = tr->tr_error))
			return(error);
		twa_done(tr->tr_sc);
		if ((tr->tr_status != TWA_CMD_BUSY) &&
			(tr->tr_status != TWA_CMD_PENDING)) {
			twa_unmap_request(tr);
			return(tr->tr_status != TWA_CMD_COMPLETE);
		}
		microtime(&t1);
	} while (t1.tv_usec <= timeout);

	/*
	 * We will reset the controller only if the request has
	 * already been submitted, so as to not lose the
	 * request packet.  If a busy request timed out, the
	 * reset will take care of freeing resources.  If a
	 * pending request timed out, we will free resources
	 * for that request, right here.  So, the caller is
	 * expected to NOT cleanup when ETIMEDOUT is returned.
	 */
	if (tr->tr_status != TWA_CMD_PENDING)
		twa_reset(tr->tr_sc);
	else {
		/* Request was never submitted.  Clean up. */
		s = splbio();
		TAILQ_REMOVE(&tr->tr_sc->twa_pending, tr, tr_link);
		splx(s);
		twa_unmap_request(tr);
		if (tr->tr_data)
			free(tr->tr_data, M_DEVBUF);

		twa_release_request(tr);
	}
	return(ETIMEDOUT);
}


static int
twa_inquiry(struct twa_request *tr, int lunid)
{
	int error;
	struct twa_command_9k *tr_9k_cmd;

	if (tr->tr_data == NULL)
		return (ENOMEM);

	memset(tr->tr_data, 0, TWA_SECTOR_SIZE);

	tr->tr_length = TWA_SECTOR_SIZE;
	tr->tr_cmd_pkt_type = TWA_CMD_PKT_TYPE_9K;
	tr->tr_flags |= TWA_CMD_DATA_IN;

	tr_9k_cmd = &tr->tr_command->command.cmd_pkt_9k;

	tr_9k_cmd->command.opcode = TWA_OP_EXECUTE_SCSI_COMMAND;
	tr_9k_cmd->unit = lunid;
	tr_9k_cmd->request_id = tr->tr_request_id;
	tr_9k_cmd->status = 0;
	tr_9k_cmd->sgl_offset = 16;
	tr_9k_cmd->sgl_entries = 1;
	/* create the CDB here */
	tr_9k_cmd->cdb[0] = INQUIRY;
	tr_9k_cmd->cdb[1] = ((lunid << 5) & 0x0e);
	tr_9k_cmd->cdb[4] = 255;

	/* XXXX setup page data no lun device
	 * it seems 9000 series does not indicate
	 * NOTPRESENT - need more investigation
	 */
	((struct scsipi_inquiry_data *)tr->tr_data)->device =
		SID_QUAL_LU_NOTPRESENT;

	error = twa_immediate_request(tr, TWA_REQUEST_TIMEOUT_PERIOD);

	if (((struct scsipi_inquiry_data *)tr->tr_data)->device ==
		SID_QUAL_LU_NOTPRESENT)
		error = 1;

	return (error);
}

static int
twa_print_inquiry_data(struct twa_softc *sc,
	struct scsipi_inquiry_data *scsipi)
{
    printf("%s: %s\n", sc->twa_dv.dv_xname, scsipi->vendor);

    return (1);
}


static uint64_t
twa_read_capacity(struct twa_request *tr, int lunid)
{
	int error;
	struct twa_command_9k *tr_9k_cmd;
	uint64_t array_size = 0LL;

	if (tr->tr_data == NULL)
		return (ENOMEM);

	memset(tr->tr_data, 0, TWA_SECTOR_SIZE);

	tr->tr_length = TWA_SECTOR_SIZE;
	tr->tr_cmd_pkt_type = TWA_CMD_PKT_TYPE_9K;
	tr->tr_flags |= TWA_CMD_DATA_OUT;

	tr_9k_cmd = &tr->tr_command->command.cmd_pkt_9k;

	tr_9k_cmd->command.opcode = TWA_OP_EXECUTE_SCSI_COMMAND;
	tr_9k_cmd->unit = lunid;
	tr_9k_cmd->request_id = tr->tr_request_id;
	tr_9k_cmd->status = 0;
	tr_9k_cmd->sgl_offset = 16;
	tr_9k_cmd->sgl_entries = 1;
	/* create the CDB here */
	tr_9k_cmd->cdb[0] = READ_CAPACITY_16;
	tr_9k_cmd->cdb[1] = ((lunid << 5) & 0x0e) | SRC16_SERVICE_ACTION;

	error = twa_immediate_request(tr, TWA_REQUEST_TIMEOUT_PERIOD);
#if BYTE_ORDER == BIG_ENDIAN
	array_size = bswap64(_8btol(((struct scsipi_read_capacity_16_data *)
				tr->tr_data)->addr) + 1);
#else
	array_size = _8btol(((struct scsipi_read_capacity_16_data *)
				tr->tr_data)->addr) + 1;
#endif
	return (array_size);
}

static int
twa_request_sense(struct twa_request *tr, int lunid)
{
	int error = 1;
	struct twa_command_9k *tr_9k_cmd;

	if (tr->tr_data == NULL)
		return (error);

	memset(tr->tr_data, 0, TWA_SECTOR_SIZE);

	tr->tr_length = TWA_SECTOR_SIZE;
	tr->tr_cmd_pkt_type = TWA_CMD_PKT_TYPE_9K;
	tr->tr_flags |= TWA_CMD_DATA_OUT;

	tr_9k_cmd = &tr->tr_command->command.cmd_pkt_9k;

	tr_9k_cmd->command.opcode = TWA_OP_EXECUTE_SCSI_COMMAND;
	tr_9k_cmd->unit = lunid;
	tr_9k_cmd->request_id = tr->tr_request_id;
	tr_9k_cmd->status = 0;
	tr_9k_cmd->sgl_offset = 16;
	tr_9k_cmd->sgl_entries = 1;
	/* create the CDB here */
	tr_9k_cmd->cdb[0] = SCSI_REQUEST_SENSE;
	tr_9k_cmd->cdb[1] = ((lunid << 5) & 0x0e);
	tr_9k_cmd->cdb[4] = 255;

	/*XXX AEN notification called in interrupt context
	 * so just queue the request. Return as quickly
	 * as possible from interrupt
	 */
	if ((tr->tr_flags & TWA_CMD_AEN) != 0)
		error = twa_map_request(tr);
 	else
		error = twa_immediate_request(tr, TWA_REQUEST_TIMEOUT_PERIOD);

	return (error);
}



static int
twa_alloc_req_pkts(struct twa_softc *sc, int num_reqs)
{
	struct twa_request	*tr;
	struct twa_command_packet *tc;
	bus_dma_segment_t	seg;
	size_t max_segs, max_xfer;
	int	i, rv, rseg, size;

	if ((sc->twa_req_buf = malloc(num_reqs * sizeof(struct twa_request),
					M_DEVBUF, M_NOWAIT)) == NULL)
		return(ENOMEM);

	size = num_reqs * sizeof(struct twa_command_packet);

	/* Allocate memory for cmd pkts. */
	if ((rv = bus_dmamem_alloc(sc->twa_dma_tag,
		size, PAGE_SIZE, 0, &seg,
		1, &rseg, BUS_DMA_NOWAIT)) != 0){
			aprint_error("%s: unable to allocate "
				"command packets, rv = %d\n",
				sc->twa_dv.dv_xname, rv);
			return (ENOMEM);
	}

	if ((rv = bus_dmamem_map(sc->twa_dma_tag,
		&seg, rseg, size, (caddr_t *)&sc->twa_cmds,
		BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) != 0) {
			aprint_error("%s: unable to map commands, rv = %d\n",
				sc->twa_dv.dv_xname, rv);
			return (1);
	}

	if ((rv = bus_dmamap_create(sc->twa_dma_tag,
		size, num_reqs, size,
		0, BUS_DMA_NOWAIT, &sc->twa_cmd_map)) != 0) {
			aprint_error("%s: unable to create command DMA map, "
				"rv = %d\n", sc->twa_dv.dv_xname, rv);
			return (ENOMEM);
	}

	if ((rv = bus_dmamap_load(sc->twa_dma_tag, sc->twa_cmd_map,
		sc->twa_cmds, size, NULL,
		BUS_DMA_NOWAIT)) != 0) {
			aprint_error("%s: unable to load command DMA map, "
				"rv = %d\n", sc->twa_dv.dv_xname, rv);
			return (1);
	}

	if ((uintptr_t)sc->twa_cmds % TWA_ALIGNMENT) {
		aprint_error("%s: DMA map memory not aligned on %d boundary\n",
			sc->twa_dv.dv_xname, TWA_ALIGNMENT);

		return (1);
	}
	tc = sc->twa_cmd_pkt_buf = (struct twa_command_packet *)sc->twa_cmds;
	sc->twa_cmd_pkt_phys = sc->twa_cmd_map->dm_segs[0].ds_addr;

	memset(sc->twa_req_buf, 0, num_reqs * sizeof(struct twa_request));
	memset(sc->twa_cmd_pkt_buf, 0,
		num_reqs * sizeof(struct twa_command_packet));

	sc->sc_twa_request = sc->twa_req_buf;
	max_segs = twa_get_maxsegs();
	max_xfer = twa_get_maxxfer(max_segs);

	for (i = 0; i < num_reqs; i++, tc++) {
		tr = &(sc->twa_req_buf[i]);
		tr->tr_command = tc;
		tr->tr_cmd_phys = sc->twa_cmd_pkt_phys +
				(i * sizeof(struct twa_command_packet));
		tr->tr_request_id = i;
		tr->tr_sc = sc;

		/*
		 * Create a map for data buffers.  maxsize (256 * 1024) used in
		 * bus_dma_tag_create above should suffice the bounce page needs
		 * for data buffers, since the max I/O size we support is 128KB.
		 * If we supported I/O's bigger than 256KB, we would have to
		 * create a second dma_tag, with the appropriate maxsize.
		 */
		if ((rv = bus_dmamap_create(sc->twa_dma_tag,
			max_xfer, max_segs, 1, 0, BUS_DMA_NOWAIT,
			&tr->tr_dma_map)) != 0) {
				aprint_error("%s: unable to create command "
					"DMA map, rv = %d\n",
					sc->twa_dv.dv_xname, rv);
				return (ENOMEM);
		}
		/* Insert request into the free queue. */
		if (i != 0) {
			sc->twa_lookup[i] = tr;
			twa_release_request(tr);
		} else
			tr->tr_flags |= TWA_CMD_AEN;
	}
	return(0);
}


static void
twa_recompute_openings(struct twa_softc *sc)
{
	struct twa_drive *td;
	int unit;
	int openings;

	if (sc->sc_nunits != 0)
		openings = ((TWA_Q_LENGTH / 2) / sc->sc_nunits);
	else
		openings = 0;
	if (openings == sc->sc_openings)
		return;
	sc->sc_openings = openings;

#ifdef TWA_DEBUG
	printf("%s: %d array%s, %d openings per array\n",
	    sc->sc_twa.dv_xname, sc->sc_nunits,
	    sc->sc_nunits == 1 ? "" : "s", sc->sc_openings);
#endif
	for (unit = 0; unit < TWA_MAX_UNITS; unit++) {
		td = &sc->sc_units[unit];
		if (td->td_dev != NULL)
			(*td->td_callbacks->tcb_openings)(td->td_dev,
				sc->sc_openings);
	}
}


static int
twa_request_bus_scan(struct twa_softc *sc)
{
	struct twa_drive *td;
	struct twa_request *tr;
	struct twa_attach_args twaa;
	int locs[TWACF_NLOCS];
	int s, unit;

	s = splbio();
	for (unit = 0; unit < TWA_MAX_UNITS; unit++) {

		if ((tr = twa_get_request(sc, 0)) == NULL) {
			splx(s);
			return (EIO);
		}

		tr->tr_cmd_pkt_type |= TWA_CMD_PKT_TYPE_INTERNAL;

		tr->tr_data = malloc(TWA_SECTOR_SIZE, M_DEVBUF, M_NOWAIT);

		if (tr->tr_data == NULL) {
			twa_release_request(tr);
			splx(s);
			return (ENOMEM);
		}
		td = &sc->sc_units[unit];

		if (twa_inquiry(tr, unit) == 0) {

			if (td->td_dev == NULL) {
            			twa_print_inquiry_data(sc,
				   ((struct scsipi_inquiry_data *)tr->tr_data));

				sc->sc_nunits++;

				sc->sc_units[unit].td_size =
					twa_read_capacity(tr, unit);

				twaa.twaa_unit = unit;

				twa_recompute_openings(sc);

				locs[TWACF_UNIT] = unit;

				sc->sc_units[unit].td_dev =
				    	config_found_sm_loc(&sc->twa_dv, "twa", locs,
					    &twaa, twa_print, config_stdsubmatch);
			}
		} else {
			if (td->td_dev != NULL) {

				sc->sc_nunits--;

				(void) config_detach(td->td_dev, DETACH_FORCE);
				td->td_dev = NULL;
				td->td_size = 0;

				twa_recompute_openings(sc);
			}
		}
		free(tr->tr_data, M_DEVBUF);

		twa_release_request(tr);
	}
	splx(s);

	return (0);
}


static int
twa_start(struct twa_request *tr)
{
	struct twa_softc	*sc = tr->tr_sc;
	u_int32_t		status_reg;
	int			s;
	int			error;

	s = splbio();
	/* Check to see if we can post a command. */
	status_reg = twa_inl(sc, TWA_STATUS_REGISTER_OFFSET);
	if ((error = twa_check_ctlr_state(sc, status_reg)))
		goto out;

	if (status_reg & TWA_STATUS_COMMAND_QUEUE_FULL) {
			if (tr->tr_status != TWA_CMD_PENDING) {
				tr->tr_status = TWA_CMD_PENDING;
				TAILQ_INSERT_TAIL(&tr->tr_sc->twa_pending,
					tr, tr_link);
			}
			twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
					TWA_CONTROL_UNMASK_COMMAND_INTERRUPT);
			error = EBUSY;
	} else {
	   	bus_dmamap_sync(sc->twa_dma_tag, sc->twa_cmd_map,
			(caddr_t)tr->tr_command - sc->twa_cmds,
			sizeof(struct twa_command_packet),
			BUS_DMASYNC_PREWRITE | BUS_DMASYNC_PREREAD);

		/* Cmd queue is not full.  Post the command. */
		TWA_WRITE_COMMAND_QUEUE(sc, tr->tr_cmd_phys +
			sizeof(struct twa_command_header));

		/* Mark the request as currently being processed. */
		tr->tr_status = TWA_CMD_BUSY;
		/* Move the request into the busy queue. */
		TAILQ_INSERT_TAIL(&tr->tr_sc->twa_busy, tr, tr_link);
	}
out:
	splx(s);
	return(error);
}


static int
twa_drain_response_queue(struct twa_softc *sc)
{
	union twa_response_queue	rq;
	u_int32_t			status_reg;

	for (;;) {
		status_reg = twa_inl(sc, TWA_STATUS_REGISTER_OFFSET);
		if (twa_check_ctlr_state(sc, status_reg))
			return(1);
		if (status_reg & TWA_STATUS_RESPONSE_QUEUE_EMPTY)
			return(0); /* no more response queue entries */
		rq = (union twa_response_queue)twa_inl(sc, TWA_RESPONSE_QUEUE_OFFSET);
	}
}


static void
twa_drain_busy_queue(struct twa_softc *sc)
{
	struct twa_request	*tr;

	/* Walk the busy queue. */

	while ((tr = TAILQ_FIRST(&sc->twa_busy)) != NULL) {
		TAILQ_REMOVE(&sc->twa_busy, tr, tr_link);

		twa_unmap_request(tr);
		if ((tr->tr_cmd_pkt_type & TWA_CMD_PKT_TYPE_INTERNAL) ||
			(tr->tr_cmd_pkt_type & TWA_CMD_PKT_TYPE_IOCTL)) {
			/* It's an internal/ioctl request.  Simply free it. */
			if (tr->tr_data)
				free(tr->tr_data, M_DEVBUF);
			twa_release_request(tr);
		} else {
			/* It's a SCSI request.  Complete it. */
			tr->tr_command->command.cmd_pkt_9k.status = EIO;
			if (tr->tr_callback)
				tr->tr_callback(tr);
		}
	}
}


static int
twa_drain_pending_queue(struct twa_softc *sc)
{
	struct twa_request	*tr;
	int			s, error = 0;

	/*
	 * Pull requests off the pending queue, and submit them.
	 */
	s = splbio();
	while ((tr = TAILQ_FIRST(&sc->twa_pending)) != NULL) {
		TAILQ_REMOVE(&sc->twa_pending, tr, tr_link);

		if ((error = twa_start(tr))) {
			if (error == EBUSY) {
				tr->tr_status = TWA_CMD_PENDING;

				/* queue at the head */
				TAILQ_INSERT_HEAD(&tr->tr_sc->twa_pending,
					tr, tr_link);
				error = 0;
				break;
			} else {
				if (tr->tr_flags & TWA_CMD_SLEEP_ON_REQUEST) {
					tr->tr_error = error;
					tr->tr_callback(tr);
					error = EIO;
				}
			}
		}
	}
	splx(s);

	return(error);
}


static int
twa_drain_aen_queue(struct twa_softc *sc)
{
	int				error = 0;
	struct twa_request		*tr;
	struct twa_command_header	*cmd_hdr;
	struct timeval	t1;
	u_int32_t		timeout;

	for (;;) {
		if ((tr = twa_get_request(sc, 0)) == NULL) {
			error = EIO;
			break;
		}
		tr->tr_cmd_pkt_type |= TWA_CMD_PKT_TYPE_INTERNAL;
		tr->tr_callback = NULL;

		tr->tr_data = malloc(TWA_SECTOR_SIZE, M_DEVBUF, M_NOWAIT);

		if (tr->tr_data == NULL) {
			error = 1;
			goto out;
		}

		if (twa_request_sense(tr, 0) != 0) {
			error = 1;
			break;
		}

		timeout = (1000/*ms*/ * 100/*us*/ * TWA_REQUEST_TIMEOUT_PERIOD);

		microtime(&t1);

		timeout += t1.tv_usec;

		do {
			twa_done(tr->tr_sc);
			if (tr->tr_status != TWA_CMD_BUSY)
				break;
			microtime(&t1);
		} while (t1.tv_usec <= timeout);

		if (tr->tr_status != TWA_CMD_COMPLETE) {
			error = ETIMEDOUT;
			break;
		}

		if ((error = tr->tr_command->command.cmd_pkt_9k.status))
			break;

		cmd_hdr = (struct twa_command_header *)(tr->tr_data);
		if ((cmd_hdr->status_block.error) /* aen_code */
				== TWA_AEN_QUEUE_EMPTY)
			break;
		(void)twa_enqueue_aen(sc, cmd_hdr);

		free(tr->tr_data, M_DEVBUF);
		twa_release_request(tr);
	}
out:
	if (tr) {
		if (tr->tr_data)
			free(tr->tr_data, M_DEVBUF);

		twa_release_request(tr);
	}
	return(error);
}


static int
twa_done(struct twa_softc *sc)
{
	union twa_response_queue	rq;
	struct twa_request		*tr;
	int				s, error = 0;
	u_int32_t			status_reg;

	for (;;) {
		status_reg = twa_inl(sc, TWA_STATUS_REGISTER_OFFSET);
		if ((error = twa_check_ctlr_state(sc, status_reg)))
			break;
		if (status_reg & TWA_STATUS_RESPONSE_QUEUE_EMPTY)
			break;
		/* Response queue is not empty. */
		rq = (union twa_response_queue)twa_inl(sc,
			TWA_RESPONSE_QUEUE_OFFSET);
		tr = sc->sc_twa_request + rq.u.response_id;

		/* Unmap the command packet, and any associated data buffer. */
		twa_unmap_request(tr);

		s = splbio();
		tr->tr_status = TWA_CMD_COMPLETE;
		TAILQ_REMOVE(&tr->tr_sc->twa_busy, tr, tr_link);
		splx(s);

		if (tr->tr_callback)
			tr->tr_callback(tr);
	}
	(void)twa_drain_pending_queue(sc);

	return(error);
}

/*
 * Function name:	twa_init_ctlr
 * Description:		Establishes a logical connection with the controller.
 *			If bundled with firmware, determines whether or not
 *			to flash firmware, based on arch_id, fw SRL (Spec.
 *			Revision Level), branch & build #'s.  Also determines
 *			whether or not the driver is compatible with the
 *			firmware on the controller, before proceeding to work
 *			with it.
 *
 * Input:		sc	-- ptr to per ctlr structure
 * Output:		None
 * Return value:	0	-- success
 *			non-zero-- failure
 */
static int
twa_init_ctlr(struct twa_softc *sc)
{
	u_int16_t	fw_on_ctlr_srl = 0;
	u_int16_t	fw_on_ctlr_arch_id = 0;
	u_int16_t	fw_on_ctlr_branch = 0;
	u_int16_t	fw_on_ctlr_build = 0;
	u_int32_t	init_connect_result = 0;
	int		error = 0;
#if 0
	int8_t		fw_flashed = FALSE;
	int8_t		fw_flash_failed = FALSE;
#endif

	/* Wait for the controller to become ready. */
	if (twa_wait_status(sc, TWA_STATUS_MICROCONTROLLER_READY,
					TWA_REQUEST_TIMEOUT_PERIOD)) {
		return(ENXIO);
	}
	/* Drain the response queue. */
	if (twa_drain_response_queue(sc))
		return(1);

	/* Establish a logical connection with the controller. */
	if ((error = twa_init_connection(sc, TWA_INIT_MESSAGE_CREDITS,
			TWA_EXTENDED_INIT_CONNECT, TWA_CURRENT_FW_SRL,
			TWA_9000_ARCH_ID, TWA_CURRENT_FW_BRANCH,
			TWA_CURRENT_FW_BUILD, &fw_on_ctlr_srl,
			&fw_on_ctlr_arch_id, &fw_on_ctlr_branch,
			&fw_on_ctlr_build, &init_connect_result))) {
		return(error);
	}
#if 0
	if ((init_connect_result & TWA_BUNDLED_FW_SAFE_TO_FLASH) &&
		(init_connect_result & TWA_CTLR_FW_RECOMMENDS_FLASH)) {
		/*
		 * The bundled firmware is safe to flash, and the firmware
		 * on the controller recommends a flash.  So, flash!
		 */
		printf("%s: flashing bundled firmware...\n", sc->twa_dv.dv_xname);

		if ((error = twa_flash_firmware(sc))) {
			fw_flash_failed = TRUE;

			printf("%s: unable to flash bundled firmware.\n", sc->twa_dv.dv_xname);
		} else {
			printf("%s: successfully flashed bundled firmware.\n",
				 sc->twa_dv.dv_xname);
			fw_flashed = TRUE;
		}
	}
	if (fw_flashed) {
		/* The firmware was flashed.  Have the new image loaded */
		error = twa_hard_reset(sc);
		if (error == 0)
			error = twa_init_ctlr(sc);
		/*
		 * If hard reset of controller failed, we need to return.
		 * Otherwise, the above recursive call to twa_init_ctlr will
		 * have completed the rest of the initialization (starting
		 * from twa_drain_aen_queue below).  Don't do it again.
		 * Just return.
		 */
		return(error);
	} else {
		/*
		 * Either we are not bundled with a firmware image, or
		 * the bundled firmware is not safe to flash,
		 * or flash failed for some reason.  See if we can at
		 * least work with the firmware on the controller in the
		 * current mode.
		 */
		if (init_connect_result & TWA_CTLR_FW_COMPATIBLE) {
			/* Yes, we can.  Make note of the operating mode. */
			sc->working_srl = TWA_CURRENT_FW_SRL;
			sc->working_branch = TWA_CURRENT_FW_BRANCH;
			sc->working_build = TWA_CURRENT_FW_BUILD;
		} else {
			/*
			 * No, we can't.  See if we can at least work with
			 * it in the base mode.  We should never come here
			 * if firmware has just been flashed.
			 */
			printf("%s: Driver/Firmware mismatch.  Negotiating for base level.\n",
					sc->twa_dv.dv_xname);
			if ((error = twa_init_connection(sc, TWA_INIT_MESSAGE_CREDITS,
					TWA_EXTENDED_INIT_CONNECT, TWA_BASE_FW_SRL,
					TWA_9000_ARCH_ID, TWA_BASE_FW_BRANCH,
					TWA_BASE_FW_BUILD, &fw_on_ctlr_srl,
					&fw_on_ctlr_arch_id, &fw_on_ctlr_branch,
					&fw_on_ctlr_build, &init_connect_result))) {
						printf("%s: can't initialize connection in base mode.\n",
							sc->twa_dv.dv_xname);
				return(error);
			}
			if (!(init_connect_result & TWA_CTLR_FW_COMPATIBLE)) {
				/*
				 * The firmware on the controller is not even
				 * compatible with our base mode.  We cannot
				 * work with it.  Bail...
				 */
				printf("Incompatible firmware on controller\n");
#ifdef TWA_FLASH_FIRMWARE
				if (fw_flash_failed)
					printf("...and could not flash bundled firmware.\n");
				else
					printf("...and bundled firmware not safe to flash.\n");
#endif /* TWA_FLASH_FIRMWARE */
				return(1);
			}
			/* We can work with this firmware, but only in base mode. */
			sc->working_srl = TWA_BASE_FW_SRL;
			sc->working_branch = TWA_BASE_FW_BRANCH;
			sc->working_build = TWA_BASE_FW_BUILD;
			sc->twa_operating_mode = TWA_BASE_MODE;
		}
	}
#endif
	twa_drain_aen_queue(sc);

	/* Set controller state to initialized. */
	sc->twa_state &= ~TWA_STATE_SHUTDOWN;
	return(0);
}


static int
twa_setup(struct twa_softc *sc)
{
	struct tw_cl_event_packet *aen_queue;
	uint32_t		i = 0;
	int			error = 0;

	/* Initialize request queues. */
	TAILQ_INIT(&sc->twa_free);
	TAILQ_INIT(&sc->twa_busy);
	TAILQ_INIT(&sc->twa_pending);

	sc->sc_nunits = 0;
	sc->twa_sc_flags = 0;

	if (twa_alloc_req_pkts(sc, TWA_Q_LENGTH)) {

		return(ENOMEM);
	}

	/* Allocate memory for the AEN queue. */
	if ((aen_queue = malloc(sizeof(struct tw_cl_event_packet) * TWA_Q_LENGTH,
					M_DEVBUF, M_WAITOK)) == NULL) {
		/*
		 * This should not cause us to return error.  We will only be
		 * unable to support AEN's.  But then, we will have to check
		 * time and again to see if we can support AEN's, if we
		 * continue.  So, we will just return error.
		 */
		return (ENOMEM);
	}
	/* Initialize the aen queue. */
	memset(aen_queue, 0, sizeof(struct tw_cl_event_packet) * TWA_Q_LENGTH);

	for (i = 0; i < TWA_Q_LENGTH; i++)
		sc->twa_aen_queue[i] = &(aen_queue[i]);

	twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
		TWA_CONTROL_DISABLE_INTERRUPTS);

	/* Initialize the controller. */
	if ((error = twa_init_ctlr(sc))) {
		/* Soft reset the controller, and try one more time. */

		printf("%s: controller initialization failed. Retrying initialization\n",
			 sc->twa_dv.dv_xname);

		if ((error = twa_soft_reset(sc)) == 0)
			error = twa_init_ctlr(sc);
	}

	twa_describe_controller(sc);

	error = twa_request_bus_scan(sc);

	twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
		TWA_CONTROL_CLEAR_ATTENTION_INTERRUPT |
		TWA_CONTROL_UNMASK_RESPONSE_INTERRUPT |
		TWA_CONTROL_ENABLE_INTERRUPTS);

	return (error);
}

void *twa_sdh;

static void
twa_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa;
	struct twa_softc *sc;
	pci_chipset_tag_t pc;
	pcireg_t csr;
	pci_intr_handle_t ih;
	const char *intrstr;

	sc = (struct twa_softc *)self;

	pa = aux;
	pc = pa->pa_pc;
	sc->pc = pa->pa_pc;
	sc->tag = pa->pa_tag;
	sc->twa_dma_tag = pa->pa_dmat;

	aprint_naive(": RAID controller\n");
	aprint_normal(": 3ware Apache\n");
		
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_3WARE_9000) {
		if (pci_mapreg_map(pa, PCI_MAPREG_START, PCI_MAPREG_TYPE_IO, 0,
	    	    &sc->twa_bus_iot, &sc->twa_bus_ioh, NULL, NULL)) {
			aprint_error("%s: can't map i/o space\n", 
			    sc->twa_dv.dv_xname);
			return;
		}
	} else if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_3WARE_9550) {
		if (pci_mapreg_map(pa, PCI_MAPREG_START + 0x08, 
	    	    PCI_MAPREG_MEM_TYPE_64BIT, 0, &sc->twa_bus_iot, 
		    &sc->twa_bus_ioh, NULL, NULL)) {
			aprint_error("%s: can't map mem space\n", 
			    sc->twa_dv.dv_xname);
			return;
		}
	} else {
		aprint_error("%s: product id 0x%02x not recognized\n", 
		    sc->twa_dv.dv_xname, PCI_PRODUCT(pa->pa_id));
		return;
	}
	/* Enable the device. */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);

	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		aprint_error("%s: can't map interrupt\n", sc->twa_dv.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);

	sc->twa_ih = pci_intr_establish(pc, ih, IPL_BIO, twa_intr, sc);
	if (sc->twa_ih == NULL) {
		aprint_error("%s: can't establish interrupt%s%s\n",
			sc->twa_dv.dv_xname,
			(intrstr) ? " at " : "",
			(intrstr) ? intrstr : "");
		return;
	}

	if (intrstr != NULL)
		aprint_normal("%s: interrupting at %s\n",
			sc->twa_dv.dv_xname, intrstr);

	twa_setup(sc);

	if (twa_sdh == NULL)
		twa_sdh = shutdownhook_establish(twa_shutdown, NULL);

	return;
}


static void
twa_shutdown(void *arg)
{
	extern struct cfdriver twa_cd;
	struct twa_softc *sc;
	int i, rv, unit;

	for (i = 0; i < twa_cd.cd_ndevs; i++) {
		if ((sc = device_lookup(&twa_cd, i)) == NULL)
			continue;

		for (unit = 0; unit < TWA_MAX_UNITS; unit++)
			if (sc->sc_units[unit].td_dev != NULL)
				(void) config_detach(sc->sc_units[unit].td_dev,
					DETACH_FORCE | DETACH_QUIET);

		twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
			TWA_CONTROL_DISABLE_INTERRUPTS);

		/* Let the controller know that we are going down. */
		rv = twa_init_connection(sc, TWA_SHUTDOWN_MESSAGE_CREDITS,
				0, 0, 0, 0, 0,
				NULL, NULL, NULL, NULL, NULL);
	}
}


void
twa_register_callbacks(struct twa_softc *sc, int unit,
    const struct twa_callbacks *tcb)
{

	sc->sc_units[unit].td_callbacks = tcb;
}


/*
 * Print autoconfiguration message for a sub-device
 */
static int
twa_print(void *aux, const char *pnp)
{
	struct twa_attach_args *twaa;

	twaa = aux;

	if (pnp !=NULL)
		aprint_normal("block device at %s\n", pnp);
	aprint_normal(" unit %d\n", twaa->twaa_unit);
	return (UNCONF);
}


static void
twa_fillin_sgl(struct twa_sg *sgl, bus_dma_segment_t *segs, int nsegments)
{
	int	i;
	for (i = 0; i < nsegments; i++) {
		sgl[i].address = segs[i].ds_addr;
		sgl[i].length = (u_int32_t)(segs[i].ds_len);
	}
}


static int
twa_submit_io(struct twa_request *tr)
{
	int	error;

	if ((error = twa_start(tr))) {
		if (error == EBUSY)
			error = 0; /* request is in the pending queue */
		else {
			tr->tr_error = error;
		}
	}
	return(error);
}


/*
 * Function name:	twa_setup_data_dmamap
 * Description:		Callback of bus_dmamap_load for the buffer associated
 *			with data.  Updates the cmd pkt (size/sgl_entries
 *			fields, as applicable) to reflect the number of sg
 *			elements.
 *
 * Input:		arg	-- ptr to request pkt
 *			segs	-- ptr to a list of segment descriptors
 *			nsegments--# of segments
 *			error	-- 0 if no errors encountered before callback,
 *				   non-zero if errors were encountered
 * Output:		None
 * Return value:	None
 */
static int
twa_setup_data_dmamap(void *arg, bus_dma_segment_t *segs,
					int nsegments, int error)
{
	struct twa_request		*tr = (struct twa_request *)arg;
	struct twa_command_packet	*cmdpkt = tr->tr_command;
	struct twa_command_9k		*cmd9k;
	union twa_command_7k		*cmd7k;
	u_int8_t			sgl_offset;

	if (error == EFBIG) {
		tr->tr_error = error;
		goto out;
	}

	if (tr->tr_cmd_pkt_type & TWA_CMD_PKT_TYPE_9K) {
		cmd9k = &(cmdpkt->command.cmd_pkt_9k);
		twa_fillin_sgl(&(cmd9k->sg_list[0]), segs, nsegments);
		cmd9k->sgl_entries += nsegments - 1;
	} else {
		/* It's a 7000 command packet. */
		cmd7k = &(cmdpkt->command.cmd_pkt_7k);
		if ((sgl_offset = cmdpkt->command.cmd_pkt_7k.generic.sgl_offset))
			twa_fillin_sgl((struct twa_sg *)
					(((u_int32_t *)cmd7k) + sgl_offset),
					segs, nsegments);
		/* Modify the size field, based on sg address size. */
		cmd7k->generic.size +=
			((TWA_64BIT_ADDRESSES ? 3 : 2) * nsegments);
	}

	if (tr->tr_flags & TWA_CMD_DATA_IN)
		bus_dmamap_sync(tr->tr_sc->twa_dma_tag, tr->tr_dma_map, 0,
			tr->tr_length, BUS_DMASYNC_PREREAD);
	if (tr->tr_flags & TWA_CMD_DATA_OUT) {
		/*
		 * If we're using an alignment buffer, and we're
		 * writing data, copy the real data out.
		 */
		if (tr->tr_flags & TWA_CMD_DATA_COPY_NEEDED)
			memcpy(tr->tr_data, tr->tr_real_data,
				tr->tr_real_length);
		bus_dmamap_sync(tr->tr_sc->twa_dma_tag, tr->tr_dma_map, 0,
			tr->tr_length, BUS_DMASYNC_PREWRITE);
	}
	error = twa_submit_io(tr);

out:
	if (error) {
		twa_unmap_request(tr);
		/*
		 * If the caller had been returned EINPROGRESS, and he has
		 * registered a callback for handling completion, the callback
		 * will never get called because we were unable to submit the
		 * request.  So, free up the request right here.
		 */
		if ((tr->tr_flags & TWA_CMD_IN_PROGRESS) && (tr->tr_callback))
			twa_release_request(tr);
	}
	return (error);
}


/*
 * Function name:	twa_map_request
 * Description:		Maps a cmd pkt and data associated with it, into
 *			DMA'able memory.
 *
 * Input:		tr	-- ptr to request pkt
 * Output:		None
 * Return value:	0	-- success
 *			non-zero-- failure
 */
int
twa_map_request(struct twa_request *tr)
{
	struct twa_softc	*sc = tr->tr_sc;
	int			 s, rv, error = 0;

	/* If the command involves data, map that too. */
	if (tr->tr_data != NULL) {

		if (((u_long)tr->tr_data & (511)) != 0) {
			tr->tr_flags |= TWA_CMD_DATA_COPY_NEEDED;
			tr->tr_real_data = tr->tr_data;
			tr->tr_real_length = tr->tr_length;
			s = splvm();
			tr->tr_data = (void *)uvm_km_alloc(kmem_map,
			    tr->tr_length, 512, UVM_KMF_NOWAIT|UVM_KMF_WIRED);
			splx(s);

			if (tr->tr_data == NULL) {
				tr->tr_data = tr->tr_real_data;
				tr->tr_length = tr->tr_real_length;
				return(ENOMEM);
			}
			if ((tr->tr_flags & TWA_CMD_DATA_IN) != 0)
				memcpy(tr->tr_data, tr->tr_real_data,
					tr->tr_length);
		}

		/*
		 * Map the data buffer into bus space and build the S/G list.
		 */
		rv = bus_dmamap_load(sc->twa_dma_tag, tr->tr_dma_map,
			tr->tr_data, tr->tr_length, NULL, BUS_DMA_NOWAIT |
			BUS_DMA_STREAMING | (tr->tr_flags & TWA_CMD_DATA_OUT) ?
			BUS_DMA_READ : BUS_DMA_WRITE);

		if (rv != 0) {
			if ((tr->tr_flags & TWA_CMD_DATA_COPY_NEEDED) != 0) {
				s = splvm();
				uvm_km_free(kmem_map, (vaddr_t)tr->tr_data,
				    tr->tr_length, UVM_KMF_WIRED);
				splx(s);
			}
			return (rv);
		}

		if ((rv = twa_setup_data_dmamap(tr,
				tr->tr_dma_map->dm_segs,
				tr->tr_dma_map->dm_nsegs, error))) {

			if (tr->tr_flags & TWA_CMD_DATA_COPY_NEEDED) {
				uvm_km_free(kmem_map, (vaddr_t)tr->tr_data,
				    tr->tr_length, UVM_KMF_WIRED);
				tr->tr_data = tr->tr_real_data;
				tr->tr_length = tr->tr_real_length;
			}
		} else
			error = tr->tr_error;

	} else
		if ((rv = twa_submit_io(tr)))
			twa_unmap_request(tr);

	return (rv);
}

#if 0
/*
 * Function name:	twa_flash_firmware
 * Description:		Flashes bundled firmware image onto controller.
 *
 * Input:		sc	-- ptr to per ctlr structure
 * Output:		None
 * Return value:	0	-- success
 *			non-zero-- failure
 */
static int
twa_flash_firmware(struct twa_softc *sc)
{
	struct twa_request			*tr;
	struct twa_command_download_firmware	*cmd;
	uint32_t				count;
	uint32_t				fw_img_chunk_size;
	uint32_t				this_chunk_size = 0;
	uint32_t				remaining_img_size = 0;
	int					s, error = 0;
	int					i;

	if ((tr = twa_get_request(sc, 0)) == NULL) {
		/* No free request packets available.  Can't proceed. */
		error = EIO;
		goto out;
	}

	count = (twa_fw_img_size / 65536);

	count += ((twa_fw_img_size % 65536) != 0) ? 1 : 0;

	tr->tr_cmd_pkt_type |= TWA_CMD_PKT_TYPE_INTERNAL;
	/* Allocate sufficient memory to hold a chunk of the firmware image. */
	fw_img_chunk_size = ((twa_fw_img_size / count) + 511) & ~511;

	s = splvm();
	tr->tr_data = (void *)uvm_km_alloc(kmem_map, fw_img_chunk_size, 512,
				UVM_KMF_WIRED);
	splx(s);

	if (tr->tr_data == NULL) {
		error = ENOMEM;
		goto out;
	}

	remaining_img_size = twa_fw_img_size;
	cmd = &(tr->tr_command->command.cmd_pkt_7k.download_fw);

	for (i = 0; i < count; i++) {
		/* Build a cmd pkt for downloading firmware. */
		memset(tr->tr_command, 0, sizeof(struct twa_command_packet));

		tr->tr_command->cmd_hdr.header_desc.size_header = 128;

		cmd->opcode = TWA_OP_DOWNLOAD_FIRMWARE;
		cmd->sgl_offset = 2;/* offset in dwords, to the beginning of sg list */
		cmd->size = 2;	/* this field will be updated at data map time */
		cmd->request_id = tr->tr_request_id;
		cmd->unit = 0;
		cmd->status = 0;
		cmd->flags = 0;
		cmd->param = 8;	/* prom image */

		if (i != (count - 1))
			this_chunk_size = fw_img_chunk_size;
		else	 /* last chunk */
			this_chunk_size = remaining_img_size;

		remaining_img_size -= this_chunk_size;

		memset(tr->tr_data, fw_img_chunk_size, 0);

		memcpy(tr->tr_data, twa_fw_img + (i * fw_img_chunk_size),
			this_chunk_size);
		/*
		 * The next line will effect only the last chunk.
		 */
		tr->tr_length = (this_chunk_size + 511) & ~511;

		tr->tr_flags |= TWA_CMD_DATA_OUT;

		error = twa_immediate_request(tr, TWA_REQUEST_TIMEOUT_PERIOD);

		if (error) {
			if (error == ETIMEDOUT)
				return(error); /* clean-up done by twa_immediate_request */
			break;
		}
		error = cmd->status;

		if (i != (count - 1)) {

			/* XXX FreeBSD code doesn't check for no error condition
			 * but based on observation, error seems to return 0
			 */
			if ((error = tr->tr_command->cmd_hdr.status_block.error) == 0) {
				continue;
			} else if ((error = tr->tr_command->cmd_hdr.status_block.error) ==
				TWA_ERROR_MORE_DATA) {
				    continue;
			} else {
				twa_hard_reset(sc);
				break;
			}
		} else	 /* last chunk */
			if (error) {
				printf("%s: firmware flash request failed. error = 0x%x\n",
					 sc->twa_dv.dv_xname, error);
				twa_hard_reset(sc);
			}
	} /* for */

	if (tr->tr_data) {
		s = splvm();
		uvm_km_free(kmem_map, (vaddr_t)tr->tr_data,
			fw_img_chunk_size, UVM_KMF_WIRED);
		splx(s);
	}
out:
	if (tr)
		twa_release_request(tr);
	return(error);
}

/*
 * Function name:	twa_hard_reset
 * Description:		Hard reset the controller.
 *
 * Input:		sc	-- ptr to per ctlr structure
 * Output:		None
 * Return value:	0	-- success
 *			non-zero-- failure
 */
static int
twa_hard_reset(struct twa_softc *sc)
{
	struct twa_request			*tr;
	struct twa_command_reset_firmware	*cmd;
	int					error;

	if ((tr = twa_get_request(sc, 0)) == NULL)
		return(EIO);
	tr->tr_cmd_pkt_type |= TWA_CMD_PKT_TYPE_INTERNAL;
	/* Build a cmd pkt for sending down the hard reset command. */
	tr->tr_command->cmd_hdr.header_desc.size_header = 128;

	cmd = &(tr->tr_command->command.cmd_pkt_7k.reset_fw);
	cmd->opcode = TWA_OP_RESET_FIRMWARE;
	cmd->size = 2;	/* this field will be updated at data map time */
	cmd->request_id = tr->tr_request_id;
	cmd->unit = 0;
	cmd->status = 0;
	cmd->flags = 0;
	cmd->param = 0;	/* don't reload FPGA logic */

	tr->tr_data = NULL;
	tr->tr_length = 0;

	error = twa_immediate_request(tr, TWA_REQUEST_TIMEOUT_PERIOD);
	if (error) {
		printf("%s: hard reset request could not "
			" be posted. error = 0x%x\n", sc->twa_dv.dv_xname, error);
		if (error == ETIMEDOUT)
			return(error); /* clean-up done by twa_immediate_request */
		goto out;
	}
	if ((error = cmd->status)) {
		printf("%s: hard reset request failed. error = 0x%x\n",
			sc->twa_dv.dv_xname, error);
	}

out:
	if (tr)
		twa_release_request(tr);
	return(error);
}
#endif

/*
 * Function name:	twa_intr
 * Description:		Interrupt handler.  Determines the kind of interrupt,
 *			and calls the appropriate handler.
 *
 * Input:		sc	-- ptr to per ctlr structure
 * Output:		None
 * Return value:	None
 */

static int
twa_intr(void *arg)
{
	int	caught, rv;
	struct twa_softc *sc;
	u_int32_t	status_reg;
	sc = (struct twa_softc *)arg;

	caught = 0;
	/* Collect current interrupt status. */
	status_reg = twa_inl(sc, TWA_STATUS_REGISTER_OFFSET);
	if (twa_check_ctlr_state(sc, status_reg)) {
		caught = 1;
		goto bail;
	}
	/* Dispatch based on the kind of interrupt. */
	if (status_reg & TWA_STATUS_HOST_INTERRUPT) {
		twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
			TWA_CONTROL_CLEAR_HOST_INTERRUPT);
		caught = 1;
	}
	if ((status_reg & TWA_STATUS_ATTENTION_INTERRUPT) != 0) {
		twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
			TWA_CONTROL_CLEAR_ATTENTION_INTERRUPT);
		rv = twa_fetch_aen(sc);
#ifdef DIAGNOSTIC
		if (rv != 0)
			printf("%s: unable to retrieve AEN (%d)\n",
				sc->twa_dv.dv_xname, rv);
#endif
		caught = 1;
	}
	if (status_reg & TWA_STATUS_COMMAND_INTERRUPT) {
		/* Start any requests that might be in the pending queue. */
		twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
			TWA_CONTROL_MASK_COMMAND_INTERRUPT);
		(void)twa_drain_pending_queue(sc);
		caught = 1;
	}
	if (status_reg & TWA_STATUS_RESPONSE_INTERRUPT) {
		twa_done(sc);
		caught = 1;
	}
bail:
	return (caught);
}


/*
 * Accept an open operation on the control device.
 */
static int
twaopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct twa_softc *twa;

	if ((twa = device_lookup(&twa_cd, minor(dev))) == NULL)
		return (ENXIO);
	if ((twa->twa_sc_flags & TWA_STATE_OPEN) != 0)
		return (EBUSY);

	twa->twa_sc_flags |= TWA_STATE_OPEN;

	return (0);
}


/*
 * Accept the last close on the control device.
 */
static int
twaclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct twa_softc *twa;

	twa = device_lookup(&twa_cd, minor(dev));
	twa->twa_sc_flags &= ~TWA_STATE_OPEN;
	return (0);
}


/*
 * Function name:	twaioctl
 * Description:		ioctl handler.
 *
 * Input:		sc	-- ptr to per ctlr structure
 *			cmd	-- ioctl cmd
 *			buf	-- ptr to buffer in kernel memory, which is
 *				   a copy of the input buffer in user-space
 * Output:		buf	-- ptr to buffer in kernel memory, which will
 *				   be copied of the output buffer in user-space
 * Return value:	0	-- success
 *			non-zero-- failure
 */
static int
twaioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct twa_softc *sc;
	struct twa_ioctl_9k	*user_buf = (struct twa_ioctl_9k *)data;
	struct tw_cl_event_packet event_buf;
	struct twa_request 	*tr = 0;
	int32_t			event_index = 0;
	int32_t			start_index;
	int			s, error = 0;

	sc = device_lookup(&twa_cd, minor(dev));

	switch (cmd) {
	case TW_OSL_IOCTL_FIRMWARE_PASS_THROUGH:
	{
		struct twa_command_packet	*cmdpkt;
		u_int32_t			data_buf_size_adjusted;

		/* Get a request packet */
		tr = twa_get_request_wait(sc, 0);
		KASSERT(tr != NULL);
		/*
		 * Make sure that the data buffer sent to firmware is a
		 * 512 byte multiple in size.
		 */
		data_buf_size_adjusted =
			(user_buf->twa_drvr_pkt.buffer_length + 511) & ~511;

		if ((tr->tr_length = data_buf_size_adjusted)) {
			if ((tr->tr_data = malloc(data_buf_size_adjusted,
			    M_DEVBUF, M_WAITOK)) == NULL) {
				error = ENOMEM;
				goto fw_passthru_done;
			}
			/* Copy the payload. */
			if ((error = copyin((void *) (user_buf->pdata),
				(void *) (tr->tr_data),
				user_buf->twa_drvr_pkt.buffer_length)) != 0) {
					goto fw_passthru_done;
			}
			tr->tr_flags |= TWA_CMD_DATA_IN | TWA_CMD_DATA_OUT;
		}
		tr->tr_cmd_pkt_type |= TWA_CMD_PKT_TYPE_IOCTL;
		cmdpkt = tr->tr_command;

		/* Copy the command packet. */
		memcpy(cmdpkt, &(user_buf->twa_cmd_pkt),
			sizeof(struct twa_command_packet));
		cmdpkt->command.cmd_pkt_7k.generic.request_id =
			tr->tr_request_id;

		/* Send down the request, and wait for it to complete. */
		if ((error = twa_wait_request(tr, TWA_REQUEST_TIMEOUT_PERIOD))) 		{
			if (error == ETIMEDOUT)
				break; /* clean-up done by twa_wait_request */
			goto fw_passthru_done;
		}

		/* Copy the command packet back into user space. */
		memcpy(&user_buf->twa_cmd_pkt, cmdpkt,
			sizeof(struct twa_command_packet));

		/* If there was a payload, copy it back too. */
		if (tr->tr_length)
			error = copyout(tr->tr_data, user_buf->pdata,
					user_buf->twa_drvr_pkt.buffer_length);
fw_passthru_done:
		/* Free resources. */
		if (tr->tr_data)
			free(tr->tr_data, M_DEVBUF);

		if (tr)
			twa_release_request(tr);
		break;
	}

	case TW_OSL_IOCTL_SCAN_BUS:
		twa_request_bus_scan(sc);
		break;

	case TW_CL_IOCTL_GET_FIRST_EVENT:
		if (sc->twa_aen_queue_wrapped) {
			if (sc->twa_aen_queue_overflow) {
				/*
				 * The aen queue has wrapped, even before some
				 * events have been retrieved.  Let the caller
				 * know that he missed out on some AEN's.
				 */
				user_buf->twa_drvr_pkt.status =
					TWA_ERROR_AEN_OVERFLOW;
				sc->twa_aen_queue_overflow = FALSE;
			} else
				user_buf->twa_drvr_pkt.status = 0;
			event_index = sc->twa_aen_head;
		} else {
			if (sc->twa_aen_head == sc->twa_aen_tail) {
				user_buf->twa_drvr_pkt.status =
					TWA_ERROR_AEN_NO_EVENTS;
				break;
			}
			user_buf->twa_drvr_pkt.status = 0;
			event_index = sc->twa_aen_tail;	/* = 0 */
		}
		if ((error = copyout(sc->twa_aen_queue[event_index],
			user_buf->pdata, sizeof(struct tw_cl_event_packet))) != 0)
				(sc->twa_aen_queue[event_index])->retrieved =
					TWA_AEN_RETRIEVED;
		break;


	case TW_CL_IOCTL_GET_LAST_EVENT:

		if (sc->twa_aen_queue_wrapped) {
			if (sc->twa_aen_queue_overflow) {
				/*
				 * The aen queue has wrapped, even before some
				 * events have been retrieved.  Let the caller
				 * know that he missed out on some AEN's.
				 */
				user_buf->twa_drvr_pkt.status =
					TWA_ERROR_AEN_OVERFLOW;
				sc->twa_aen_queue_overflow = FALSE;
			} else
				user_buf->twa_drvr_pkt.status = 0;
		} else {
			if (sc->twa_aen_head == sc->twa_aen_tail) {
				user_buf->twa_drvr_pkt.status =
					TWA_ERROR_AEN_NO_EVENTS;
				break;
			}
			user_buf->twa_drvr_pkt.status = 0;
		}
		event_index = (sc->twa_aen_head - 1 + TWA_Q_LENGTH) % TWA_Q_LENGTH;
		if ((error = copyout(sc->twa_aen_queue[event_index], user_buf->pdata,
					sizeof(struct tw_cl_event_packet))) != 0)

		(sc->twa_aen_queue[event_index])->retrieved =
			TWA_AEN_RETRIEVED;
		break;


	case TW_CL_IOCTL_GET_NEXT_EVENT:

		user_buf->twa_drvr_pkt.status = 0;
		if (sc->twa_aen_queue_wrapped) {

			if (sc->twa_aen_queue_overflow) {
				/*
				 * The aen queue has wrapped, even before some
				 * events have been retrieved.  Let the caller
				 * know that he missed out on some AEN's.
				 */
				user_buf->twa_drvr_pkt.status =
					TWA_ERROR_AEN_OVERFLOW;
				sc->twa_aen_queue_overflow = FALSE;
			}
			start_index = sc->twa_aen_head;
		} else {
			if (sc->twa_aen_head == sc->twa_aen_tail) {
				user_buf->twa_drvr_pkt.status =
					TWA_ERROR_AEN_NO_EVENTS;
				break;
			}
			start_index = sc->twa_aen_tail;	/* = 0 */
		}
		error = copyin(user_buf->pdata, &event_buf,
				sizeof(struct tw_cl_event_packet));

		event_index = (start_index + event_buf.sequence_id -
				(sc->twa_aen_queue[start_index])->sequence_id + 1)
				% TWA_Q_LENGTH;

		if (! ((sc->twa_aen_queue[event_index])->sequence_id >
						event_buf.sequence_id)) {
			if (user_buf->twa_drvr_pkt.status == TWA_ERROR_AEN_OVERFLOW)
				sc->twa_aen_queue_overflow = TRUE; /* so we report the overflow next time */
			user_buf->twa_drvr_pkt.status =
				TWA_ERROR_AEN_NO_EVENTS;
			break;
		}
		if ((error = copyout(sc->twa_aen_queue[event_index], user_buf->pdata,
					sizeof(struct tw_cl_event_packet))) != 0)

		(sc->twa_aen_queue[event_index])->retrieved =
			TWA_AEN_RETRIEVED;
		break;


	case TW_CL_IOCTL_GET_PREVIOUS_EVENT:

		user_buf->twa_drvr_pkt.status = 0;
		if (sc->twa_aen_queue_wrapped) {
			if (sc->twa_aen_queue_overflow) {
				/*
				 * The aen queue has wrapped, even before some
				 * events have been retrieved.  Let the caller
				 * know that he missed out on some AEN's.
				 */
				user_buf->twa_drvr_pkt.status =
					TWA_ERROR_AEN_OVERFLOW;
				sc->twa_aen_queue_overflow = FALSE;
			}
			start_index = sc->twa_aen_head;
		} else {
			if (sc->twa_aen_head == sc->twa_aen_tail) {
				user_buf->twa_drvr_pkt.status =
					TWA_ERROR_AEN_NO_EVENTS;
				break;
			}
			start_index = sc->twa_aen_tail;	/* = 0 */
		}
		if ((error = copyin(user_buf->pdata, &event_buf,
				sizeof(struct tw_cl_event_packet))) != 0)

		event_index = (start_index + event_buf.sequence_id -
			(sc->twa_aen_queue[start_index])->sequence_id - 1) % TWA_Q_LENGTH;
		if (! ((sc->twa_aen_queue[event_index])->sequence_id <
			event_buf.sequence_id)) {
			if (user_buf->twa_drvr_pkt.status == TWA_ERROR_AEN_OVERFLOW)
				sc->twa_aen_queue_overflow = TRUE; /* so we report the overflow next time */
			user_buf->twa_drvr_pkt.status =
				TWA_ERROR_AEN_NO_EVENTS;
			break;
		}
		if ((error = copyout(sc->twa_aen_queue [event_index], user_buf->pdata,
 				sizeof(struct tw_cl_event_packet))) != 0)
				aprint_error("%s: get_previous: Could not copyout to "
					"event_buf. error = %x\n", sc->twa_dv.dv_xname, error);
		(sc->twa_aen_queue[event_index])->retrieved = TWA_AEN_RETRIEVED;
		break;

	case TW_CL_IOCTL_GET_LOCK:
	{
		struct tw_cl_lock_packet	twa_lock;

		copyin(user_buf->pdata, &twa_lock,
				sizeof(struct tw_cl_lock_packet));
		s = splbio();
		if ((sc->twa_ioctl_lock.lock == TWA_LOCK_FREE) ||
			(twa_lock.force_flag) ||
			(time.tv_sec >= sc->twa_ioctl_lock.timeout)) {

			sc->twa_ioctl_lock.lock = TWA_LOCK_HELD;
			sc->twa_ioctl_lock.timeout = time.tv_sec +
				(twa_lock.timeout_msec / 1000);
			twa_lock.time_remaining_msec = twa_lock.timeout_msec;
			user_buf->twa_drvr_pkt.status = 0;
		} else {
			twa_lock.time_remaining_msec =
				(sc->twa_ioctl_lock.timeout - time.tv_sec) *
				1000;
			user_buf->twa_drvr_pkt.status =
					TWA_ERROR_IOCTL_LOCK_ALREADY_HELD;
		}
		splx(s);
		copyout(&twa_lock, user_buf->pdata,
				sizeof(struct tw_cl_lock_packet));
		break;
	}

	case TW_CL_IOCTL_RELEASE_LOCK:
		s = splbio();
		if (sc->twa_ioctl_lock.lock == TWA_LOCK_FREE) {
			user_buf->twa_drvr_pkt.status =
				TWA_ERROR_IOCTL_LOCK_NOT_HELD;
		} else {
			sc->twa_ioctl_lock.lock = TWA_LOCK_FREE;
			user_buf->twa_drvr_pkt.status = 0;
		}
		splx(s);
		break;

	case TW_CL_IOCTL_GET_COMPATIBILITY_INFO:
	{
		struct tw_cl_compatibility_packet	comp_pkt;

		memcpy(comp_pkt.driver_version, TWA_DRIVER_VERSION_STRING,
					sizeof(TWA_DRIVER_VERSION_STRING));
		comp_pkt.working_srl = sc->working_srl;
		comp_pkt.working_branch = sc->working_branch;
		comp_pkt.working_build = sc->working_build;
		user_buf->twa_drvr_pkt.status = 0;

		/* Copy compatibility information to user space. */
		copyout(&comp_pkt, user_buf->pdata,
				min(sizeof(struct tw_cl_compatibility_packet),
					user_buf->twa_drvr_pkt.buffer_length));
		break;
	}

	case TWA_IOCTL_GET_UNITNAME:	/* WASABI EXTENSION */
	{
		struct twa_unitname	*tn;
		struct twa_drive	*tdr;

		tn = (struct twa_unitname *)data;
			/* XXX mutex */
		if (tn->tn_unit < 0 || tn->tn_unit >= TWA_MAX_UNITS)
			return (EINVAL);
		tdr = &sc->sc_units[tn->tn_unit];
		if (tdr->td_dev == NULL)
			tn->tn_name[0] = '\0';
		else
			strlcpy(tn->tn_name, tdr->td_dev->dv_xname,
			    sizeof(tn->tn_name));
		return (0);
	}

	default:
		/* Unknown opcode. */
		error = ENOTTY;
	}

	return(error);
}


const struct cdevsw twa_cdevsw = {
	twaopen, twaclose, noread, nowrite, twaioctl,
	nostop, notty, nopoll, nommap,
};


/*
 * Function name:	twa_get_param
 * Description:		Get a firmware parameter.
 *
 * Input:		sc		-- ptr to per ctlr structure
 *			table_id	-- parameter table #
 *			param_id	-- index of the parameter in the table
 *			param_size	-- size of the parameter in bytes
 *			callback	-- ptr to function, if any, to be called
 *					back on completion; NULL if no callback.
 * Output:		None
 * Return value:	ptr to param structure	-- success
 *			NULL			-- failure
 */
static int
twa_get_param(struct twa_softc *sc, int table_id, int param_id,
		size_t param_size, void (* callback)(struct twa_request *tr),
		struct twa_param_9k **param)
{
	int			rv = 0;
	struct twa_request	*tr;
	union twa_command_7k	*cmd;

	/* Get a request packet. */
	if ((tr = twa_get_request(sc, 0)) == NULL) {
		rv = EAGAIN;
		goto out;
	}

	tr->tr_cmd_pkt_type |= TWA_CMD_PKT_TYPE_INTERNAL;

	/* Allocate memory to read data into. */
	if ((*param = (struct twa_param_9k *)
		malloc(TWA_SECTOR_SIZE, M_DEVBUF, M_NOWAIT)) == NULL) {
		rv = ENOMEM;
		goto out;
	}

	memset(*param, 0, sizeof(struct twa_param_9k) - 1 + param_size);
	tr->tr_data = *param;
	tr->tr_length = TWA_SECTOR_SIZE;
	tr->tr_flags = TWA_CMD_DATA_IN | TWA_CMD_DATA_OUT;

	/* Build the cmd pkt. */
	cmd = &(tr->tr_command->command.cmd_pkt_7k);

	tr->tr_command->cmd_hdr.header_desc.size_header = 128;

	cmd->param.opcode = TWA_OP_GET_PARAM;
	cmd->param.sgl_offset = 2;
	cmd->param.size = 2;
	cmd->param.request_id = tr->tr_request_id;
	cmd->param.unit = 0;
	cmd->param.param_count = 1;

	/* Specify which parameter we need. */
	(*param)->table_id = table_id | TWA_9K_PARAM_DESCRIPTOR;
	(*param)->parameter_id = param_id;
	(*param)->parameter_size_bytes = param_size;

	/* Submit the command. */
	if (callback == NULL) {
		/* There's no call back; wait till the command completes. */
		rv = twa_immediate_request(tr, TWA_REQUEST_TIMEOUT_PERIOD);

		if (rv != 0)
			goto out;

		if ((rv = cmd->param.status) != 0) {
		     /* twa_drain_complete_queue will have done the unmapping */
		     goto out;
		}
		twa_release_request(tr);
		return (rv);
	} else {
		/* There's a call back.  Simply submit the command. */
		tr->tr_callback = callback;
		rv = twa_map_request(tr);
		return (rv);
	}
out:
	if (tr)
		twa_release_request(tr);
	return(rv);
}


/*
 * Function name:	twa_set_param
 * Description:		Set a firmware parameter.
 *
 * Input:		sc		-- ptr to per ctlr structure
 *			table_id	-- parameter table #
 *			param_id	-- index of the parameter in the table
 *			param_size	-- size of the parameter in bytes
 *			callback	-- ptr to function, if any, to be called
 *					back on completion; NULL if no callback.
 * Output:		None
 * Return value:	0	-- success
 *			non-zero-- failure
 */
static int
twa_set_param(struct twa_softc *sc, int table_id,
			int param_id, int param_size, void *data,
			void (* callback)(struct twa_request *tr))
{
	struct twa_request	*tr;
	union twa_command_7k	*cmd;
	struct twa_param_9k	*param = NULL;
	int			error = ENOMEM;

	tr = twa_get_request(sc, 0);
	if (tr == NULL)
		return (EAGAIN);

	tr->tr_cmd_pkt_type |= TWA_CMD_PKT_TYPE_INTERNAL;

	/* Allocate memory to send data using. */
	if ((param = (struct twa_param_9k *)
			malloc(TWA_SECTOR_SIZE, M_DEVBUF, M_NOWAIT)) == NULL)
		goto out;
	memset(param, 0, sizeof(struct twa_param_9k) - 1 + param_size);
	tr->tr_data = param;
	tr->tr_length = TWA_SECTOR_SIZE;
	tr->tr_flags = TWA_CMD_DATA_IN | TWA_CMD_DATA_OUT;

	/* Build the cmd pkt. */
	cmd = &(tr->tr_command->command.cmd_pkt_7k);

	tr->tr_command->cmd_hdr.header_desc.size_header = 128;

	cmd->param.opcode = TWA_OP_SET_PARAM;
	cmd->param.sgl_offset = 2;
	cmd->param.size = 2;
	cmd->param.request_id = tr->tr_request_id;
	cmd->param.unit = 0;
	cmd->param.param_count = 1;

	/* Specify which parameter we want to set. */
	param->table_id = table_id | TWA_9K_PARAM_DESCRIPTOR;
	param->parameter_id = param_id;
	param->parameter_size_bytes = param_size;
	memcpy(param->data, data, param_size);

	/* Submit the command. */
	if (callback == NULL) {
		/* There's no call back;  wait till the command completes. */
		error = twa_immediate_request(tr, TWA_REQUEST_TIMEOUT_PERIOD);
		if (error == ETIMEDOUT)
			return(error); /* clean-up done by twa_immediate_request */
		if (error)
			goto out;
		if ((error = cmd->param.status)) {
			goto out; /* twa_drain_complete_queue will have done the unmapping */
		}
		free(param, M_DEVBUF);
		twa_release_request(tr);
		return(error);
	} else {
		/* There's a call back.  Simply submit the command. */
		tr->tr_callback = callback;
		if ((error = twa_map_request(tr)))
			goto out;

		return (0);
	}
out:
	if (param)
		free(param, M_DEVBUF);
	if (tr)
		twa_release_request(tr);
	return(error);
}


/*
 * Function name:	twa_init_connection
 * Description:		Send init_connection cmd to firmware
 *
 * Input:		sc		-- ptr to per ctlr structure
 *			message_credits	-- max # of requests that we might send
 *					 down simultaneously.  This will be
 *					 typically set to 256 at init-time or
 *					after a reset, and to 1 at shutdown-time
 *			set_features	-- indicates if we intend to use 64-bit
 *					sg, also indicates if we want to do a
 *					basic or an extended init_connection;
 *
 * Note: The following input/output parameters are valid, only in case of an
 *		extended init_connection:
 *
 *			current_fw_srl		-- srl of fw we are bundled
 *						with, if any; 0 otherwise
 *			current_fw_arch_id	-- arch_id of fw we are bundled
 *						with, if any; 0 otherwise
 *			current_fw_branch	-- branch # of fw we are bundled
 *						with, if any; 0 otherwise
 *			current_fw_build	-- build # of fw we are bundled
 *						with, if any; 0 otherwise
 * Output:		fw_on_ctlr_srl		-- srl of fw on ctlr
 *			fw_on_ctlr_arch_id	-- arch_id of fw on ctlr
 *			fw_on_ctlr_branch	-- branch # of fw on ctlr
 *			fw_on_ctlr_build	-- build # of fw on ctlr
 *			init_connect_result	-- result bitmap of fw response
 * Return value:	0	-- success
 *			non-zero-- failure
 */
static int
twa_init_connection(struct twa_softc *sc, u_int16_t message_credits,
			u_int32_t set_features, u_int16_t current_fw_srl,
			u_int16_t current_fw_arch_id, u_int16_t current_fw_branch,
			u_int16_t current_fw_build, u_int16_t *fw_on_ctlr_srl,
			u_int16_t *fw_on_ctlr_arch_id, u_int16_t *fw_on_ctlr_branch,
			u_int16_t *fw_on_ctlr_build, u_int32_t *init_connect_result)
{
	struct twa_request		*tr;
	struct twa_command_init_connect	*init_connect;
	int				error = 1;

	/* Get a request packet. */
	if ((tr = twa_get_request(sc, 0)) == NULL)
		goto out;
	tr->tr_cmd_pkt_type |= TWA_CMD_PKT_TYPE_INTERNAL;
	/* Build the cmd pkt. */
	init_connect = &(tr->tr_command->command.cmd_pkt_7k.init_connect);

	tr->tr_command->cmd_hdr.header_desc.size_header = 128;

	init_connect->opcode = TWA_OP_INIT_CONNECTION;
   	init_connect->request_id = tr->tr_request_id;
	init_connect->message_credits = message_credits;
	init_connect->features = set_features;
	if (TWA_64BIT_ADDRESSES) {
		printf("64 bit addressing supported for scatter/gather list\n");
		init_connect->features |= TWA_64BIT_SG_ADDRESSES;
	}
	if (set_features & TWA_EXTENDED_INIT_CONNECT) {
		/*
		 * Fill in the extra fields needed for
		 * an extended init_connect.
		 */
		init_connect->size = 6;
		init_connect->fw_srl = current_fw_srl;
		init_connect->fw_arch_id = current_fw_arch_id;
		init_connect->fw_branch = current_fw_branch;
	} else
		init_connect->size = 3;

	/* Submit the command, and wait for it to complete. */
	error = twa_immediate_request(tr, TWA_REQUEST_TIMEOUT_PERIOD);
	if (error == ETIMEDOUT)
		return(error); /* clean-up done by twa_immediate_request */
	if (error)
		goto out;
	if ((error = init_connect->status)) {
		goto out; /* twa_drain_complete_queue will have done the unmapping */
	}
	if (set_features & TWA_EXTENDED_INIT_CONNECT) {
		*fw_on_ctlr_srl = init_connect->fw_srl;
		*fw_on_ctlr_arch_id = init_connect->fw_arch_id;
		*fw_on_ctlr_branch = init_connect->fw_branch;
		*fw_on_ctlr_build = init_connect->fw_build;
		*init_connect_result = init_connect->result;
	}
	twa_release_request(tr);
	return(error);

out:
	if (tr)
		twa_release_request(tr);
	return(error);
}


static int
twa_reset(struct twa_softc *sc)
{
	int	s;
	int	error = 0;

	/*
	 * Disable interrupts from the controller, and mask any
	 * accidental entry into our interrupt handler.
	 */
	twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
		TWA_CONTROL_DISABLE_INTERRUPTS);

	s = splbio();

	/* Soft reset the controller. */
	if ((error = twa_soft_reset(sc)))
		goto out;

	/* Re-establish logical connection with the controller. */
	if ((error = twa_init_connection(sc, TWA_INIT_MESSAGE_CREDITS,
					0, 0, 0, 0, 0,
					NULL, NULL, NULL, NULL, NULL))) {
		goto out;
	}
	/*
	 * Complete all requests in the complete queue; error back all requests
	 * in the busy queue.  Any internal requests will be simply freed.
	 * Re-submit any requests in the pending queue.
	 */
	twa_drain_busy_queue(sc);

out:
	splx(s);
	/*
	 * Enable interrupts, and also clear attention and response interrupts.
	 */
	twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
		TWA_CONTROL_CLEAR_ATTENTION_INTERRUPT |
		TWA_CONTROL_UNMASK_RESPONSE_INTERRUPT |
		TWA_CONTROL_ENABLE_INTERRUPTS);
	return(error);
}


static int
twa_soft_reset(struct twa_softc *sc)
{
	u_int32_t	status_reg;

	twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
			TWA_CONTROL_ISSUE_SOFT_RESET |
			TWA_CONTROL_CLEAR_HOST_INTERRUPT |
			TWA_CONTROL_CLEAR_ATTENTION_INTERRUPT |
			TWA_CONTROL_MASK_COMMAND_INTERRUPT |
			TWA_CONTROL_MASK_RESPONSE_INTERRUPT |
			TWA_CONTROL_DISABLE_INTERRUPTS);

	if (twa_wait_status(sc, TWA_STATUS_MICROCONTROLLER_READY |
				TWA_STATUS_ATTENTION_INTERRUPT, 30)) {
		aprint_error("%s: no attention interrupt after reset.\n",
			sc->twa_dv.dv_xname);
		return(1);
	}
	twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
		TWA_CONTROL_CLEAR_ATTENTION_INTERRUPT);

	if (twa_drain_response_queue(sc)) {
		aprint_error("%s: cannot drain response queue.\n",sc->twa_dv.dv_xname);
		return(1);
	}
	if (twa_drain_aen_queue(sc)) {
		aprint_error("%s: cannot drain AEN queue.\n", sc->twa_dv.dv_xname);
		return(1);
	}
	if (twa_find_aen(sc, TWA_AEN_SOFT_RESET)) {
		aprint_error("%s: reset not reported by controller.\n",
			 sc->twa_dv.dv_xname);
		return(1);
	}
	status_reg = twa_inl(sc, TWA_STATUS_REGISTER_OFFSET);
	if (TWA_STATUS_ERRORS(status_reg) ||
				twa_check_ctlr_state(sc, status_reg)) {
		aprint_error("%s: controller errors detected.\n", sc->twa_dv.dv_xname);
		return(1);
	}
	return(0);
}


static int
twa_wait_status(struct twa_softc *sc, u_int32_t status, u_int32_t timeout)
{
	struct timeval		t1;
	time_t		end_time;
	u_int32_t	status_reg;

	timeout = (timeout * 1000 * 100);

	microtime(&t1);

	end_time = t1.tv_usec + timeout;

	do {
		status_reg = twa_inl(sc, TWA_STATUS_REGISTER_OFFSET);
		if ((status_reg & status) == status)/* got the required bit(s)? */
			return(0);
		DELAY(100000);
		microtime(&t1);
	} while (t1.tv_usec <= end_time);

	return(1);
}


static int
twa_fetch_aen(struct twa_softc *sc)
{
	struct twa_request	*tr;
	int			s, error = 0;

	s = splbio();

	if ((tr = twa_get_request(sc, TWA_CMD_AEN)) == NULL)
		return(EIO);
	tr->tr_cmd_pkt_type |= TWA_CMD_PKT_TYPE_INTERNAL;
	tr->tr_callback = twa_aen_callback;
	tr->tr_data = malloc(TWA_SECTOR_SIZE, M_DEVBUF, M_NOWAIT);
	if (twa_request_sense(tr, 0) != 0) {
		if (tr->tr_data)
			free(tr->tr_data, M_DEVBUF);
		twa_release_request(tr);
		error = 1;
	}
	splx(s);

	return(error);
}



/*
 * Function name:	twa_aen_callback
 * Description:		Callback for requests to fetch AEN's.
 *
 * Input:		tr	-- ptr to completed request pkt
 * Output:		None
 * Return value:	None
 */
static void
twa_aen_callback(struct twa_request *tr)
{
	int i;
	int fetch_more_aens = 0;
	struct twa_softc		*sc = tr->tr_sc;
	struct twa_command_header	*cmd_hdr =
		(struct twa_command_header *)(tr->tr_data);
	struct twa_command_9k		*cmd =
		&(tr->tr_command->command.cmd_pkt_9k);

	if (! cmd->status) {
		if ((tr->tr_cmd_pkt_type & TWA_CMD_PKT_TYPE_9K) &&
			(cmd->cdb[0] == 0x3 /* REQUEST_SENSE */))
			if (twa_enqueue_aen(sc, cmd_hdr)
				!= TWA_AEN_QUEUE_EMPTY)
				fetch_more_aens = 1;
	} else {
		cmd_hdr->err_specific_desc[sizeof(cmd_hdr->err_specific_desc) - 1] = '\0';
		for (i = 0; i < 18; i++)
			printf("%x\t", tr->tr_command->cmd_hdr.sense_data[i]);

		printf(""); /* print new line */

		for (i = 0; i < 128; i++)
			printf("%x\t", ((int8_t *)(tr->tr_data))[i]);
	}
	if (tr->tr_data)
		free(tr->tr_data, M_DEVBUF);
	twa_release_request(tr);

	if (fetch_more_aens)
		twa_fetch_aen(sc);
}


/*
 * Function name:	twa_enqueue_aen
 * Description:		Queues AEN's to be supplied to user-space tools on request.
 *
 * Input:		sc	-- ptr to per ctlr structure
 *			cmd_hdr	-- ptr to hdr of fw cmd pkt, from where the AEN
 *				   details can be retrieved.
 * Output:		None
 * Return value:	None
 */
static uint16_t
twa_enqueue_aen(struct twa_softc *sc, struct twa_command_header *cmd_hdr)
{
	int			rv, s;
	struct tw_cl_event_packet *event;
	uint16_t		aen_code;
	unsigned long		sync_time;

	s = splbio();
	aen_code = cmd_hdr->status_block.error;

	switch (aen_code) {
	case TWA_AEN_SYNC_TIME_WITH_HOST:

		sync_time = (time.tv_sec - (3 * 86400)) % 604800;
		rv = twa_set_param(sc, TWA_PARAM_TIME_TABLE,
				TWA_PARAM_TIME_SchedulerTime, 4,
				&sync_time, twa_aen_callback);
#ifdef DIAGNOSTIC
		if (rv != 0)
			printf("%s: unable to sync time with ctlr\n",
				sc->twa_dv.dv_xname);
#endif
		break;

	case TWA_AEN_QUEUE_EMPTY:
		break;

	default:
		/* Queue the event. */
		event = sc->twa_aen_queue[sc->twa_aen_head];
		if (event->retrieved == TWA_AEN_NOT_RETRIEVED)
			sc->twa_aen_queue_overflow = TRUE;
		event->severity =
			cmd_hdr->status_block.substatus_block.severity;
		event->time_stamp_sec = time.tv_sec;
		event->aen_code = aen_code;
		event->retrieved = TWA_AEN_NOT_RETRIEVED;
		event->sequence_id = ++(sc->twa_current_sequence_id);
		cmd_hdr->err_specific_desc[sizeof(cmd_hdr->err_specific_desc) - 1] = '\0';
		event->parameter_len = strlen(cmd_hdr->err_specific_desc);
		memcpy(event->parameter_data, cmd_hdr->err_specific_desc,
			event->parameter_len);

		if (event->severity < TWA_AEN_SEVERITY_DEBUG) {
			printf("%s: AEN 0x%04X: %s: %s: %s\n",
				sc->twa_dv.dv_xname,
				aen_code,
				twa_aen_severity_table[event->severity],
				twa_find_msg_string(twa_aen_table, aen_code),
				event->parameter_data);
		}

		if ((sc->twa_aen_head + 1) == TWA_Q_LENGTH)
			sc->twa_aen_queue_wrapped = TRUE;
		sc->twa_aen_head = (sc->twa_aen_head + 1) % TWA_Q_LENGTH;
		break;
	} /* switch */
	splx(s);

	return (aen_code);
}



/*
 * Function name:	twa_find_aen
 * Description:		Reports whether a given AEN ever occurred.
 *
 * Input:		sc	-- ptr to per ctlr structure
 *			aen_code-- AEN to look for
 * Output:		None
 * Return value:	0	-- success
 *			non-zero-- failure
 */
static int
twa_find_aen(struct twa_softc *sc, u_int16_t aen_code)
{
	u_int32_t	last_index;
	int		s;
	int		i;

	s = splbio();

	if (sc->twa_aen_queue_wrapped)
		last_index = sc->twa_aen_head;
	else
		last_index = 0;

	i = sc->twa_aen_head;
	do {
		i = (i + TWA_Q_LENGTH - 1) % TWA_Q_LENGTH;
		if ((sc->twa_aen_queue[i])->aen_code == aen_code) {
			splx(s);
			return(0);
		}
	} while (i != last_index);

	splx(s);
	return(1);
}

static void inline
twa_request_init(struct twa_request *tr, int flags)
{
	tr->tr_data = NULL;
	tr->tr_real_data = NULL;
	tr->tr_length = 0;
	tr->tr_real_length = 0;
	tr->tr_status = TWA_CMD_SETUP;/* command is in setup phase */
	tr->tr_flags = flags;
	tr->tr_error = 0;
	tr->tr_callback = NULL;
	tr->tr_cmd_pkt_type = 0;

	/*
	 * Look at the status field in the command packet to see how
	 * it completed the last time it was used, and zero out only
	 * the portions that might have changed.  Note that we don't
	 * care to zero out the sglist.
	 */
	if (tr->tr_command->command.cmd_pkt_9k.status)
		memset(tr->tr_command, 0,
			sizeof(struct twa_command_header) + 28);
	else
		memset(&(tr->tr_command->command), 0, 28);
}

struct twa_request *
twa_get_request_wait(struct twa_softc *sc, int flags)
{
	struct twa_request *tr;
	int s;

	KASSERT((flags & TWA_CMD_AEN) == 0);

	s = splbio();
	while ((tr = TAILQ_FIRST(&sc->twa_free)) == NULL) {
		sc->twa_sc_flags |= TWA_STATE_REQUEST_WAIT;
		(void) tsleep(&sc->twa_free, PRIBIO, "twaccb", hz);
	}
	TAILQ_REMOVE(&sc->twa_free, tr, tr_link);

	splx(s);

	twa_request_init(tr, flags);

	return(tr);
}


struct twa_request *
twa_get_request(struct twa_softc *sc, int flags)
{
	int s;
	struct twa_request *tr;

	/* Get a free request packet. */
	s = splbio();
	if (__predict_false((flags & TWA_CMD_AEN) != 0)) {

		if ((sc->sc_twa_request->tr_flags & TWA_CMD_AEN_BUSY) == 0) {
			tr = sc->sc_twa_request;
			flags |= TWA_CMD_AEN_BUSY;
		} else {
			splx(s);
			return (NULL);
		}
	} else {
		if (__predict_false((tr =
				TAILQ_FIRST(&sc->twa_free)) == NULL)) {
			splx(s);
			return (NULL);
		}
		TAILQ_REMOVE(&sc->twa_free, tr, tr_link);
	}
	splx(s);

	twa_request_init(tr, flags);

	return(tr);
}


/*
 * Print some information about the controller
 */
static void
twa_describe_controller(struct twa_softc *sc)
{
	struct twa_param_9k	*p[10];
	int			i, rv = 0;
	uint32_t		dsize;
	uint8_t			ports;

	memset(p, sizeof(struct twa_param_9k *), 10);

	/* Get the port count. */
	rv |= twa_get_param(sc, TWA_PARAM_CONTROLLER,
		TWA_PARAM_CONTROLLER_PortCount, 1, NULL, &p[0]);

	/* get version strings */
	rv |= twa_get_param(sc, TWA_PARAM_VERSION, TWA_PARAM_VERSION_FW,
		16, NULL, &p[1]);
	rv |= twa_get_param(sc, TWA_PARAM_VERSION, TWA_PARAM_VERSION_BIOS,
		16, NULL, &p[2]);
	rv |= twa_get_param(sc, TWA_PARAM_VERSION, TWA_PARAM_VERSION_Mon,
		16, NULL, &p[3]);
	rv |= twa_get_param(sc, TWA_PARAM_VERSION, TWA_PARAM_VERSION_PCBA,
		8, NULL, &p[4]);
	rv |= twa_get_param(sc, TWA_PARAM_VERSION, TWA_PARAM_VERSION_ATA,
		8, NULL, &p[5]);
	rv |= twa_get_param(sc, TWA_PARAM_VERSION, TWA_PARAM_VERSION_PCI,
		8, NULL, &p[6]);
	rv |= twa_get_param(sc, TWA_PARAM_DRIVESUMMARY, TWA_PARAM_DRIVESTATUS,
		16, NULL, &p[7]);

	if (rv) {
		/* some error occurred */
		aprint_error("%s: failed to fetch version information\n",
			sc->twa_dv.dv_xname);
		goto bail;
	}

	ports = *(u_int8_t *)(p[0]->data);

	aprint_normal("%s: %d ports, Firmware %.16s, BIOS %.16s\n",
		sc->twa_dv.dv_xname, ports,
		p[1]->data, p[2]->data);

	aprint_verbose("%s: Monitor %.16s, PCB %.8s, Achip %.8s, Pchip %.8s\n",
		sc->twa_dv.dv_xname,
		p[3]->data, p[4]->data,
		p[5]->data, p[6]->data);

	for (i = 0; i < ports; i++) {

		if ((*((char *)(p[7]->data + i)) & TWA_DRIVE_DETECTED) == 0)
			continue;

		rv = twa_get_param(sc, TWA_PARAM_DRIVE_TABLE,
			TWA_PARAM_DRIVEMODELINDEX,
			TWA_PARAM_DRIVEMODEL_LENGTH, NULL, &p[8]);

		if (rv != 0) {
			aprint_error("%s: unable to get drive model for port"
				" %d\n", sc->twa_dv.dv_xname, i);
			continue;
		}

		rv = twa_get_param(sc, TWA_PARAM_DRIVE_TABLE,
			TWA_PARAM_DRIVESIZEINDEX,
			TWA_PARAM_DRIVESIZE_LENGTH, NULL, &p[9]);

		if (rv != 0) {
			aprint_error("%s: unable to get drive size"
				" for port %d\n", sc->twa_dv.dv_xname,
					i);
			free(p[8], M_DEVBUF);
			continue;
		}

		dsize = *(uint32_t *)(p[9]->data);

		aprint_verbose("%s: port %d: %.40s %d MB\n",
		    sc->twa_dv.dv_xname, i, p[8]->data, dsize / 2048);

		if (p[8])
			free(p[8], M_DEVBUF);
		if (p[9])
			free(p[9], M_DEVBUF);
	}
bail:
	if (p[0])
		free(p[0], M_DEVBUF);
	if (p[1])
		free(p[1], M_DEVBUF);
	if (p[2])
		free(p[2], M_DEVBUF);
	if (p[3])
		free(p[3], M_DEVBUF);
	if (p[4])
		free(p[4], M_DEVBUF);
	if (p[5])
		free(p[5], M_DEVBUF);
	if (p[6])
		free(p[6], M_DEVBUF);
}



/*
 * Function name:	twa_check_ctlr_state
 * Description:		Makes sure that the fw status register reports a
 *			proper status.
 *
 * Input:		sc		-- ptr to per ctlr structure
 *			status_reg	-- value in the status register
 * Output:		None
 * Return value:	0	-- no errors
 *			non-zero-- errors
 */
static int
twa_check_ctlr_state(struct twa_softc *sc, u_int32_t status_reg)
{
	int		result = 0;
	struct timeval	t1;
	static time_t	last_warning[2] = {0, 0};

	/* Check if the 'micro-controller ready' bit is not set. */
	if ((status_reg & TWA_STATUS_EXPECTED_BITS) !=
				TWA_STATUS_EXPECTED_BITS) {

		microtime(&t1);

		last_warning[0] += (5 * 1000 * 100);

		if (t1.tv_usec > last_warning[0]) {
			microtime(&t1);
			last_warning[0] = t1.tv_usec;
		}
		result = 1;
	}

	/* Check if any error bits are set. */
	if ((status_reg & TWA_STATUS_UNEXPECTED_BITS) != 0) {

		microtime(&t1);
		last_warning[1] += (5 * 1000 * 100);
		if (t1.tv_usec > last_warning[1]) {
		     	microtime(&t1);
			last_warning[1] = t1.tv_usec;
		}
		if (status_reg & TWA_STATUS_PCI_PARITY_ERROR_INTERRUPT) {
			aprint_error("%s: clearing PCI parity error "
				"re-seat/move/replace card.\n",
				 sc->twa_dv.dv_xname);
			twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
				TWA_CONTROL_CLEAR_PARITY_ERROR);
			pci_conf_write(sc->pc, sc->tag,
				PCI_COMMAND_STATUS_REG,
				TWA_PCI_CONFIG_CLEAR_PARITY_ERROR);
			result = 1;
		}
		if (status_reg & TWA_STATUS_PCI_ABORT_INTERRUPT) {
			aprint_error("%s: clearing PCI abort\n",
				sc->twa_dv.dv_xname);
			twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
				TWA_CONTROL_CLEAR_PCI_ABORT);
			pci_conf_write(sc->pc, sc->tag,
				PCI_COMMAND_STATUS_REG,
				TWA_PCI_CONFIG_CLEAR_PCI_ABORT);
			result = 1;
		}
		if (status_reg & TWA_STATUS_QUEUE_ERROR_INTERRUPT) {
			aprint_error("%s: clearing controller queue error\n",
				sc->twa_dv.dv_xname);
			twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
				TWA_CONTROL_CLEAR_PCI_ABORT);
			result = 1;
		}
		if (status_reg & TWA_STATUS_SBUF_WRITE_ERROR) {
			aprint_error("%s: clearing SBUF write error\n",
				sc->twa_dv.dv_xname);
			twa_outl(sc, TWA_CONTROL_REGISTER_OFFSET,
				TWA_CONTROL_CLEAR_SBUF_WRITE_ERROR);
			result = 1;
		}
		if (status_reg & TWA_STATUS_MICROCONTROLLER_ERROR) {
			aprint_error("%s: micro-controller error\n",
				sc->twa_dv.dv_xname);
			result = 1;
		}
	}
	return(result);
}


