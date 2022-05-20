/*	$NetBSD: vmbusvar.h,v 1.7 2022/05/20 13:55:17 nonaka Exp $	*/
/*	$OpenBSD: hypervvar.h,v 1.13 2017/06/23 19:05:42 mikeb Exp $	*/

/*
 * Copyright (c) 2016 Mike Belopuhov <mike@esdenera.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
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

#ifndef _VMBUSVAR_H_
#define _VMBUSVAR_H_

#include <sys/param.h>
#include <sys/device.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/condvar.h>
#include <sys/evcnt.h>
#include <sys/kcpuset.h>
#include <sys/mutex.h>
#include <sys/pool.h>

#include <dev/hyperv/hypervreg.h>
#include <dev/hyperv/hypervvar.h>

/* #define HYPERV_DEBUG */

#ifdef HYPERV_DEBUG
#define DPRINTF(x...)		printf(x)
#else
#define DPRINTF(x...)
#endif

typedef void (*vmbus_channel_callback_t)(void *);

struct vmbus_softc;
struct vmbus_channel;

struct vmbus_msg {
	uint64_t			msg_flags;
#define  MSGF_NOSLEEP			__BIT(0)
#define  MSGF_NOQUEUE			__BIT(1)
#define  MSGF_ORPHANED			__BIT(2)
	struct hyperv_hypercall_postmsg_in msg_req __aligned(8);
	void				*msg_rsp;
	size_t				msg_rsplen;
	TAILQ_ENTRY(vmbus_msg)		msg_entry;
};
__CTASSERT((offsetof(struct vmbus_msg, msg_req) % 8) == 0);
TAILQ_HEAD(vmbus_queue, vmbus_msg);

struct vmbus_chev {
	int				vce_type;
#define VMBUS_CHEV_TYPE_OFFER		0
#define VMBUS_CHEV_TYPE_RESCIND		1
	void				*vce_arg;
	SIMPLEQ_ENTRY(vmbus_chev)	vce_entry;
};
SIMPLEQ_HEAD(vmbus_chevq, vmbus_chev);

struct vmbus_dev {
	int					vd_type;
#define VMBUS_DEV_TYPE_ATTACH			0
#define VMBUS_DEV_TYPE_DETACH			1
	struct vmbus_channel			*vd_chan;
	SIMPLEQ_ENTRY(vmbus_dev)		vd_entry;
};
SIMPLEQ_HEAD(vmbus_devq, vmbus_dev);

struct vmbus_ring_data {
	struct vmbus_bufring		*rd_ring;
	uint32_t			rd_size;
	kmutex_t			rd_lock;
	uint32_t			rd_prod;
	uint32_t			rd_cons;
	uint32_t			rd_dsize;
};

TAILQ_HEAD(vmbus_channels, vmbus_channel);

struct vmbus_channel {
	struct vmbus_softc		*ch_sc;
	device_t			ch_dev;
	u_int				ch_refs;

	int				ch_state;
#define  VMBUS_CHANSTATE_OFFERED	1
#define  VMBUS_CHANSTATE_OPENED		2
#define  VMBUS_CHANSTATE_CLOSING	3
#define  VMBUS_CHANSTATE_CLOSED		4
	uint32_t			ch_id;
	uint16_t			ch_subidx;

	struct hyperv_guid		ch_type;
	struct hyperv_guid		ch_inst;
	char				ch_ident[38];

	void				*ch_ring;
	uint32_t			ch_ring_gpadl;
	u_long				ch_ring_size;
	struct hyperv_dma		ch_ring_dma;

	struct vmbus_ring_data		ch_wrd;
	struct vmbus_ring_data		ch_rrd;

	int				ch_cpuid;
	uint32_t			ch_vcpu;

	void				(*ch_handler)(void *);
	void				*ch_ctx;
	struct evcnt			ch_evcnt;
	void				*ch_taskq;

	kmutex_t			ch_event_lock;
	kcondvar_t			ch_event_cv;

	uint32_t			ch_flags;
#define  CHF_BATCHED			__BIT(0)
#define  CHF_MONITOR			__BIT(1)
#define  CHF_REVOKED			__BIT(2)

	uint8_t				ch_mgroup;
	uint8_t				ch_mindex;
	struct hyperv_mon_param		*ch_monprm;
	struct hyperv_dma		ch_monprm_dma;

	TAILQ_ENTRY(vmbus_channel)	ch_prientry;
	TAILQ_ENTRY(vmbus_channel)	ch_entry;

	kmutex_t			ch_subchannel_lock;
	kcondvar_t			ch_subchannel_cv;
	struct vmbus_channels		ch_subchannels;
	u_int				ch_subchannel_count;
	TAILQ_ENTRY(vmbus_channel)	ch_subentry;
	struct vmbus_channel		*ch_primary_channel;
};

#define VMBUS_CHAN_ISPRIMARY(chan)	((chan)->ch_subidx == 0)

struct vmbus_attach_args {
	struct hyperv_guid		*aa_type;
	struct hyperv_guid		*aa_inst;
	char				*aa_ident;
	struct vmbus_channel		*aa_chan;
	bus_space_tag_t			aa_iot;
	bus_space_tag_t			aa_memt;
};

struct vmbus_percpu_data {
	void			*simp;	/* Synthetic Interrupt Message Page */
	void			*siep;	/* Synthetic Interrupt Event Flags Page */

	/* Rarely used fields */
	struct hyperv_dma	simp_dma;
	struct hyperv_dma	siep_dma;

	void			*md_cookie;
} __aligned(CACHE_LINE_SIZE);

struct vmbus_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_tag_t		sc_memt;
	bus_dma_tag_t		sc_dmat;

	pool_cache_t		sc_msgpool;

	void			*sc_msg_sih;

	u_long			*sc_wevents;	/* Write events */
	u_long			*sc_revents;	/* Read events */
	struct vmbus_channel * volatile *sc_chanmap;
	volatile u_long		sc_evtmask[VMBUS_EVTFLAGS_MAX];
	struct vmbus_mnf	*sc_monitor[2];
	struct vmbus_percpu_data sc_percpu[MAXCPUS];

	/*
	 * Rarely used fields
	 */
	uint32_t		sc_flags;
#define  VMBUS_SCFLAG_SYNIC		__BIT(0)
#define  VMBUS_SCFLAG_CONNECTED		__BIT(1)
#define  VMBUS_SCFLAG_OFFERS_DELIVERED	__BIT(2)
	uint32_t		sc_proto;
	int			sc_channel_max;

	kcpuset_t		*sc_intr_cpuset;

	/* Shared memory for Write/Read events */
	void			*sc_events;
	struct hyperv_dma	sc_events_dma;

	struct hyperv_dma	sc_monitor_dma[2];

	struct vmbus_queue 	sc_reqs;	/* Request queue */
	kmutex_t		sc_req_lock;
	struct vmbus_queue 	sc_rsps;	/* Response queue */
	kmutex_t		sc_rsp_lock;

	struct vmbus_chevq	sc_chevq;
	kmutex_t		sc_chevq_lock;
	kcondvar_t		sc_chevq_cv;

	struct vmbus_devq	sc_devq;
	kmutex_t		sc_devq_lock;
	kcondvar_t		sc_devq_cv;

	struct vmbus_devq	sc_subch_devq;
	kmutex_t		sc_subch_devq_lock;
	kcondvar_t		sc_subch_devq_cv;

	struct vmbus_channels	sc_prichans;
	kmutex_t		sc_prichan_lock;

	struct vmbus_channels	sc_channels;
	kmutex_t		sc_channel_lock;

	volatile uint32_t	sc_handle;
};

static __inline void
clear_bit(u_int b, volatile void *p)
{
	atomic_and_32(((volatile u_int *)p) + (b >> 5), ~(1 << (b & 0x1f)));
}

static __inline void
set_bit(u_int b, volatile void *p)
{
	atomic_or_32(((volatile u_int *)p) + (b >> 5), 1 << (b & 0x1f));
}

static __inline int
test_bit(u_int b, volatile void *p)
{
	return !!(((volatile u_int *)p)[b >> 5] & (1 << (b & 0x1f)));
}

extern const struct hyperv_guid hyperv_guid_network;
extern const struct hyperv_guid hyperv_guid_ide;
extern const struct hyperv_guid hyperv_guid_scsi;
extern const struct hyperv_guid hyperv_guid_shutdown;
extern const struct hyperv_guid hyperv_guid_timesync;
extern const struct hyperv_guid hyperv_guid_heartbeat;
extern const struct hyperv_guid hyperv_guid_kvp;
extern const struct hyperv_guid hyperv_guid_vss;
extern const struct hyperv_guid hyperv_guid_dynmem;
extern const struct hyperv_guid hyperv_guid_mouse;
extern const struct hyperv_guid hyperv_guid_kbd;
extern const struct hyperv_guid hyperv_guid_video;
extern const struct hyperv_guid hyperv_guid_fc;
extern const struct hyperv_guid hyperv_guid_fcopy;
extern const struct hyperv_guid hyperv_guid_pcie;
extern const struct hyperv_guid hyperv_guid_netdir;
extern const struct hyperv_guid hyperv_guid_rdesktop;
extern const struct hyperv_guid hyperv_guid_avma1;
extern const struct hyperv_guid hyperv_guid_avma2;
extern const struct hyperv_guid hyperv_guid_avma3;
extern const struct hyperv_guid hyperv_guid_avma4;

int	vmbus_match(device_t, cfdata_t, void *);
int	vmbus_attach(struct vmbus_softc *);
int	vmbus_detach(struct vmbus_softc *, int);

int	vmbus_handle_alloc(struct vmbus_channel *, const struct hyperv_dma *,
	    uint32_t, uint32_t *);
void	vmbus_handle_free(struct vmbus_channel *, uint32_t);
int	vmbus_channel_open(struct vmbus_channel *, size_t, void *, size_t,
	    vmbus_channel_callback_t, void *);
int	vmbus_channel_close(struct vmbus_channel *);
int	vmbus_channel_close_direct(struct vmbus_channel *);
int	vmbus_channel_setdeferred(struct vmbus_channel *, const char *);
void	vmbus_channel_schedule(struct vmbus_channel *);
int	vmbus_channel_send(struct vmbus_channel *, void *, uint32_t, uint64_t,
	    int, uint32_t);
int	vmbus_channel_send_sgl(struct vmbus_channel *, struct vmbus_gpa *,
	    uint32_t, void *, uint32_t, uint64_t);
int	vmbus_channel_send_prpl(struct vmbus_channel *,
	    struct vmbus_gpa_range *, uint32_t, void *, uint32_t, uint64_t);
int	vmbus_channel_recv(struct vmbus_channel *, void *, uint32_t, uint32_t *,
	    uint64_t *, int);
void	vmbus_channel_cpu_set(struct vmbus_channel *, int);
void	vmbus_channel_cpu_rr(struct vmbus_channel *);
bool	vmbus_channel_is_revoked(struct vmbus_channel *);
bool	vmbus_channel_tx_empty(struct vmbus_channel *);
bool	vmbus_channel_rx_empty(struct vmbus_channel *);
void	vmbus_channel_pause(struct vmbus_channel *);
uint32_t vmbus_channel_unpause(struct vmbus_channel *);
uint32_t vmbus_channel_ready(struct vmbus_channel *);

struct vmbus_channel **
	vmbus_subchannel_get(struct vmbus_channel *, int);
void	vmbus_subchannel_rel(struct vmbus_channel **, int);
void	vmbus_subchannel_drain(struct vmbus_channel *);

#endif	/* _VMBUSVAR_H_ */
