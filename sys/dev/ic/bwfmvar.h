/* $NetBSD: bwfmvar.h,v 1.13 2022/03/14 06:40:12 mlelstv Exp $ */
/* $OpenBSD: bwfmvar.h,v 1.1 2017/10/11 17:19:50 patrick Exp $ */
/*
 * Copyright (c) 2010-2016 Broadcom Corporation
 * Copyright (c) 2016,2017 Patrick Wildt <patrick@blueri.se>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef	_DEV_IC_BWFMVAR_H
#define	_DEV_IC_BWFMVAR_H

#include <sys/types.h>

#include <sys/cdefs.h>
#include <sys/device_if.h>
#include <sys/queue.h>
#include <sys/workqueue.h>

#include <net/if_ether.h>

#include <net80211/ieee80211.h>
#include <net80211/ieee80211_proto.h>
#include <net80211/ieee80211_var.h>

struct ieee80211_key;
struct mbuf;
struct pool_cache;

/* Chipcommon Core Chip IDs */
#define BRCM_CC_43143_CHIP_ID		43143
#define BRCM_CC_43235_CHIP_ID		43235
#define BRCM_CC_43236_CHIP_ID		43236
#define BRCM_CC_43238_CHIP_ID		43238
#define BRCM_CC_43241_CHIP_ID		0x4324
#define BRCM_CC_43242_CHIP_ID		43242
#define BRCM_CC_4329_CHIP_ID		0x4329
#define BRCM_CC_4330_CHIP_ID		0x4330
#define BRCM_CC_4334_CHIP_ID		0x4334
#define BRCM_CC_43340_CHIP_ID		43340
#define BRCM_CC_43341_CHIP_ID		43341
#define BRCM_CC_43362_CHIP_ID		43362
#define BRCM_CC_43364_CHIP_ID		43364
#define BRCM_CC_4335_CHIP_ID		0x4335
#define BRCM_CC_4339_CHIP_ID		0x4339
#define BRCM_CC_43430_CHIP_ID		43430
#define BRCM_CC_4345_CHIP_ID		0x4345
#define BRCM_CC_43465_CHIP_ID		43465
#define BRCM_CC_4350_CHIP_ID		0x4350
#define BRCM_CC_43525_CHIP_ID		43525
#define BRCM_CC_4354_CHIP_ID		0x4354
#define BRCM_CC_4356_CHIP_ID		0x4356
#define BRCM_CC_43566_CHIP_ID		43566
#define BRCM_CC_43567_CHIP_ID		43567
#define BRCM_CC_43569_CHIP_ID		43569
#define BRCM_CC_43570_CHIP_ID		43570
#define BRCM_CC_4358_CHIP_ID		0x4358
#define BRCM_CC_4359_CHIP_ID		0x4359
#define BRCM_CC_43602_CHIP_ID		43602
#define BRCM_CC_4365_CHIP_ID		0x4365
#define BRCM_CC_4366_CHIP_ID		0x4366
#define BRCM_CC_43664_CHIP_ID		43664
#define BRCM_CC_4371_CHIP_ID		0x4371
#define CY_CC_4373_CHIP_ID		0x4373
#define CY_CC_43012_CHIP_ID		43012

/* Defaults */
#define BWFM_DEFAULT_SCAN_CHANNEL_TIME	40
#define BWFM_DEFAULT_SCAN_UNASSOC_TIME	40
#define BWFM_DEFAULT_SCAN_PASSIVE_TIME	120

#define	BWFM_TASK_COUNT			256

struct bwfm_softc;

struct bwfm_firmware_selector {
	uint32_t	fwsel_chip;	/* chip ID */
	uint32_t	fwsel_revmask;	/* mask of applicable chip revs */
	const char	*fwsel_basename;/* firmware file base name */
};
#define	BWFM_FW_ENTRY(c, r, b)						\
	{ .fwsel_chip = (c),						\
	  .fwsel_revmask = (r),						\
	  .fwsel_basename = b }

#define	BWFM_FW_ENTRY_END						\
	{ .fwsel_basename = NULL }

#define	BWFM_FWSEL_REV_EQ(x)	__BIT(x)
#define	BWFM_FWSEL_REV_LE(x)	__BITS(0,x)
#define	BWFM_FWSEL_REV_GE(x)	__BITS(x,31)
#define	BWFM_FWSEL_ALLREVS	__BITS(0,31)

#define	BWFM_FILETYPE_UCODE	0
#define	BWFM_FILETYPE_NVRAM	1
#define	BWFM_FILETYPE_CLM	2
#define	BWFM_FILETYPE_TXCAP	3
#define	BWFM_FILETYPE_CAL	4
#define	BWFM_NFILETYPES		5

struct bwfm_firmware_context {
	/* inputs */
	uint32_t	ctx_chip;
	uint32_t	ctx_chiprev;
	const char *	ctx_model;
	uint32_t	ctx_req;

#define	BWFM_FWREQ(x)		__BIT(x)
#define	BWFM_FWOPT(x)		__BIT((x)+16)

	/* outputs */
	struct {
		void *	ctx_f_data;
		size_t	ctx_f_size;
	} ctx_file[BWFM_NFILETYPES];
};

struct bwfm_core {
	uint16_t	 co_id;
	uint16_t	 co_rev;
	uint32_t	 co_base;
	uint32_t	 co_wrapbase;
	LIST_ENTRY(bwfm_core) co_link;
};

struct bwfm_chip {
	uint32_t	 ch_chip;
	uint32_t	 ch_chiprev;
	uint32_t	 ch_cc_caps;
	uint32_t	 ch_cc_caps_ext;
	uint32_t	 ch_pmucaps;
	uint32_t	 ch_pmurev;
	uint32_t	 ch_rambase;
	uint32_t	 ch_ramsize;
	uint32_t	 ch_srsize;
	char		 ch_name[8];
	LIST_HEAD(,bwfm_core) ch_list;
	int (*ch_core_isup)(struct bwfm_softc *, struct bwfm_core *);
	void (*ch_core_disable)(struct bwfm_softc *, struct bwfm_core *,
	    uint32_t prereset, uint32_t reset);
	void (*ch_core_reset)(struct bwfm_softc *, struct bwfm_core *,
	    uint32_t prereset, uint32_t reset, uint32_t postreset);
};

struct bwfm_bus_ops {
	void (*bs_init)(struct bwfm_softc *);
	void (*bs_stop)(struct bwfm_softc *);
	int (*bs_txcheck)(struct bwfm_softc *);
	int (*bs_txdata)(struct bwfm_softc *, struct mbuf **);
	int (*bs_txctl)(struct bwfm_softc *, char *, size_t);
	int (*bs_rxctl)(struct bwfm_softc *, char *, size_t *);
};

struct bwfm_buscore_ops {
	uint32_t (*bc_read)(struct bwfm_softc *, uint32_t);
	void (*bc_write)(struct bwfm_softc *, uint32_t, uint32_t);
	int (*bc_prepare)(struct bwfm_softc *);
	int (*bc_reset)(struct bwfm_softc *);
	int (*bc_setup)(struct bwfm_softc *);
	void (*bc_activate)(struct bwfm_softc *, const uint32_t);
};

struct bwfm_proto_ops {
	int (*proto_query_dcmd)(struct bwfm_softc *, int, int,
	    char *, size_t *);
	int (*proto_set_dcmd)(struct bwfm_softc *, int, int,
	    char *, size_t);
};
extern const struct bwfm_proto_ops bwfm_proto_bcdc_ops;

enum bwfm_task_cmd {
	BWFM_TASK_NEWSTATE,
	BWFM_TASK_KEY_SET,
	BWFM_TASK_KEY_DELETE,
	BWFM_TASK_RX_EVENT,
};

struct bwfm_cmd_newstate {
	enum ieee80211_state	 state;
	int			 arg;
};

struct bwfm_cmd_key {
	const struct ieee80211_key *key;
	uint8_t			 mac[IEEE80211_ADDR_LEN];
};

struct bwfm_task {
	struct work		 t_work;
	struct bwfm_softc	*t_sc;
	enum bwfm_task_cmd	 t_cmd;
	union {
		struct bwfm_cmd_newstate	newstate;
		struct bwfm_cmd_key		key;
		struct mbuf			*mbuf;
	} t_u;
#define	t_newstate	t_u.newstate
#define	t_key		t_u.key
#define	t_mbuf		t_u.mbuf
};

struct bwfm_softc {
	device_t		 sc_dev;
	struct ieee80211com	 sc_ic;
	struct ethercom		 sc_ec;
#define	sc_if			 sc_ec.ec_if

	const struct bwfm_bus_ops	*sc_bus_ops;
	const struct bwfm_buscore_ops	*sc_buscore_ops;
	const struct bwfm_proto_ops	*sc_proto_ops;
	struct bwfm_chip	 sc_chip;
	uint8_t			 sc_io_type;
#define		BWFM_IO_TYPE_D11N		1
#define		BWFM_IO_TYPE_D11AC		2

	int			 sc_tx_timer;

	bool			 sc_if_attached;
	struct pool_cache	*sc_freetask;
	struct workqueue	*sc_taskq;

	int			(*sc_newstate)(struct ieee80211com *,
				    enum ieee80211_state, int);

	int			 sc_bcdc_reqid;

	union {
		struct bwfm_bss_info bss_info;
		uint8_t padding[BWFM_BSS_INFO_BUFLEN];
	}			sc_bss_buf;

	uint8_t			*sc_clm;
	size_t			sc_clmsize;
	uint8_t			*sc_txcap;
	size_t			sc_txcapsize;
	uint8_t			*sc_cal;
	size_t			sc_calsize;
};

void bwfm_attach(struct bwfm_softc *);
void bwfm_chip_socram_ramsize(struct bwfm_softc *, struct bwfm_core *);
void bwfm_chip_sysmem_ramsize(struct bwfm_softc *, struct bwfm_core *);
void bwfm_chip_tcm_ramsize(struct bwfm_softc *, struct bwfm_core *);
void bwfm_chip_tcm_rambase(struct bwfm_softc *);
void bwfm_start(struct ifnet *);
int bwfm_detach(struct bwfm_softc *, int);
int bwfm_chip_attach(struct bwfm_softc *);
int bwfm_chip_set_active(struct bwfm_softc *, uint32_t);
void bwfm_chip_set_passive(struct bwfm_softc *);
int bwfm_chip_sr_capable(struct bwfm_softc *);
struct bwfm_core *bwfm_chip_get_core(struct bwfm_softc *, int);
struct bwfm_core *bwfm_chip_get_pmu(struct bwfm_softc *);
void bwfm_rx(struct bwfm_softc *, struct mbuf *m);

void	bwfm_firmware_context_init(struct bwfm_firmware_context *,
	    uint32_t, uint32_t, const char *, uint32_t);
bool	bwfm_firmware_open(struct bwfm_softc *,
	    const struct bwfm_firmware_selector *,
	    struct bwfm_firmware_context *);
void	bwfm_firmware_close(struct bwfm_firmware_context *);
void *	bwfm_firmware_data(struct bwfm_firmware_context *,
	    unsigned int, size_t *);
const char *bwfm_firmware_description(unsigned int);

#endif	/* _DEV_IC_BWFMVAR_H */
