/*	$NetBSD: aacvar.h,v 1.1.8.2 2002/06/23 17:46:07 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 * Copyright (c) 2000 Michael Smith
 * Copyright (c) 2000 BSDi
 * Copyright (c) 2000 Niklas Hallqvist
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
 *	from FreeBSD: aacvar.h,v 1.1 2000/09/13 03:20:34 msmith Exp
 *	via OpenBSD: aacvar.h,v 1.2 2002/03/14 01:26:53 millert Exp
 */

#ifndef _PCI_AACVAR_H_
#define	_PCI_AACVAR_H_

/* Debugging */
#ifdef AAC_DEBUG
#define AAC_DPRINTF(mask, args) if ((aac_debug & (mask)) != 0) printf args
#define AAC_D_INTR	0x01
#define AAC_D_MISC	0x02
#define AAC_D_CMD	0x04
#define AAC_D_QUEUE	0x08
#define AAC_D_IO	0x10
extern int aac_debug;

#define AAC_PRINT_FIB(sc, fib)	aac_print_fib((sc), (fib), __FUNCTION__)
#else
#define AAC_DPRINTF(mask, args)
#define AAC_PRINT_FIB(sc, fib)
#endif

struct aac_code_lookup {
	char	*string;
	u_int32_t code;
};

extern const struct	 aac_code_lookup aac_command_status_table[];
extern const struct	 aac_code_lookup aac_cpu_variant[];
extern const struct	 aac_code_lookup aac_battery_platform[];
extern const struct	 aac_code_lookup aac_container_types[];

struct aac_softc;

/*
 * We allocate a small set of FIBs for the adapter to use to send us messages.
 */
#define AAC_ADAPTER_FIBS	8

/*
 * Firmware messages are passed in the printf buffer.
 */
#define AAC_PRINTF_BUFSIZE	256

/*
 * We wait this many seconds for the adapter to come ready if it is still
 * booting.
 */
#define AAC_BOOT_TIMEOUT	(3 * 60)

/*
 * Wait this long for a lost interrupt to get detected.
 */
#define AAC_WATCH_TIMEOUT	10000		/* 10000 * 1ms = 10s */

/*
 * Timeout for immediate commands.
 */
#define AAC_IMMEDIATE_TIMEOUT	30

/*
 * Delay 20ms after the qnotify in sync operations.  Experimentally deduced.
 */
#define AAC_SYNC_DELAY		20000

/*
 * The firmware interface allows for a 16-bit s/g list length.  We limit 
 * ourselves to a reasonable maximum and ensure alignment.
 */
#define AAC_MAX_SGENTRIES	17	/* max S/G entries, limit 65535 */		
#define	AAC_MAX_XFER		((AAC_MAX_SGENTRIES - 1) * PAGE_SIZE)

/*
 * Fixed sector size.
 */
#define	AAC_SECTOR_SIZE		512

/*
 * Number of CCBs to allocate, and to reserve for control operations.
 */
#define	AAC_NCCBS		256
#define	AAC_NCCBS_RESERVE	4

/*
 * Quirk listings.
 */
#define	AAC_QUIRK_PERC2QC	0x0001

/*
 * We gather a number of adapter-visible items into a single structure.
 *
 * The ordering of this strucure may be important; we copy the Linux driver:
 *
 * Adapter FIBs
 * Init struct
 * Queue headers (Comm Area)
 * Printf buffer
 *
 * In addition, we add:
 * Sync Fib
 */
struct aac_common {
	/* fibs for the controller to send us messages */
	struct aac_fib ac_fibs[AAC_ADAPTER_FIBS];

	/* the init structure */
	struct aac_adapter_init	ac_init;

	/* arena within which the queue structures are kept */
	u_int8_t ac_qbuf[sizeof(struct aac_queue_table) + AAC_QUEUE_ALIGN];

	/* buffer for text messages from the controller */
	char	ac_printf[AAC_PRINTF_BUFSIZE];
    
	/* fib for synchronous commands */
	struct aac_fib ac_sync_fib;
};

/*
 * Interface operations
 */
struct aac_interface {
	int	(*aif_get_fwstatus)(struct aac_softc *);
	void	(*aif_qnotify)(struct aac_softc *, int);
	int	(*aif_get_istatus)(struct aac_softc *);
	void	(*aif_set_istatus)(struct aac_softc *, int);
	void	(*aif_set_mailbox)(struct aac_softc *, u_int32_t,
				   u_int32_t, u_int32_t, u_int32_t, u_int32_t);
	int	(*aif_get_mailboxstatus)(struct aac_softc *);
	void	(*aif_set_interrupts)(struct aac_softc *, int);
};

#define AAC_GET_FWSTATUS(sc)		((sc)->sc_if.aif_get_fwstatus(sc))
#define AAC_QNOTIFY(sc, qbit) \
	((sc)->sc_if.aif_qnotify((sc), (qbit)))
#define AAC_GET_ISTATUS(sc)		((sc)->sc_if.aif_get_istatus(sc))
#define AAC_CLEAR_ISTATUS(sc, mask) \
	((sc)->sc_if.aif_set_istatus((sc), (mask)))
#define AAC_SET_MAILBOX(sc, command, arg0, arg1, arg2, arg3) \
	do {								\
		((sc)->sc_if.aif_set_mailbox((sc), (command), (arg0),	\
		    (arg1), (arg2), (arg3)));				\
	} while(0)
#define AAC_GET_MAILBOXSTATUS(sc) \
	((sc)->sc_if.aif_get_mailboxstatus(sc))
#define	AAC_MASK_INTERRUPTS(sc)	\
	((sc)->sc_if.aif_set_interrupts((sc), 0))
#define AAC_UNMASK_INTERRUPTS(sc) \
	((sc)->sc_if.aif_set_interrupts((sc), 1))

#define AAC_SETREG4(sc, reg, val) \
	bus_space_write_4((sc)->sc_memt, (sc)->sc_memh, (reg), (val))
#define AAC_GETREG4(sc, reg) \
	bus_space_read_4((sc)->sc_memt, (sc)->sc_memh, (reg))
#define AAC_SETREG2(sc, reg, val) \
	bus_space_write_2((sc)->sc_memt, (sc)->sc_memh, (reg), (val))
#define AAC_GETREG2(sc, reg) \
	bus_space_read_2((sc)->sc_memt, (sc)->sc_memh, (reg))
#define AAC_SETREG1(sc, reg, val) \
	bus_space_write_1((sc)->sc_memt, (sc)->sc_memh, (reg), (val))
#define AAC_GETREG1(sc, reg) \
	bus_space_read_1((sc)->sc_memt, (sc)->sc_memh, (reg))


/*
 * A command contol block, one for each corresponding command index of the
 * controller.
 */
struct aac_ccb {
	SIMPLEQ_ENTRY(aac_ccb)	ac_chain;

	struct aac_fib		*ac_fib;
	bus_addr_t		ac_fibphys;
	bus_dmamap_t		ac_dmamap_xfer;

	void			*ac_data;
	size_t			ac_datalen;
	u_int			ac_flags;

	void			(*ac_intr)(struct aac_ccb *);
	struct device		*ac_device;
	void			*ac_context;
};
#define AAC_CCB_MAPPED	 	0x01
#define AAC_CCB_COMPLETED 	0x02
#define AAC_CCB_DATA_IN		0x04
#define AAC_CCB_DATA_OUT	0x08

struct aac_drive {
	u_int	hd_present;
	u_int	hd_devtype;
	u_int	hd_size;
};

/*
 * Per-controller structure.
 */
struct aac_softc {
	struct device		sc_dv;
	void			*sc_ih;
	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_memh;
	bus_dma_tag_t		sc_dmat;

	struct FsaRevision	sc_revision;
	int			sc_hwif;
	int			sc_quirks;
	struct aac_interface	sc_if;

	struct aac_common	*sc_common;
	bus_dma_segment_t	sc_common_seg;
	bus_dmamap_t		sc_common_dmamap;
	struct aac_fib		*sc_fibs;
	bus_dma_segment_t	sc_fibs_seg;
	bus_dmamap_t		sc_fibs_dmamap;

	struct aac_ccb		*sc_ccbs;
	SIMPLEQ_HEAD(, aac_ccb)	sc_ccb_free;
	SIMPLEQ_HEAD(, aac_ccb)	sc_ccb_queue;
	SIMPLEQ_HEAD(, aac_ccb)	sc_ccb_complete;

	struct aac_queue_table	*sc_queues;
	struct aac_queue_entry	*sc_qentries[AAC_QUEUE_COUNT];
	struct aac_drive	sc_hdr[AAC_MAX_CONTAINERS];
	int			sc_nunits;
	int			sc_flags;
};
#define AAC_HWIF_I960RX		0
#define AAC_HWIF_STRONGARM	1

#define	AAC_ONLINE		2

struct aac_attach_args {
	int		aaca_unit;
};

#define	aaccf_unit	cf_loc[AACCF_UNIT]

int	aac_attach(struct aac_softc *);
void	aac_ccb_enqueue(struct aac_softc *, struct aac_ccb *);
void	aac_ccb_free(struct aac_softc *, struct aac_ccb *);
struct aac_ccb *aac_ccb_alloc(struct aac_softc *, int);
int	aac_ccb_map(struct aac_softc *, struct aac_ccb *);
int	aac_ccb_poll(struct aac_softc *, struct aac_ccb *, int);
int	aac_ccb_submit(struct aac_softc *, struct aac_ccb *);
void	aac_ccb_unmap(struct aac_softc *, struct aac_ccb *);
const char	*aac_describe_code(const struct aac_code_lookup *, u_int32_t);
int	aac_intr(void *);

#endif	/* !_PCI_AACVAR_H_ */
