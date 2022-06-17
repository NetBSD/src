/*	$NetBSD: if_iavf.c,v 1.16 2022/06/17 06:18:09 yamaguchi Exp $	*/

/*
 * Copyright (c) 2013-2015, Intel Corporation
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2016,2017 David Gwynne <dlg@openbsd.org>
 * Copyright (c) 2019 Jonathan Matthew <jmatthew@openbsd.org>
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

/*
 * Copyright (c) 2020 Internet Initiative Japan, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_iavf.c,v 1.16 2022/06/17 06:18:09 yamaguchi Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/bitops.h>
#include <sys/bus.h>
#include <sys/cprng.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/interrupt.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/pcq.h>
#include <sys/queue.h>
#include <sys/syslog.h>
#include <sys/workqueue.h>
#include <sys/xcall.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/rss_config.h>

#include <netinet/tcp.h>	/* for struct tcphdr */
#include <netinet/udp.h>	/* for struct udphdr */

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_ixlreg.h>
#include <dev/pci/if_ixlvar.h>
#include <dev/pci/if_iavfvar.h>

#include <prop/proplib.h>

#define IAVF_PCIREG		PCI_MAPREG_START
#define IAVF_AQ_NUM		256
#define IAVF_AQ_MASK		(IAVF_AQ_NUM-1)
#define IAVF_AQ_ALIGN		64
#define IAVF_AQ_BUFLEN		4096
#define I40E_AQ_LARGE_BUF	512
#define IAVF_VF_MAJOR		1
#define IAVF_VF_MINOR		1

#define IAVF_VFR_INPROGRESS	0
#define IAVF_VFR_COMPLETED	1
#define IAVF_VFR_VFACTIVE	2

#define IAVF_REG_VFR			0xdeadbeef

#define IAVF_ITR_RX			0x0
#define IAVF_ITR_TX			0x1
#define IAVF_ITR_MISC			0x2
#define IAVF_NOITR			0x3

#define IAVF_MTU_ETHERLEN		(ETHER_HDR_LEN \
					+ ETHER_CRC_LEN)
#define IAVF_MAX_MTU			(9600 - IAVF_MTU_ETHERLEN)
#define IAVF_MIN_MTU			(ETHER_MIN_LEN - ETHER_CRC_LEN)

#define IAVF_WORKQUEUE_PRI	PRI_SOFTNET

#define IAVF_TX_PKT_DESCS		8
#define IAVF_TX_QUEUE_ALIGN		128
#define IAVF_RX_QUEUE_ALIGN		128
#define IAVF_TX_PKT_MAXSIZE		(MCLBYTES * IAVF_TX_PKT_DESCS)
#define IAVF_MCLBYTES			(MCLBYTES - ETHER_ALIGN)

#define IAVF_TICK_INTERVAL		(5 * hz)
#define IAVF_WATCHDOG_TICKS		3
#define IAVF_WATCHDOG_STOP		0

#define IAVF_TXRX_PROCESS_UNLIMIT	UINT_MAX
#define IAVF_TX_PROCESS_LIMIT		256
#define IAVF_RX_PROCESS_LIMIT		256
#define IAVF_TX_INTR_PROCESS_LIMIT	256
#define IAVF_RX_INTR_PROCESS_LIMIT	0U

#define IAVF_EXEC_TIMEOUT		3000

#define IAVF_IFCAP_RXCSUM	(IFCAP_CSUM_IPv4_Rx |	\
				 IFCAP_CSUM_TCPv4_Rx |	\
				 IFCAP_CSUM_UDPv4_Rx |	\
				 IFCAP_CSUM_TCPv6_Rx |	\
				 IFCAP_CSUM_UDPv6_Rx)
#define IAVF_IFCAP_TXCSUM	(IFCAP_CSUM_IPv4_Tx |	\
				 IFCAP_CSUM_TCPv4_Tx |	\
				 IFCAP_CSUM_UDPv4_Tx |	\
				 IFCAP_CSUM_TCPv6_Tx |	\
				 IFCAP_CSUM_UDPv6_Tx)
#define IAVF_CSUM_ALL_OFFLOAD	(M_CSUM_IPv4 |			\
				 M_CSUM_TCPv4 | M_CSUM_TCPv6 |	\
				 M_CSUM_UDPv4 | M_CSUM_UDPv6)

struct iavf_softc;	/* defined */

struct iavf_module_params {
	int		 debug;
	uint32_t	 rx_itr;
	uint32_t	 tx_itr;
	unsigned int	 rx_ndescs;
	unsigned int	 tx_ndescs;
	int		 max_qps;
};

struct iavf_product {
	unsigned int	 vendor_id;
	unsigned int	 product_id;
};

struct iavf_link_speed {
	uint64_t	baudrate;
	uint64_t	media;
};

struct iavf_aq_regs {
	bus_size_t		atq_tail;
	bus_size_t		atq_head;
	bus_size_t		atq_len;
	bus_size_t		atq_bal;
	bus_size_t		atq_bah;

	bus_size_t		arq_tail;
	bus_size_t		arq_head;
	bus_size_t		arq_len;
	bus_size_t		arq_bal;
	bus_size_t		arq_bah;

	uint32_t		atq_len_enable;
	uint32_t		atq_tail_mask;
	uint32_t		atq_head_mask;

	uint32_t		arq_len_enable;
	uint32_t		arq_tail_mask;
	uint32_t		arq_head_mask;
};

struct iavf_work {
	struct work	 ixw_cookie;
	void		(*ixw_func)(void *);
	void		*ixw_arg;
	unsigned int	 ixw_added;
};

struct iavf_tx_map {
	struct mbuf		*txm_m;
	bus_dmamap_t		 txm_map;
	unsigned int		 txm_eop;
};

struct iavf_tx_ring {
	unsigned int		 txr_qid;
	char			 txr_name[16];

	struct iavf_softc	*txr_sc;
	kmutex_t		 txr_lock;
	pcq_t			*txr_intrq;
	void			*txr_si;
	unsigned int		 txr_prod;
	unsigned int		 txr_cons;

	struct iavf_tx_map	*txr_maps;
	struct ixl_dmamem	 txr_mem;
	bus_size_t		 txr_tail;

	int			 txr_watchdog;

	struct evcnt		 txr_defragged;
	struct evcnt		 txr_defrag_failed;
	struct evcnt		 txr_pcqdrop;
	struct evcnt		 txr_transmitdef;
	struct evcnt		 txr_defer;
	struct evcnt		 txr_watchdogto;
	struct evcnt		 txr_intr;
};

struct iavf_rx_map {
	struct mbuf		*rxm_m;
	bus_dmamap_t		 rxm_map;
};

struct iavf_rx_ring {
	unsigned int		 rxr_qid;
	char			 rxr_name[16];

	struct iavf_softc	*rxr_sc;
	kmutex_t		 rxr_lock;

	unsigned int		 rxr_prod;
	unsigned int		 rxr_cons;

	struct iavf_rx_map	*rxr_maps;
	struct ixl_dmamem	 rxr_mem;
	bus_size_t		 rxr_tail;

	struct mbuf		*rxr_m_head;
	struct mbuf		**rxr_m_tail;

	struct evcnt		 rxr_mgethdr_failed;
	struct evcnt		 rxr_mgetcl_failed;
	struct evcnt		 rxr_mbuf_load_failed;
	struct evcnt		 rxr_defer;
	struct evcnt		 rxr_intr;
};

struct iavf_queue_pair {
	struct iavf_tx_ring	*qp_txr;
	struct iavf_rx_ring	*qp_rxr;
	struct work		 qp_work;
	void			*qp_si;
	bool			 qp_workqueue;
};

struct iavf_stat_counters {
	struct evcnt	 isc_rx_bytes;
	struct evcnt	 isc_rx_unicast;
	struct evcnt	 isc_rx_multicast;
	struct evcnt	 isc_rx_broadcast;
	struct evcnt	 isc_rx_discards;
	struct evcnt	 isc_rx_unknown_protocol;
	struct evcnt	 isc_tx_bytes;
	struct evcnt	 isc_tx_unicast;
	struct evcnt	 isc_tx_multicast;
	struct evcnt	 isc_tx_broadcast;
	struct evcnt	 isc_tx_discards;
	struct evcnt	 isc_tx_errors;
};

/*
 * Locking notes:
 * + A field in iavf_tx_ring is protected by txr_lock (a spin mutex), and
 *   A field in iavf_rx_ring is protected by rxr_lock (a spin mutex).
 *   - more than one lock must not be held at once.
 * + fields named sc_atq_*, sc_arq_*, and sc_adminq_* are protected by
 *   sc_adminq_lock(a spin mutex).
 *   - The lock is held while accessing sc_aq_regs
 *     and is not held with txr_lock and rxr_lock together.
 * + Other fields in iavf_softc is protected by sc_cfg_lock
 *   (an adaptive mutex).
 *   - The lock must be held before acquiring another lock.
 *
 * Locking order:
 *   - IFNET_LOCK => sc_cfg_lock => sc_adminq_lock
 *   - sc_cfg_lock => ETHER_LOCK => sc_adminq_lock
 *   - sc_cfg_lock => txr_lock
 *   - sc_cfg_lock => rxr_lock
 */

struct iavf_softc {
	device_t		 sc_dev;
	enum i40e_mac_type	 sc_mac_type;
	int			 sc_debuglevel;
	bool			 sc_attached;
	bool			 sc_dead;
	kmutex_t		 sc_cfg_lock;
	callout_t		 sc_tick;
	struct ifmedia		 sc_media;
	uint64_t		 sc_media_status;
	uint64_t		 sc_media_active;
	int			 sc_link_state;

	const struct iavf_aq_regs *
				 sc_aq_regs;

	struct ethercom		 sc_ec;
	uint8_t			 sc_enaddr[ETHER_ADDR_LEN];
	uint8_t			 sc_enaddr_fake[ETHER_ADDR_LEN];
	uint8_t			 sc_enaddr_added[ETHER_ADDR_LEN];
	uint8_t			 sc_enaddr_reset[ETHER_ADDR_LEN];
	struct if_percpuq	*sc_ipq;

	struct pci_attach_args	 sc_pa;
	bus_dma_tag_t		 sc_dmat;
	bus_space_tag_t		 sc_memt;
	bus_space_handle_t	 sc_memh;
	bus_size_t		 sc_mems;
	pci_intr_handle_t	*sc_ihp;
	void			**sc_ihs;
	unsigned int		 sc_nintrs;

	uint32_t		 sc_major_ver;
	uint32_t		 sc_minor_ver;
	uint32_t		 sc_vf_id;
	uint32_t		 sc_vf_cap;
	uint16_t		 sc_vsi_id;
	uint16_t		 sc_qset_handle;
	uint16_t		 sc_max_mtu;
	bool			 sc_got_vf_resources;
	bool			 sc_got_irq_map;
	unsigned int		 sc_max_vectors;

	kmutex_t		 sc_adminq_lock;
	kcondvar_t		 sc_adminq_cv;
	struct ixl_dmamem	 sc_atq;
	unsigned int		 sc_atq_prod;
	unsigned int		 sc_atq_cons;
	struct ixl_aq_bufs	 sc_atq_idle;
	struct ixl_aq_bufs	 sc_atq_live;
	struct ixl_dmamem	 sc_arq;
	struct ixl_aq_bufs	 sc_arq_idle;
	struct ixl_aq_bufs	 sc_arq_live;
	unsigned int		 sc_arq_prod;
	unsigned int		 sc_arq_cons;
	struct iavf_work	 sc_arq_refill;
	uint32_t		 sc_arq_opcode;
	uint32_t		 sc_arq_retval;

	uint32_t		 sc_tx_itr;
	uint32_t		 sc_rx_itr;
	unsigned int		 sc_tx_ring_ndescs;
	unsigned int		 sc_rx_ring_ndescs;
	unsigned int		 sc_nqueue_pairs;
	unsigned int		 sc_nqps_alloc;
	unsigned int		 sc_nqps_vsi;
	unsigned int		 sc_nqps_req;
	struct iavf_queue_pair	*sc_qps;
	bool			 sc_txrx_workqueue;
	u_int			 sc_tx_intr_process_limit;
	u_int			 sc_tx_process_limit;
	u_int			 sc_rx_intr_process_limit;
	u_int			 sc_rx_process_limit;

	struct workqueue	*sc_workq;
	struct workqueue	*sc_workq_txrx;
	struct iavf_work	 sc_reset_task;
	struct iavf_work	 sc_wdto_task;
	struct iavf_work	 sc_req_queues_task;
	bool			 sc_req_queues_retried;
	bool			 sc_resetting;
	bool			 sc_reset_up;

	struct sysctllog	*sc_sysctllog;
	struct iavf_stat_counters
				 sc_stat_counters;
};

#define IAVF_LOG(_sc, _lvl, _fmt, _args...)				\
do {									\
	if (!(_sc)->sc_attached) {					\
		switch (_lvl) {						\
		case LOG_ERR:						\
		case LOG_WARNING:					\
			aprint_error_dev((_sc)->sc_dev, _fmt, ##_args);	\
			break;						\
		case LOG_INFO:						\
			aprint_normal_dev((_sc)->sc_dev,_fmt, ##_args);	\
			break;						\
		case LOG_DEBUG:						\
		default:						\
			aprint_debug_dev((_sc)->sc_dev, _fmt, ##_args);	\
		}							\
	} else {							\
		struct ifnet *_ifp = &(_sc)->sc_ec.ec_if;		\
		log((_lvl), "%s: " _fmt, _ifp->if_xname, ##_args);	\
	}								\
} while (0)

static int	iavf_dmamem_alloc(bus_dma_tag_t, struct ixl_dmamem *,
		    bus_size_t, bus_size_t);
static void	iavf_dmamem_free(bus_dma_tag_t, struct ixl_dmamem *);
static struct ixl_aq_buf *
		iavf_aqb_get(struct iavf_softc *, struct ixl_aq_bufs *);
static struct ixl_aq_buf *
		iavf_aqb_get_locked(struct ixl_aq_bufs *);
static void	iavf_aqb_put_locked(struct ixl_aq_bufs *, struct ixl_aq_buf *);
static void	iavf_aqb_clean(struct ixl_aq_bufs *, bus_dma_tag_t);

static const struct iavf_product *
		iavf_lookup(const struct pci_attach_args *);
static enum i40e_mac_type
		iavf_mactype(pci_product_id_t);
static void	iavf_pci_csr_setup(pci_chipset_tag_t, pcitag_t);
static int	iavf_wait_active(struct iavf_softc *);
static bool	iavf_is_etheranyaddr(const uint8_t *);
static void	iavf_prepare_fakeaddr(struct iavf_softc *);
static int	iavf_replace_lla(struct ifnet *,
		    const uint8_t *, const uint8_t *);
static void	iavf_evcnt_attach(struct evcnt *,
		    const char *, const char *);
static int	iavf_setup_interrupts(struct iavf_softc *);
static void	iavf_teardown_interrupts(struct iavf_softc *);
static int	iavf_setup_sysctls(struct iavf_softc *);
static void	iavf_teardown_sysctls(struct iavf_softc *);
static int	iavf_setup_stats(struct iavf_softc *);
static void	iavf_teardown_stats(struct iavf_softc *);
static struct workqueue *
		iavf_workq_create(const char *, pri_t, int, int);
static void	iavf_workq_destroy(struct workqueue *);
static int	iavf_work_set(struct iavf_work *, void (*)(void *), void *);
static void	iavf_work_add(struct workqueue *, struct iavf_work *);
static void	iavf_work_wait(struct workqueue *, struct iavf_work *);
static unsigned int
		iavf_calc_msix_count(struct iavf_softc *);
static unsigned int
		iavf_calc_queue_pair_size(struct iavf_softc *);
static int	iavf_queue_pairs_alloc(struct iavf_softc *);
static void	iavf_queue_pairs_free(struct iavf_softc *);
static int	iavf_arq_fill(struct iavf_softc *);
static void	iavf_arq_refill(void *);
static int	iavf_arq_poll(struct iavf_softc *, uint32_t, int);
static void	iavf_atq_done(struct iavf_softc *);
static int	iavf_init_admin_queue(struct iavf_softc *);
static void	iavf_cleanup_admin_queue(struct iavf_softc *);
static int	iavf_arq(struct iavf_softc *);
static int	iavf_adminq_exec(struct iavf_softc *,
		    struct ixl_aq_desc *, struct ixl_aq_buf *);
static int	iavf_adminq_poll(struct iavf_softc *,
		    struct ixl_aq_desc *, struct ixl_aq_buf *, int);
static int	iavf_adminq_poll_locked(struct iavf_softc *,
		    struct ixl_aq_desc *, struct ixl_aq_buf *, int);
static int	iavf_add_multi(struct iavf_softc *, uint8_t *, uint8_t *);
static int	iavf_del_multi(struct iavf_softc *, uint8_t *, uint8_t *);
static void	iavf_del_all_multi(struct iavf_softc *);

static int	iavf_get_version(struct iavf_softc *, struct ixl_aq_buf *);
static int	iavf_get_vf_resources(struct iavf_softc *, struct ixl_aq_buf *);
static int	iavf_get_stats(struct iavf_softc *);
static int	iavf_config_irq_map(struct iavf_softc *, struct ixl_aq_buf *);
static int	iavf_config_vsi_queues(struct iavf_softc *);
static int	iavf_config_hena(struct iavf_softc *);
static int	iavf_config_rss_key(struct iavf_softc *);
static int	iavf_config_rss_lut(struct iavf_softc *);
static int	iavf_config_promisc_mode(struct iavf_softc *, int, int);
static int	iavf_config_vlan_stripping(struct iavf_softc *, int);
static int	iavf_config_vlan_id(struct iavf_softc *, uint16_t, uint32_t);
static int	iavf_queue_select(struct iavf_softc *, int);
static int	iavf_request_queues(struct iavf_softc *, unsigned int);
static int	iavf_reset_vf(struct iavf_softc *);
static int	iavf_eth_addr(struct iavf_softc *, const uint8_t *, uint32_t);
static void	iavf_process_version(struct iavf_softc *,
		    struct ixl_aq_desc *, struct ixl_aq_buf *);
static void	iavf_process_vf_resources(struct iavf_softc *,
		    struct ixl_aq_desc *, struct ixl_aq_buf *);
static void	iavf_process_irq_map(struct iavf_softc *,
		    struct ixl_aq_desc *);
static void	iavf_process_vc_event(struct iavf_softc *,
		    struct ixl_aq_desc *, struct ixl_aq_buf *);
static void	iavf_process_stats(struct iavf_softc *,
		    struct ixl_aq_desc *, struct ixl_aq_buf *);
static void	iavf_process_req_queues(struct iavf_softc *,
		    struct ixl_aq_desc *, struct ixl_aq_buf *);

static int	iavf_intr(void *);
static int	iavf_queue_intr(void *);
static void	iavf_tick(void *);
static void	iavf_tick_halt(void *);
static void	iavf_reset_request(void *);
static void	iavf_reset_start(void *);
static void	iavf_reset(void *);
static void	iavf_reset_finish(struct iavf_softc *);
static int	iavf_init(struct ifnet *);
static int	iavf_init_locked(struct iavf_softc *);
static void	iavf_stop(struct ifnet *, int);
static void	iavf_stop_locked(struct iavf_softc *);
static int	iavf_ioctl(struct ifnet *, u_long, void *);
static void	iavf_start(struct ifnet *);
static int	iavf_transmit(struct ifnet *, struct mbuf*);
static int	iavf_watchdog(struct iavf_tx_ring *);
static void	iavf_watchdog_timeout(void *);
static int	iavf_media_change(struct ifnet *);
static void	iavf_media_status(struct ifnet *, struct ifmediareq *);
static int	iavf_ifflags_cb(struct ethercom *);
static int	iavf_vlan_cb(struct ethercom *, uint16_t, bool);
static void	iavf_deferred_transmit(void *);
static void	iavf_handle_queue(void *);
static void	iavf_handle_queue_wk(struct work *, void *);
static int	iavf_reinit(struct iavf_softc *);
static int	iavf_rxfill(struct iavf_softc *, struct iavf_rx_ring *);
static void	iavf_txr_clean(struct iavf_softc *, struct iavf_tx_ring *);
static void	iavf_rxr_clean(struct iavf_softc *, struct iavf_rx_ring *);
static int	iavf_txeof(struct iavf_softc *, struct iavf_tx_ring *,
		    u_int, struct evcnt *);
static int	iavf_rxeof(struct iavf_softc *, struct iavf_rx_ring *,
		    u_int, struct evcnt *);
static int	iavf_iff(struct iavf_softc *);
static int	iavf_iff_locked(struct iavf_softc *);
static void	iavf_post_request_queues(void *);
static int	iavf_sysctl_itr_handler(SYSCTLFN_PROTO);

static int	iavf_match(device_t, cfdata_t, void *);
static void	iavf_attach(device_t, device_t, void*);
static int	iavf_detach(device_t, int);
static int	iavf_finalize_teardown(device_t);

CFATTACH_DECL3_NEW(iavf, sizeof(struct iavf_softc),
    iavf_match, iavf_attach, iavf_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

static const struct iavf_product iavf_products[] = {
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XL710_VF },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_XL710_VF_HV },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_X722_VF },
	/* required last entry */
	{0, 0}
};

static const struct iavf_link_speed iavf_link_speeds[] = {
	{ 0, 0 },
	{ IF_Mbps(100), IFM_100_TX },
	{ IF_Mbps(1000), IFM_1000_T },
	{ IF_Gbps(10), IFM_10G_T },
	{ IF_Gbps(40), IFM_40G_CR4 },
	{ IF_Gbps(20), IFM_20G_KR2 },
	{ IF_Gbps(25), IFM_25G_CR }
};

static const struct iavf_aq_regs iavf_aq_regs = {
	.atq_tail	= I40E_VF_ATQT1,
	.atq_tail_mask	= I40E_VF_ATQT1_ATQT_MASK,
	.atq_head	= I40E_VF_ATQH1,
	.atq_head_mask	= I40E_VF_ARQH1_ARQH_MASK,
	.atq_len	= I40E_VF_ATQLEN1,
	.atq_bal	= I40E_VF_ATQBAL1,
	.atq_bah	= I40E_VF_ATQBAH1,
	.atq_len_enable	= I40E_VF_ATQLEN1_ATQENABLE_MASK,

	.arq_tail	= I40E_VF_ARQT1,
	.arq_tail_mask	= I40E_VF_ARQT1_ARQT_MASK,
	.arq_head	= I40E_VF_ARQH1,
	.arq_head_mask	= I40E_VF_ARQH1_ARQH_MASK,
	.arq_len	= I40E_VF_ARQLEN1,
	.arq_bal	= I40E_VF_ARQBAL1,
	.arq_bah	= I40E_VF_ARQBAH1,
	.arq_len_enable	= I40E_VF_ARQLEN1_ARQENABLE_MASK,
};

static struct iavf_module_params iavf_params = {
	.debug = 0,
	.rx_itr = 0x07a, /* 4K intrs/sec */
	.tx_itr = 0x07a, /* 4K intrs/sec */
	.tx_ndescs = 512,
	.rx_ndescs = 256,
	.max_qps = INT_MAX,
};

#define delaymsec(_x)	DELAY(1000 * (_x))
#define iavf_rd(_s, _r)			\
	bus_space_read_4((_s)->sc_memt, (_s)->sc_memh, (_r))
#define iavf_wr(_s, _r, _v)		\
	bus_space_write_4((_s)->sc_memt, (_s)->sc_memh, (_r), (_v))
#define iavf_barrier(_s, _r, _l, _o) \
	bus_space_barrier((_s)->sc_memt, (_s)->sc_memh, (_r), (_l), (_o))
#define iavf_flush(_s)	(void)iavf_rd((_s), I40E_VFGEN_RSTAT)
#define iavf_nqueues(_sc)	(1 << ((_sc)->sc_nqueue_pairs - 1))
#define iavf_allqueues(_sc)	((1 << ((_sc)->sc_nqueue_pairs)) - 1)

static inline void
iavf_intr_barrier(void)
{

	/* make all interrupt handler finished */
	xc_barrier(0);
}
static inline void
iavf_intr_enable(struct iavf_softc *sc)
{

	iavf_wr(sc, I40E_VFINT_DYN_CTL01, I40E_VFINT_DYN_CTL0_INTENA_MASK |
	    I40E_VFINT_DYN_CTL0_CLEARPBA_MASK |
	    (IAVF_NOITR << I40E_VFINT_DYN_CTL0_ITR_INDX_SHIFT));
	iavf_wr(sc, I40E_VFINT_ICR0_ENA1, I40E_VFINT_ICR0_ENA1_ADMINQ_MASK);
	iavf_flush(sc);
}

static inline void
iavf_intr_disable(struct iavf_softc *sc)
{

	iavf_wr(sc, I40E_VFINT_DYN_CTL01,
	    (IAVF_NOITR << I40E_VFINT_DYN_CTL0_ITR_INDX_SHIFT));
	iavf_wr(sc, I40E_VFINT_ICR0_ENA1, 0);
	iavf_flush(sc);
}

static inline void
iavf_queue_intr_enable(struct iavf_softc *sc, unsigned int qid)
{

	iavf_wr(sc, I40E_VFINT_DYN_CTLN1(qid),
	    I40E_VFINT_DYN_CTLN1_INTENA_MASK |
	    I40E_VFINT_DYN_CTLN1_CLEARPBA_MASK |
	    (IAVF_NOITR << I40E_VFINT_DYN_CTLN1_ITR_INDX_SHIFT));
	iavf_flush(sc);
}

static inline void
iavf_queue_intr_disable(struct iavf_softc *sc, unsigned int qid)
{

	iavf_wr(sc, I40E_VFINT_DYN_CTLN1(qid),
	    (IAVF_NOITR << I40E_VFINT_DYN_CTLN1_ITR_INDX_SHIFT));
	iavf_flush(sc);
}

static inline void
iavf_aq_vc_set_opcode(struct ixl_aq_desc *iaq, uint32_t opcode)
{
	struct iavf_aq_vc *vc;

	vc = (struct iavf_aq_vc *)&iaq->iaq_cookie;
	vc->iaq_vc_opcode = htole32(opcode);
}

static inline uint32_t
iavf_aq_vc_get_opcode(const struct ixl_aq_desc *iaq)
{
	const struct iavf_aq_vc *vc;

	vc = (const struct iavf_aq_vc *)&iaq->iaq_cookie;
	return le32toh(vc->iaq_vc_opcode);
}

static inline uint32_t
iavf_aq_vc_get_retval(const struct ixl_aq_desc *iaq)
{
	const struct iavf_aq_vc *vc;

	vc = (const struct iavf_aq_vc *)&iaq->iaq_cookie;
	return le32toh(vc->iaq_vc_retval);
}

static int
iavf_match(device_t parent, cfdata_t match, void *aux)
{
	const struct pci_attach_args *pa = aux;

	return (iavf_lookup(pa) != NULL) ? 1 : 0;
}

static void
iavf_attach(device_t parent, device_t self, void *aux)
{
	struct iavf_softc *sc;
	struct pci_attach_args *pa = aux;
	struct ifnet *ifp;
	struct ixl_aq_buf *aqb;
	pcireg_t memtype;
	char xnamebuf[MAXCOMLEN];
	int error, i;

	sc = device_private(self);
	sc->sc_dev = self;
	ifp = &sc->sc_ec.ec_if;

	sc->sc_pa = *pa;
	sc->sc_dmat = (pci_dma64_available(pa)) ? pa->pa_dmat64 : pa->pa_dmat;
	sc->sc_aq_regs = &iavf_aq_regs;
	sc->sc_debuglevel = iavf_params.debug;
	sc->sc_tx_ring_ndescs = iavf_params.tx_ndescs;
	sc->sc_rx_ring_ndescs = iavf_params.rx_ndescs;
	sc->sc_tx_itr = iavf_params.tx_itr;
	sc->sc_rx_itr = iavf_params.rx_itr;
	sc->sc_nqps_req = MIN(ncpu, iavf_params.max_qps);
	iavf_prepare_fakeaddr(sc);

	sc->sc_mac_type = iavf_mactype(PCI_PRODUCT(pa->pa_id));
	iavf_pci_csr_setup(pa->pa_pc, pa->pa_tag);

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, IAVF_PCIREG);
	if (pci_mapreg_map(pa, IAVF_PCIREG, memtype, 0,
	    &sc->sc_memt, &sc->sc_memh, NULL, &sc->sc_mems)) {
		aprint_error(": unable to map registers\n");
		return;
	}

	if (iavf_wait_active(sc) != 0) {
		aprint_error(": VF reset timed out\n");
		goto unmap;
	}

	mutex_init(&sc->sc_cfg_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	mutex_init(&sc->sc_adminq_lock, MUTEX_DEFAULT, IPL_NET);
	SIMPLEQ_INIT(&sc->sc_atq_idle);
	SIMPLEQ_INIT(&sc->sc_atq_live);
	SIMPLEQ_INIT(&sc->sc_arq_idle);
	SIMPLEQ_INIT(&sc->sc_arq_live);
	sc->sc_arq_cons = 0;
	sc->sc_arq_prod = 0;
	aqb = NULL;

	if (iavf_dmamem_alloc(sc->sc_dmat, &sc->sc_atq,
	    sizeof(struct ixl_aq_desc) * IAVF_AQ_NUM, IAVF_AQ_ALIGN) != 0) {
		aprint_error(": unable to allocate atq\n");
		goto free_mutex;
	}

	if (iavf_dmamem_alloc(sc->sc_dmat, &sc->sc_arq,
	    sizeof(struct ixl_aq_desc) * IAVF_AQ_NUM, IAVF_AQ_ALIGN) != 0) {
		aprint_error(": unable to allocate arq\n");
		goto free_atq;
	}

	for (i = 0; i < IAVF_AQ_NUM; i++) {
		aqb = iavf_aqb_get(sc, NULL);
		if (aqb != NULL) {
			iavf_aqb_put_locked(&sc->sc_arq_idle, aqb);
		}
	}
	aqb = NULL;

	if (!iavf_arq_fill(sc)) {
		aprint_error(": unable to fill arq descriptors\n");
		goto free_arq;
	}

	if (iavf_init_admin_queue(sc) != 0) {
		aprint_error(": unable to initialize admin queue\n");
		goto shutdown;
	}

	aqb = iavf_aqb_get(sc, NULL);
	if (aqb == NULL) {
		aprint_error(": unable to allocate buffer for ATQ\n");
		goto shutdown;
	}

	error = iavf_get_version(sc, aqb);
	switch (error) {
	case 0:
		break;
	case ETIMEDOUT:
		aprint_error(": timeout waiting for VF version\n");
		goto shutdown;
	case ENOTSUP:
		aprint_error(": unsupported VF version %d\n", sc->sc_major_ver);
		goto shutdown;
	default:
		aprint_error(":unable to get VF interface version\n");
		goto shutdown;
	}

	if (iavf_get_vf_resources(sc, aqb) != 0) {
		aprint_error(": timeout waiting for VF resources\n");
		goto shutdown;
	}

	aprint_normal(", VF version %d.%d%s",
	    sc->sc_major_ver, sc->sc_minor_ver,
	    (sc->sc_minor_ver > IAVF_VF_MINOR) ? "(minor mismatch)" : "");
	aprint_normal(", VF %d, VSI %d", sc->sc_vf_id, sc->sc_vsi_id);
	aprint_normal("\n");
	aprint_naive("\n");

	aprint_normal_dev(self, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_enaddr));

	if (iavf_queue_pairs_alloc(sc) != 0) {
		goto shutdown;
	}

	if (iavf_setup_interrupts(sc) != 0) {
		goto free_queue_pairs;
	}

	if (iavf_config_irq_map(sc, aqb) != 0) {
		aprint_error(", timed out waiting for IRQ map response\n");
		goto teardown_intrs;
	}

	if (iavf_setup_sysctls(sc) != 0) {
		goto teardown_intrs;
	}

	if (iavf_setup_stats(sc) != 0) {
		goto teardown_sysctls;
	}

	iavf_aqb_put_locked(&sc->sc_atq_idle, aqb);
	aqb = NULL;

	snprintf(xnamebuf, sizeof(xnamebuf),
	    "%s_adminq_cv", device_xname(self));
	cv_init(&sc->sc_adminq_cv, xnamebuf);

	callout_init(&sc->sc_tick, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_tick, iavf_tick, sc);

	iavf_work_set(&sc->sc_reset_task, iavf_reset_start, sc);
	iavf_work_set(&sc->sc_arq_refill, iavf_arq_refill, sc);
	iavf_work_set(&sc->sc_wdto_task, iavf_watchdog_timeout, sc);
	iavf_work_set(&sc->sc_req_queues_task, iavf_post_request_queues, sc);
	snprintf(xnamebuf, sizeof(xnamebuf), "%s_wq_cfg", device_xname(self));
	sc->sc_workq = iavf_workq_create(xnamebuf, IAVF_WORKQUEUE_PRI,
	    IPL_NET, WQ_MPSAFE);
	if (sc->sc_workq == NULL)
		goto destroy_cv;

	snprintf(xnamebuf, sizeof(xnamebuf), "%s_wq_txrx", device_xname(self));
	error = workqueue_create(&sc->sc_workq_txrx, xnamebuf,
	    iavf_handle_queue_wk, sc, IAVF_WORKQUEUE_PRI, IPL_NET,
	    WQ_PERCPU|WQ_MPSAFE);
	if (error != 0) {
		sc->sc_workq_txrx = NULL;
		goto teardown_wqs;
	}

	if_initialize(ifp);

	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_extflags = IFEF_MPSAFE;
	ifp->if_ioctl = iavf_ioctl;
	ifp->if_start = iavf_start;
	ifp->if_transmit = iavf_transmit;
	ifp->if_watchdog = NULL;
	ifp->if_init = iavf_init;
	ifp->if_stop = iavf_stop;

	IFQ_SET_MAXLEN(&ifp->if_snd, sc->sc_tx_ring_ndescs);
	IFQ_SET_READY(&ifp->if_snd);
	sc->sc_ipq = if_percpuq_create(ifp);

	ifp->if_capabilities |= IAVF_IFCAP_RXCSUM;
	ifp->if_capabilities |= IAVF_IFCAP_TXCSUM;

	ether_set_vlan_cb(&sc->sc_ec, iavf_vlan_cb);
	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_HWTAGGING;
	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_HWFILTER;
	sc->sc_ec.ec_capenable = sc->sc_ec.ec_capabilities;

	ether_set_ifflags_cb(&sc->sc_ec, iavf_ifflags_cb);

	sc->sc_ec.ec_ifmedia = &sc->sc_media;
	ifmedia_init_with_lock(&sc->sc_media, IFM_IMASK, iavf_media_change,
	    iavf_media_status, &sc->sc_cfg_lock);

	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_AUTO);

	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, sc->sc_enaddr);

	sc->sc_txrx_workqueue = true;
	sc->sc_tx_process_limit = IAVF_TX_PROCESS_LIMIT;
	sc->sc_rx_process_limit = IAVF_RX_PROCESS_LIMIT;
	sc->sc_tx_intr_process_limit = IAVF_TX_INTR_PROCESS_LIMIT;
	sc->sc_rx_intr_process_limit = IAVF_RX_INTR_PROCESS_LIMIT;

	if_register(ifp);
	if_link_state_change(ifp, sc->sc_link_state);
	iavf_intr_enable(sc);
	if (sc->sc_nqps_vsi < sc->sc_nqps_req)
		iavf_work_add(sc->sc_workq, &sc->sc_req_queues_task);
	sc->sc_attached = true;
	return;

teardown_wqs:
	config_finalize_register(self, iavf_finalize_teardown);
destroy_cv:
	cv_destroy(&sc->sc_adminq_cv);
	callout_destroy(&sc->sc_tick);
	iavf_teardown_stats(sc);
teardown_sysctls:
	iavf_teardown_sysctls(sc);
teardown_intrs:
	iavf_teardown_interrupts(sc);
free_queue_pairs:
	iavf_queue_pairs_free(sc);
shutdown:
	if (aqb != NULL)
		iavf_aqb_put_locked(&sc->sc_atq_idle, aqb);
	iavf_cleanup_admin_queue(sc);
	iavf_aqb_clean(&sc->sc_atq_idle, sc->sc_dmat);
	iavf_aqb_clean(&sc->sc_arq_idle, sc->sc_dmat);
free_arq:
	iavf_dmamem_free(sc->sc_dmat, &sc->sc_arq);
free_atq:
	iavf_dmamem_free(sc->sc_dmat, &sc->sc_atq);
free_mutex:
	mutex_destroy(&sc->sc_cfg_lock);
	mutex_destroy(&sc->sc_adminq_lock);
unmap:
	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);
	sc->sc_mems = 0;
	sc->sc_attached = false;
}

static int
iavf_detach(device_t self, int flags)
{
	struct iavf_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ec.ec_if;

	if (!sc->sc_attached)
		return 0;

	iavf_stop(ifp, 1);

	/*
	 * set a dummy function to halt callout safely
	 * even if a workqueue entry calls callout_schedule()
	 */
	callout_setfunc(&sc->sc_tick, iavf_tick_halt, sc);
	iavf_work_wait(sc->sc_workq, &sc->sc_reset_task);
	iavf_work_wait(sc->sc_workq, &sc->sc_wdto_task);

	callout_halt(&sc->sc_tick, NULL);
	callout_destroy(&sc->sc_tick);

	/* detach the I/F before stop adminq due to callbacks */
	ether_ifdetach(ifp);
	if_detach(ifp);
	ifmedia_fini(&sc->sc_media);
	if_percpuq_destroy(sc->sc_ipq);

	iavf_intr_disable(sc);
	iavf_intr_barrier();
	iavf_work_wait(sc->sc_workq, &sc->sc_arq_refill);

	mutex_enter(&sc->sc_adminq_lock);
	iavf_cleanup_admin_queue(sc);
	mutex_exit(&sc->sc_adminq_lock);
	iavf_aqb_clean(&sc->sc_atq_idle, sc->sc_dmat);
	iavf_aqb_clean(&sc->sc_arq_idle, sc->sc_dmat);
	iavf_dmamem_free(sc->sc_dmat, &sc->sc_arq);
	iavf_dmamem_free(sc->sc_dmat, &sc->sc_atq);
	cv_destroy(&sc->sc_adminq_cv);

	iavf_workq_destroy(sc->sc_workq);
	sc->sc_workq = NULL;

	iavf_queue_pairs_free(sc);
	iavf_teardown_interrupts(sc);
	iavf_teardown_sysctls(sc);
	iavf_teardown_stats(sc);
	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);

	mutex_destroy(&sc->sc_adminq_lock);
	mutex_destroy(&sc->sc_cfg_lock);

	return 0;
}

static int
iavf_finalize_teardown(device_t self)
{
	struct iavf_softc *sc = device_private(self);

	if (sc->sc_workq != NULL) {
		iavf_workq_destroy(sc->sc_workq);
		sc->sc_workq = NULL;
	}

	if (sc->sc_workq_txrx != NULL) {
		workqueue_destroy(sc->sc_workq_txrx);
		sc->sc_workq_txrx = NULL;
	}

	return 0;
}

static int
iavf_init(struct ifnet *ifp)
{
	struct iavf_softc *sc;
	int rv;

	sc = ifp->if_softc;
	mutex_enter(&sc->sc_cfg_lock);
	rv = iavf_init_locked(sc);
	mutex_exit(&sc->sc_cfg_lock);

	return rv;
}

static int
iavf_init_locked(struct iavf_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	unsigned int i;
	int error;

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	if (ISSET(ifp->if_flags, IFF_RUNNING))
		iavf_stop_locked(sc);

	if (sc->sc_resetting)
		return ENXIO;

	error = iavf_reinit(sc);
	if (error) {
		iavf_stop_locked(sc);
		return error;
	}

	SET(ifp->if_flags, IFF_RUNNING);
	CLR(ifp->if_flags, IFF_OACTIVE);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		iavf_wr(sc, I40E_VFINT_ITRN1(IAVF_ITR_RX, i), sc->sc_rx_itr);
		iavf_wr(sc, I40E_VFINT_ITRN1(IAVF_ITR_TX, i), sc->sc_tx_itr);
	}
	iavf_wr(sc, I40E_VFINT_ITR01(IAVF_ITR_RX), sc->sc_rx_itr);
	iavf_wr(sc, I40E_VFINT_ITR01(IAVF_ITR_TX), sc->sc_tx_itr);
	iavf_wr(sc, I40E_VFINT_ITR01(IAVF_ITR_MISC), 0);

	error = iavf_iff_locked(sc);
	if (error) {
		iavf_stop_locked(sc);
		return error;
	};

	/* ETHERCAP_VLAN_HWFILTER can not be disabled */
	SET(sc->sc_ec.ec_capenable, ETHERCAP_VLAN_HWFILTER);

	callout_schedule(&sc->sc_tick, IAVF_TICK_INTERVAL);
	return 0;
}

static int
iavf_reinit(struct iavf_softc *sc)
{
	struct iavf_rx_ring *rxr;
	struct iavf_tx_ring *txr;
	unsigned int i;
	uint32_t reg;

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	sc->sc_reset_up = true;
	sc->sc_nqueue_pairs = MIN(sc->sc_nqps_alloc, sc->sc_nintrs - 1);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		rxr = sc->sc_qps[i].qp_rxr;
		txr = sc->sc_qps[i].qp_txr;

		iavf_rxfill(sc, rxr);
		txr->txr_watchdog = IAVF_WATCHDOG_STOP;
	}

	if (iavf_config_vsi_queues(sc) != 0)
		return EIO;

	if (iavf_config_hena(sc) != 0)
		return EIO;

	iavf_config_rss_key(sc);
	iavf_config_rss_lut(sc);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		iavf_queue_intr_enable(sc, i);
	}
	/* unmask */
	reg = iavf_rd(sc, I40E_VFINT_DYN_CTL01);
	reg |= (IAVF_NOITR << I40E_VFINT_DYN_CTL0_ITR_INDX_SHIFT);
	iavf_wr(sc, I40E_VFINT_DYN_CTL01, reg);

	if (iavf_queue_select(sc, IAVF_VC_OP_ENABLE_QUEUES) != 0)
		return EIO;

	return 0;
}

static void
iavf_stop(struct ifnet *ifp, int disable)
{
	struct iavf_softc *sc;

	sc = ifp->if_softc;
	mutex_enter(&sc->sc_cfg_lock);
	iavf_stop_locked(sc);
	mutex_exit(&sc->sc_cfg_lock);
}

static void
iavf_stop_locked(struct iavf_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct iavf_rx_ring *rxr;
	struct iavf_tx_ring *txr;
	uint32_t reg;
	unsigned int i;

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	CLR(ifp->if_flags, IFF_RUNNING);
	sc->sc_reset_up = false;
	callout_stop(&sc->sc_tick);

	if (!sc->sc_resetting) {
		/* disable queues*/
		if (iavf_queue_select(sc, IAVF_VC_OP_DISABLE_QUEUES) != 0) {
			goto die;
		}
	}

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		iavf_queue_intr_disable(sc, i);
	}

	/* mask interrupts */
	reg = iavf_rd(sc, I40E_VFINT_DYN_CTL01);
	reg |= I40E_VFINT_DYN_CTL0_INTENA_MSK_MASK |
	    (IAVF_NOITR << I40E_VFINT_DYN_CTL0_ITR_INDX_SHIFT);
	iavf_wr(sc, I40E_VFINT_DYN_CTL01, reg);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		rxr = sc->sc_qps[i].qp_rxr;
		txr = sc->sc_qps[i].qp_txr;

		mutex_enter(&rxr->rxr_lock);
		iavf_rxr_clean(sc, rxr);
		mutex_exit(&rxr->rxr_lock);

		mutex_enter(&txr->txr_lock);
		iavf_txr_clean(sc, txr);
		mutex_exit(&txr->txr_lock);

		workqueue_wait(sc->sc_workq_txrx,
		    &sc->sc_qps[i].qp_work);
	}

	return;
die:
	if (!sc->sc_dead) {
		sc->sc_dead = true;
		log(LOG_INFO, "%s: Request VF reset\n", ifp->if_xname);

		iavf_work_set(&sc->sc_reset_task, iavf_reset_request, sc);
		iavf_work_add(sc->sc_workq, &sc->sc_reset_task);
	}
	log(LOG_CRIT, "%s: failed to shut down rings\n", ifp->if_xname);
}

static int
iavf_watchdog(struct iavf_tx_ring *txr)
{
	struct iavf_softc *sc;

	sc = txr->txr_sc;

	mutex_enter(&txr->txr_lock);

	if (txr->txr_watchdog == IAVF_WATCHDOG_STOP
	    || --txr->txr_watchdog > 0) {
		mutex_exit(&txr->txr_lock);
		return 0;
	}

	txr->txr_watchdog = IAVF_WATCHDOG_STOP;
	txr->txr_watchdogto.ev_count++;
	mutex_exit(&txr->txr_lock);

	device_printf(sc->sc_dev, "watchdog timeout on queue %d\n",
	    txr->txr_qid);
	return 1;
}

static void
iavf_watchdog_timeout(void *xsc)
{
	struct iavf_softc *sc;
	struct ifnet *ifp;

	sc = xsc;
	ifp = &sc->sc_ec.ec_if;

	mutex_enter(&sc->sc_cfg_lock);
	if (ISSET(ifp->if_flags, IFF_RUNNING))
		iavf_init_locked(sc);
	mutex_exit(&sc->sc_cfg_lock);
}

static int
iavf_media_change(struct ifnet *ifp)
{
	struct iavf_softc *sc;
	struct ifmedia *ifm;

	sc = ifp->if_softc;
	ifm = &sc->sc_media;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return EINVAL;

	switch (IFM_SUBTYPE(ifm->ifm_media)) {
	case IFM_AUTO:
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static void
iavf_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct iavf_softc *sc = ifp->if_softc;

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	ifmr->ifm_status = sc->sc_media_status;
	ifmr->ifm_active = sc->sc_media_active;
}

static int
iavf_ifflags_cb(struct ethercom *ec)
{
	struct ifnet *ifp = &ec->ec_if;
	struct iavf_softc *sc = ifp->if_softc;

	/* vlan hwfilter can not be disabled */
	SET(ec->ec_capenable, ETHERCAP_VLAN_HWFILTER);

	return iavf_iff(sc);
}

static int
iavf_vlan_cb(struct ethercom *ec, uint16_t vid, bool set)
{
	struct ifnet *ifp = &ec->ec_if;
	struct iavf_softc *sc = ifp->if_softc;
	int rv;

	mutex_enter(&sc->sc_cfg_lock);

	if (sc->sc_resetting) {
		mutex_exit(&sc->sc_cfg_lock);

		/* all vlan id was already removed */
		if (!set)
			return 0;

		return ENXIO;
	}

	/* ETHERCAP_VLAN_HWFILTER can not be disabled */
	SET(sc->sc_ec.ec_capenable, ETHERCAP_VLAN_HWFILTER);

	if (set) {
		rv = iavf_config_vlan_id(sc, vid, IAVF_VC_OP_ADD_VLAN);
		if (!ISSET(sc->sc_ec.ec_capenable, ETHERCAP_VLAN_HWTAGGING)) {
			iavf_config_vlan_stripping(sc,
			    sc->sc_ec.ec_capenable);
		}
	} else {
		rv = iavf_config_vlan_id(sc, vid, IAVF_VC_OP_DEL_VLAN);
	}

	mutex_exit(&sc->sc_cfg_lock);

	if (rv != 0)
		return EIO;

	return 0;
}

static int
iavf_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ifreq *ifr = (struct ifreq *)data;
	struct iavf_softc *sc = (struct iavf_softc *)ifp->if_softc;
	const struct sockaddr *sa;
	uint8_t addrhi[ETHER_ADDR_LEN], addrlo[ETHER_ADDR_LEN];
	int s, error = 0;
	unsigned int nmtu;

	switch (cmd) {
	case SIOCSIFMTU:
		nmtu = ifr->ifr_mtu;

		if (nmtu < IAVF_MIN_MTU || nmtu > IAVF_MAX_MTU) {
			error = EINVAL;
			break;
		}
		if (ifp->if_mtu != nmtu) {
			s = splnet();
			error = ether_ioctl(ifp, cmd, data);
			splx(s);
			if (error == ENETRESET)
				error = iavf_init(ifp);
		}
		break;
	case SIOCADDMULTI:
		sa = ifreq_getaddr(SIOCADDMULTI, ifr);
		if (ether_addmulti(sa, &sc->sc_ec) == ENETRESET) {
			error = ether_multiaddr(sa, addrlo, addrhi);
			if (error != 0)
				return error;

			error = iavf_add_multi(sc, addrlo, addrhi);
			if (error != 0 && error != ENETRESET) {
				ether_delmulti(sa, &sc->sc_ec);
				error = EIO;
			}
		}
		break;

	case SIOCDELMULTI:
		sa = ifreq_getaddr(SIOCDELMULTI, ifr);
		if (ether_delmulti(sa, &sc->sc_ec) == ENETRESET) {
			error = ether_multiaddr(sa, addrlo, addrhi);
			if (error != 0)
				return error;

			error = iavf_del_multi(sc, addrlo, addrhi);
		}
		break;

	default:
		s = splnet();
		error = ether_ioctl(ifp, cmd, data);
		splx(s);
	}

	if (error == ENETRESET)
		error = iavf_iff(sc);

	return error;
}

static int
iavf_iff(struct iavf_softc *sc)
{
	int error;

	mutex_enter(&sc->sc_cfg_lock);
	error = iavf_iff_locked(sc);
	mutex_exit(&sc->sc_cfg_lock);

	return error;
}

static int
iavf_iff_locked(struct iavf_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int unicast, multicast;
	const uint8_t *enaddr;

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	if (!ISSET(ifp->if_flags, IFF_RUNNING))
		return 0;

	unicast = 0;
	multicast = 0;
	if (ISSET(ifp->if_flags, IFF_PROMISC)) {
		unicast = 1;
		multicast = 1;
	} else if (ISSET(ifp->if_flags, IFF_ALLMULTI)) {
		multicast = 1;
	}

	iavf_config_promisc_mode(sc, unicast, multicast);

	iavf_config_vlan_stripping(sc, sc->sc_ec.ec_capenable);

	enaddr = CLLADDR(ifp->if_sadl);
	if (memcmp(enaddr, sc->sc_enaddr_added, ETHER_ADDR_LEN) != 0) {
		if (!iavf_is_etheranyaddr(sc->sc_enaddr_added)) {
			iavf_eth_addr(sc, sc->sc_enaddr_added,
			    IAVF_VC_OP_DEL_ETH_ADDR);
		}
		memcpy(sc->sc_enaddr_added, enaddr, ETHER_ADDR_LEN);
		iavf_eth_addr(sc, enaddr, IAVF_VC_OP_ADD_ETH_ADDR);
	}

	return 0;
}

static const struct iavf_product *
iavf_lookup(const struct pci_attach_args *pa)
{
	const struct iavf_product *iavfp;

	for (iavfp = iavf_products; iavfp->vendor_id != 0; iavfp++) {
		if (PCI_VENDOR(pa->pa_id) == iavfp->vendor_id &&
		    PCI_PRODUCT(pa->pa_id) == iavfp->product_id)
			return iavfp;
	}

	return NULL;
}

static enum i40e_mac_type
iavf_mactype(pci_product_id_t id)
{

	switch (id) {
	case PCI_PRODUCT_INTEL_XL710_VF:
	case PCI_PRODUCT_INTEL_XL710_VF_HV:
		return I40E_MAC_VF;
	case PCI_PRODUCT_INTEL_X722_VF:
		return I40E_MAC_X722_VF;
	}

	return I40E_MAC_GENERIC;
}

static const struct iavf_link_speed *
iavf_find_link_speed(struct iavf_softc *sc, uint32_t link_speed)
{
	size_t i;

	for (i = 0; i < __arraycount(iavf_link_speeds); i++) {
		if (link_speed & (1 << i))
			return (&iavf_link_speeds[i]);
	}

	return NULL;
}

static void
iavf_pci_csr_setup(pci_chipset_tag_t pc, pcitag_t tag)
{
	pcireg_t csr;

	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
	csr |= (PCI_COMMAND_MASTER_ENABLE |
	    PCI_COMMAND_MEM_ENABLE);
	pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);
}

static int
iavf_wait_active(struct iavf_softc *sc)
{
	int tries;
	uint32_t reg;

	for (tries = 0; tries < 100; tries++) {
		reg = iavf_rd(sc, I40E_VFGEN_RSTAT) &
		    I40E_VFGEN_RSTAT_VFR_STATE_MASK;
		if (reg == IAVF_VFR_VFACTIVE ||
		    reg == IAVF_VFR_COMPLETED)
			return 0;

		delaymsec(10);
	}

	return -1;
}

static bool
iavf_is_etheranyaddr(const uint8_t *enaddr)
{
	static const uint8_t etheranyaddr[ETHER_ADDR_LEN] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

	if (memcmp(enaddr, etheranyaddr, ETHER_ADDR_LEN) != 0)
		return false;

	return true;
}

static void
iavf_prepare_fakeaddr(struct iavf_softc *sc)
{
	uint64_t rndval;

	if (!iavf_is_etheranyaddr(sc->sc_enaddr_fake))
		return;

	rndval = cprng_strong64();

	memcpy(sc->sc_enaddr_fake, &rndval, sizeof(sc->sc_enaddr_fake));
	sc->sc_enaddr_fake[0] &= 0xFE;
	sc->sc_enaddr_fake[0] |= 0x02;
}

static int
iavf_replace_lla(struct ifnet *ifp, const uint8_t *prev, const uint8_t *next)
{
	union {
		struct sockaddr sa;
		struct sockaddr_dl sdl;
		struct sockaddr_storage ss;
	} u;
	struct psref psref_prev, psref_next;
	struct ifaddr *ifa_prev, *ifa_next;
	const struct sockaddr_dl *nsdl;
	int s, error;

	KASSERT(IFNET_LOCKED(ifp));

	error = 0;
	ifa_prev = ifa_next = NULL;

	if (memcmp(prev, next, ETHER_ADDR_LEN) == 0) {
		goto done;
	}

	if (sockaddr_dl_init(&u.sdl, sizeof(u.ss), ifp->if_index,
	    ifp->if_type, ifp->if_xname, strlen(ifp->if_xname),
	    prev, ETHER_ADDR_LEN) == NULL) {
		error = EINVAL;
		goto done;
	}

	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa_prev, ifp) {
		if (sockaddr_cmp(&u.sa, ifa_prev->ifa_addr) == 0) {
			ifa_acquire(ifa_prev, &psref_prev);
			break;
		}
	}
	pserialize_read_exit(s);

	if (sockaddr_dl_init(&u.sdl, sizeof(u.ss), ifp->if_index,
	    ifp->if_type, ifp->if_xname, strlen(ifp->if_xname),
	    next, ETHER_ADDR_LEN) == NULL) {
		error = EINVAL;
		goto done;
	}

	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa_next, ifp) {
		if (sockaddr_cmp(&u.sa, ifa_next->ifa_addr) == 0) {
			ifa_acquire(ifa_next, &psref_next);
			break;
		}
	}
	pserialize_read_exit(s);

	if (ifa_next == NULL) {
		nsdl = &u.sdl;
		ifa_next = if_dl_create(ifp, &nsdl);
		if (ifa_next == NULL) {
			error = ENOMEM;
			goto done;
		}

		s = pserialize_read_enter();
		ifa_acquire(ifa_next, &psref_next);
		pserialize_read_exit(s);

		sockaddr_copy(ifa_next->ifa_addr,
		    ifa_next->ifa_addr->sa_len, &u.sa);
		ifa_insert(ifp, ifa_next);
	} else {
		nsdl = NULL;
	}

	if (ifa_prev != NULL && ifa_prev == ifp->if_dl) {
		if_activate_sadl(ifp, ifa_next, nsdl);
	}

	ifa_release(ifa_next, &psref_next);
	ifa_next = NULL;

	if (ifa_prev != NULL && ifa_prev != ifp->if_hwdl) {
		ifaref(ifa_prev);
		ifa_release(ifa_prev, &psref_prev);
		ifa_remove(ifp, ifa_prev);
		KASSERTMSG(ifa_prev->ifa_refcnt == 1, "ifa_refcnt=%d",
		   ifa_prev->ifa_refcnt);
		ifafree(ifa_prev);
		ifa_prev = NULL;
	}

	if (ISSET(ifp->if_flags, IFF_RUNNING))
		error = ENETRESET;

done:
	if (ifa_prev != NULL)
		ifa_release(ifa_prev, &psref_prev);
	if (ifa_next != NULL)
		ifa_release(ifa_next, &psref_next);

	return error;
}
static int
iavf_add_multi(struct iavf_softc *sc, uint8_t *addrlo, uint8_t *addrhi)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	int rv;

	if (ISSET(ifp->if_flags, IFF_ALLMULTI))
		return 0;

	if (memcmp(addrlo, addrhi, ETHER_ADDR_LEN) != 0) {
		iavf_del_all_multi(sc);
		SET(ifp->if_flags, IFF_ALLMULTI);
		return ENETRESET;
	}

	rv = iavf_eth_addr(sc, addrlo, IAVF_VC_OP_ADD_ETH_ADDR);

	if (rv == ENOSPC) {
		iavf_del_all_multi(sc);
		SET(ifp->if_flags, IFF_ALLMULTI);
		return ENETRESET;
	}

	return rv;
}

static int
iavf_del_multi(struct iavf_softc *sc, uint8_t *addrlo, uint8_t *addrhi)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ethercom *ec = &sc->sc_ec;
	struct ether_multi *enm, *enm_last;
	struct ether_multistep step;
	int error, rv = 0;

	if (!ISSET(ifp->if_flags, IFF_ALLMULTI)) {
		if (memcmp(addrlo, addrhi, ETHER_ADDR_LEN) != 0)
			return 0;

		iavf_eth_addr(sc, addrlo, IAVF_VC_OP_DEL_ETH_ADDR);
		return 0;
	}

	ETHER_LOCK(ec);
	for (ETHER_FIRST_MULTI(step, ec, enm); enm != NULL;
	    ETHER_NEXT_MULTI(step, enm)) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0) {
			goto out;
		}
	}

	for (ETHER_FIRST_MULTI(step, ec, enm); enm != NULL;
	    ETHER_NEXT_MULTI(step, enm)) {
		error = iavf_eth_addr(sc, enm->enm_addrlo,
		    IAVF_VC_OP_ADD_ETH_ADDR);
		if (error != 0)
			break;
	}

	if (enm != NULL) {
		enm_last = enm;
		for (ETHER_FIRST_MULTI(step, ec, enm); enm != NULL;
		    ETHER_NEXT_MULTI(step, enm)) {
			if (enm == enm_last)
				break;

			iavf_eth_addr(sc, enm->enm_addrlo,
			    IAVF_VC_OP_DEL_ETH_ADDR);
		}
	} else {
		CLR(ifp->if_flags, IFF_ALLMULTI);
		rv = ENETRESET;
	}

out:
	ETHER_UNLOCK(ec);
	return rv;
}

static void
iavf_del_all_multi(struct iavf_softc *sc)
{
	struct ethercom *ec = &sc->sc_ec;
	struct ether_multi *enm;
	struct ether_multistep step;

	ETHER_LOCK(ec);
	for (ETHER_FIRST_MULTI(step, ec, enm); enm != NULL;
	    ETHER_NEXT_MULTI(step, enm)) {
		iavf_eth_addr(sc, enm->enm_addrlo,
		    IAVF_VC_OP_DEL_ETH_ADDR);
	}
	ETHER_UNLOCK(ec);
}

static int
iavf_setup_interrupts(struct iavf_softc *sc)
{
	struct pci_attach_args *pa;
	kcpuset_t *affinity = NULL;
	char intrbuf[PCI_INTRSTR_LEN], xnamebuf[32];
	char const *intrstr;
	int counts[PCI_INTR_TYPE_SIZE];
	int error, affinity_to;
	unsigned int vector, qid, num;

	/* queue pairs + misc interrupt */
	num = sc->sc_nqps_alloc + 1;

	num = MIN(num, iavf_calc_msix_count(sc));
	if (num <= 0) {
		return -1;
	}

	KASSERT(sc->sc_nqps_alloc > 0);
	num = MIN(num, sc->sc_nqps_alloc + 1);

	pa = &sc->sc_pa;
	memset(counts, 0, sizeof(counts));
	counts[PCI_INTR_TYPE_MSIX] = num;

	error = pci_intr_alloc(pa, &sc->sc_ihp, counts, PCI_INTR_TYPE_MSIX);
	if (error != 0) {
		IAVF_LOG(sc, LOG_WARNING, "couldn't allocate interrupts\n");
		return -1;
	}

	KASSERT(pci_intr_type(pa->pa_pc, sc->sc_ihp[0]) == PCI_INTR_TYPE_MSIX);

	if (counts[PCI_INTR_TYPE_MSIX] < 1) {
		IAVF_LOG(sc, LOG_ERR, "couldn't allocate interrupts\n");
	} else if (counts[PCI_INTR_TYPE_MSIX] != (int)num) {
		IAVF_LOG(sc, LOG_DEBUG,
		    "request %u intruppts, but allocate %d interrupts\n",
		    num, counts[PCI_INTR_TYPE_MSIX]);
		num = counts[PCI_INTR_TYPE_MSIX];
	}

	sc->sc_ihs = kmem_zalloc(sizeof(sc->sc_ihs[0]) * num, KM_NOSLEEP);
	if (sc->sc_ihs == NULL) {
		IAVF_LOG(sc, LOG_ERR,
		    "couldn't allocate memory for interrupts\n");
		goto fail;
	}

	/* vector #0 is Misc interrupt */
	vector = 0;
	pci_intr_setattr(pa->pa_pc, &sc->sc_ihp[vector], PCI_INTR_MPSAFE, true);
	intrstr = pci_intr_string(pa->pa_pc, sc->sc_ihp[vector],
	    intrbuf, sizeof(intrbuf));
	snprintf(xnamebuf, sizeof(xnamebuf), "%s-Misc",
	    device_xname(sc->sc_dev));

	sc->sc_ihs[vector] = pci_intr_establish_xname(pa->pa_pc,
	    sc->sc_ihp[vector], IPL_NET, iavf_intr, sc, xnamebuf);
	if (sc->sc_ihs[vector] == NULL) {
		IAVF_LOG(sc, LOG_WARNING,
		    "unable to establish interrupt at %s", intrstr);
		goto fail;
	}

	kcpuset_create(&affinity, false);
	affinity_to = 0;
	qid = 0;
	for (vector = 1; vector < num; vector++) {
		pci_intr_setattr(pa->pa_pc, &sc->sc_ihp[vector],
		    PCI_INTR_MPSAFE, true);
		intrstr = pci_intr_string(pa->pa_pc, sc->sc_ihp[vector],
		    intrbuf, sizeof(intrbuf));
		snprintf(xnamebuf, sizeof(xnamebuf), "%s-TXRX%u",
		    device_xname(sc->sc_dev), qid);

		sc->sc_ihs[vector] = pci_intr_establish_xname(pa->pa_pc,
		    sc->sc_ihp[vector], IPL_NET, iavf_queue_intr,
		    (void *)&sc->sc_qps[qid], xnamebuf);
		if (sc->sc_ihs[vector] == NULL) {
			IAVF_LOG(sc, LOG_WARNING,
			    "unable to establish interrupt at %s\n", intrstr);
			goto fail;
		}

		kcpuset_zero(affinity);
		kcpuset_set(affinity, affinity_to);
		error = interrupt_distribute(sc->sc_ihs[vector],
		    affinity, NULL);

		if (error == 0) {
			IAVF_LOG(sc, LOG_INFO,
			    "for TXRX%d interrupt at %s, affinity to %d\n",
			    qid, intrstr, affinity_to);
		} else {
			IAVF_LOG(sc, LOG_INFO,
			    "for TXRX%d interrupt at %s\n",
			    qid, intrstr);
		}

		qid++;
		affinity_to = (affinity_to + 1) % ncpu;
	}

	vector = 0;
	kcpuset_zero(affinity);
	kcpuset_set(affinity, affinity_to);
	intrstr = pci_intr_string(pa->pa_pc, sc->sc_ihp[vector],
	    intrbuf, sizeof(intrbuf));
	error = interrupt_distribute(sc->sc_ihs[vector], affinity, NULL);
	if (error == 0) {
		IAVF_LOG(sc, LOG_INFO,
		    "for Misc interrupt at %s, affinity to %d\n",
		    intrstr, affinity_to);
	} else {
		IAVF_LOG(sc, LOG_INFO,
		    "for MISC interrupt at %s\n", intrstr);
	}

	kcpuset_destroy(affinity);

	sc->sc_nintrs = num;
	return 0;

fail:
	if (affinity != NULL)
		kcpuset_destroy(affinity);
	for (vector = 0; vector < num; vector++) {
		if (sc->sc_ihs[vector] == NULL)
			continue;
		pci_intr_disestablish(pa->pa_pc, sc->sc_ihs[vector]);
	}
	kmem_free(sc->sc_ihs, sizeof(sc->sc_ihs[0]) * num);
	pci_intr_release(pa->pa_pc, sc->sc_ihp, num);

	return -1;
}

static void
iavf_teardown_interrupts(struct iavf_softc *sc)
{
	struct pci_attach_args *pa;
	unsigned int i;

	if (sc->sc_ihs == NULL)
		return;

	pa = &sc->sc_pa;

	for (i = 0; i < sc->sc_nintrs; i++) {
		pci_intr_disestablish(pa->pa_pc, sc->sc_ihs[i]);
	}

	kmem_free(sc->sc_ihs, sizeof(sc->sc_ihs[0]) * sc->sc_nintrs);
	sc->sc_ihs = NULL;

	pci_intr_release(pa->pa_pc, sc->sc_ihp, sc->sc_nintrs);
	sc->sc_nintrs = 0;
}

static int
iavf_setup_sysctls(struct iavf_softc *sc)
{
	const char *devname;
	struct sysctllog **log;
	const struct sysctlnode *rnode, *rxnode, *txnode;
	int error;

	log = &sc->sc_sysctllog;
	devname = device_xname(sc->sc_dev);

	error = sysctl_createv(log, 0, NULL, &rnode,
	    0, CTLTYPE_NODE, devname,
	    SYSCTL_DESCR("iavf information and settings"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "txrx_workqueue",
	    SYSCTL_DESCR("Use workqueue for packet processing"),
	    NULL, 0, &sc->sc_txrx_workqueue, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "debug_level",
	    SYSCTL_DESCR("Debug level"),
	    NULL, 0, &sc->sc_debuglevel, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, &rxnode,
	    0, CTLTYPE_NODE, "rx",
	    SYSCTL_DESCR("iavf information and settings for Rx"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rxnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "itr",
	    SYSCTL_DESCR("Interrupt Throttling"),
	    iavf_sysctl_itr_handler, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rxnode, NULL,
	    CTLFLAG_READONLY, CTLTYPE_INT, "descriptor_num",
	    SYSCTL_DESCR("descriptor size"),
	    NULL, 0, &sc->sc_rx_ring_ndescs, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rxnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "intr_process_limit",
	    SYSCTL_DESCR("max number of Rx packets"
	    " to process for interrupt processing"),
	    NULL, 0, &sc->sc_rx_intr_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rxnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "process_limit",
	    SYSCTL_DESCR("max number of Rx packets"
	    " to process for deferred processing"),
	    NULL, 0, &sc->sc_rx_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &rnode, &txnode,
	    0, CTLTYPE_NODE, "tx",
	    SYSCTL_DESCR("iavf information and settings for Tx"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &txnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "itr",
	    SYSCTL_DESCR("Interrupt Throttling"),
	    iavf_sysctl_itr_handler, 0,
	    (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &txnode, NULL,
	    CTLFLAG_READONLY, CTLTYPE_INT, "descriptor_num",
	    SYSCTL_DESCR("the number of Tx descriptors"),
	    NULL, 0, &sc->sc_tx_ring_ndescs, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &txnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "intr_process_limit",
	    SYSCTL_DESCR("max number of Tx packets"
	    " to process for interrupt processing"),
	    NULL, 0, &sc->sc_tx_intr_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;

	error = sysctl_createv(log, 0, &txnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "process_limit",
	    SYSCTL_DESCR("max number of Tx packets"
	    " to process for deferred processing"),
	    NULL, 0, &sc->sc_tx_process_limit, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto out;
out:
	return error;
}

static void
iavf_teardown_sysctls(struct iavf_softc *sc)
{

	sysctl_teardown(&sc->sc_sysctllog);
}

static int
iavf_setup_stats(struct iavf_softc *sc)
{
	struct iavf_stat_counters *isc;
	const char *dn;

	dn = device_xname(sc->sc_dev);
	isc = &sc->sc_stat_counters;

	iavf_evcnt_attach(&isc->isc_rx_bytes, dn, "Rx bytes");
	iavf_evcnt_attach(&isc->isc_rx_unicast, dn, "Rx unicast");
	iavf_evcnt_attach(&isc->isc_rx_multicast, dn, "Rx multicast");
	iavf_evcnt_attach(&isc->isc_rx_broadcast, dn, "Rx broadcast");
	iavf_evcnt_attach(&isc->isc_rx_discards, dn, "Rx discards");
	iavf_evcnt_attach(&isc->isc_rx_unknown_protocol,
	    dn, "Rx unknown protocol");

	iavf_evcnt_attach(&isc->isc_tx_bytes, dn, "Tx bytes");
	iavf_evcnt_attach(&isc->isc_tx_unicast, dn, "Tx unicast");
	iavf_evcnt_attach(&isc->isc_tx_multicast, dn, "Tx multicast");
	iavf_evcnt_attach(&isc->isc_tx_broadcast, dn, "Tx broadcast");
	iavf_evcnt_attach(&isc->isc_tx_discards, dn, "Tx discards");
	iavf_evcnt_attach(&isc->isc_tx_errors, dn, "Tx errors");

	return 0;
}

static void
iavf_teardown_stats(struct iavf_softc *sc)
{
	struct iavf_stat_counters *isc;

	isc = &sc->sc_stat_counters;

	evcnt_detach(&isc->isc_rx_bytes);
	evcnt_detach(&isc->isc_rx_unicast);
	evcnt_detach(&isc->isc_rx_multicast);
	evcnt_detach(&isc->isc_rx_broadcast);
	evcnt_detach(&isc->isc_rx_discards);
	evcnt_detach(&isc->isc_rx_unknown_protocol);

	evcnt_detach(&isc->isc_tx_bytes);
	evcnt_detach(&isc->isc_tx_unicast);
	evcnt_detach(&isc->isc_tx_multicast);
	evcnt_detach(&isc->isc_tx_broadcast);
	evcnt_detach(&isc->isc_tx_discards);
	evcnt_detach(&isc->isc_tx_errors);

}

static int
iavf_init_admin_queue(struct iavf_softc *sc)
{
	uint32_t reg;

	sc->sc_atq_cons = 0;
	sc->sc_atq_prod = 0;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
	    0, IXL_DMA_LEN(&sc->sc_arq),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	iavf_wr(sc, sc->sc_aq_regs->atq_head, 0);
	iavf_wr(sc, sc->sc_aq_regs->arq_head, 0);
	iavf_wr(sc, sc->sc_aq_regs->atq_tail, 0);
	iavf_wr(sc, sc->sc_aq_regs->arq_tail, 0);

	iavf_barrier(sc, 0, sc->sc_mems, BUS_SPACE_BARRIER_WRITE);

	iavf_wr(sc, sc->sc_aq_regs->atq_bal,
	    ixl_dmamem_lo(&sc->sc_atq));
	iavf_wr(sc, sc->sc_aq_regs->atq_bah,
	    ixl_dmamem_hi(&sc->sc_atq));
	iavf_wr(sc, sc->sc_aq_regs->atq_len,
	    sc->sc_aq_regs->atq_len_enable | IAVF_AQ_NUM);

	iavf_wr(sc, sc->sc_aq_regs->arq_bal,
	    ixl_dmamem_lo(&sc->sc_arq));
	iavf_wr(sc, sc->sc_aq_regs->arq_bah,
	    ixl_dmamem_hi(&sc->sc_arq));
	iavf_wr(sc, sc->sc_aq_regs->arq_len,
	    sc->sc_aq_regs->arq_len_enable | IAVF_AQ_NUM);

	iavf_wr(sc, sc->sc_aq_regs->arq_tail, sc->sc_arq_prod);

	reg = iavf_rd(sc, sc->sc_aq_regs->atq_bal);
	if (reg != ixl_dmamem_lo(&sc->sc_atq))
		goto fail;

	reg = iavf_rd(sc, sc->sc_aq_regs->arq_bal);
	if (reg != ixl_dmamem_lo(&sc->sc_arq))
		goto fail;

	sc->sc_dead = false;
	return 0;

fail:
	iavf_wr(sc, sc->sc_aq_regs->atq_len, 0);
	iavf_wr(sc, sc->sc_aq_regs->arq_len, 0);
	return -1;
}

static void
iavf_cleanup_admin_queue(struct iavf_softc *sc)
{
	struct ixl_aq_buf *aqb;

	iavf_wr(sc, sc->sc_aq_regs->atq_head, 0);
	iavf_wr(sc, sc->sc_aq_regs->arq_head, 0);
	iavf_wr(sc, sc->sc_aq_regs->atq_tail, 0);
	iavf_wr(sc, sc->sc_aq_regs->arq_tail, 0);

	iavf_wr(sc, sc->sc_aq_regs->atq_bal, 0);
	iavf_wr(sc, sc->sc_aq_regs->atq_bah, 0);
	iavf_wr(sc, sc->sc_aq_regs->atq_len, 0);

	iavf_wr(sc, sc->sc_aq_regs->arq_bal, 0);
	iavf_wr(sc, sc->sc_aq_regs->arq_bah, 0);
	iavf_wr(sc, sc->sc_aq_regs->arq_len, 0);
	iavf_flush(sc);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
	    0, IXL_DMA_LEN(&sc->sc_arq),
	    BUS_DMASYNC_POSTREAD);
	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	sc->sc_atq_cons = 0;
	sc->sc_atq_prod = 0;
	sc->sc_arq_cons = 0;
	sc->sc_arq_prod = 0;

	memset(IXL_DMA_KVA(&sc->sc_arq), 0, IXL_DMA_LEN(&sc->sc_arq));
	memset(IXL_DMA_KVA(&sc->sc_atq), 0, IXL_DMA_LEN(&sc->sc_atq));

	while ((aqb = iavf_aqb_get_locked(&sc->sc_arq_live)) != NULL) {
		bus_dmamap_sync(sc->sc_dmat, aqb->aqb_map, 0, aqb->aqb_size,
		    BUS_DMASYNC_POSTREAD);
		iavf_aqb_put_locked(&sc->sc_arq_idle, aqb);
	}

	while ((aqb = iavf_aqb_get_locked(&sc->sc_atq_live)) != NULL) {
		bus_dmamap_sync(sc->sc_dmat, aqb->aqb_map, 0, aqb->aqb_size,
		    BUS_DMASYNC_POSTREAD);
		iavf_aqb_put_locked(&sc->sc_atq_idle, aqb);
	}
}

static unsigned int
iavf_calc_msix_count(struct iavf_softc *sc)
{
	struct pci_attach_args *pa;
	int count;

	pa = &sc->sc_pa;
	count = pci_msix_count(pa->pa_pc, pa->pa_tag);
	if (count < 0) {
		IAVF_LOG(sc, LOG_DEBUG,"MSIX config error\n");
		count = 0;
	}

	return MIN(sc->sc_max_vectors, (unsigned int)count);
}

static unsigned int
iavf_calc_queue_pair_size(struct iavf_softc *sc)
{
	unsigned int nqp, nvec;

	nvec = iavf_calc_msix_count(sc);
	if (sc->sc_max_vectors > 1) {
		/* decrease the number of misc interrupt */
		nvec -= 1;
	}

	nqp = ncpu;
	nqp = MIN(nqp, sc->sc_nqps_vsi);
	nqp = MIN(nqp, nvec);
	nqp = MIN(nqp, (unsigned int)iavf_params.max_qps);

	return nqp;
}

static struct iavf_tx_ring *
iavf_txr_alloc(struct iavf_softc *sc, unsigned int qid)
{
	struct iavf_tx_ring *txr;
	struct iavf_tx_map *maps;
	unsigned int i;
	int error;

	txr = kmem_zalloc(sizeof(*txr), KM_NOSLEEP);
	if (txr == NULL)
		return NULL;

	maps = kmem_zalloc(sizeof(maps[0]) * sc->sc_tx_ring_ndescs,
	    KM_NOSLEEP);
	if (maps == NULL)
		goto free_txr;

	if (iavf_dmamem_alloc(sc->sc_dmat, &txr->txr_mem,
	    sizeof(struct ixl_tx_desc) * sc->sc_tx_ring_ndescs,
	    IAVF_TX_QUEUE_ALIGN) != 0) {
		goto free_maps;
	}

	for (i = 0; i < sc->sc_tx_ring_ndescs; i++) {
		error = bus_dmamap_create(sc->sc_dmat, IAVF_TX_PKT_MAXSIZE,
		    IAVF_TX_PKT_DESCS, IAVF_TX_PKT_MAXSIZE, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &maps[i].txm_map);
		if (error)
			goto destroy_maps;
	}

	txr->txr_intrq = pcq_create(sc->sc_tx_ring_ndescs, KM_NOSLEEP);
	if (txr->txr_intrq == NULL)
		goto destroy_maps;

	txr->txr_si = softint_establish(SOFTINT_NET|SOFTINT_MPSAFE,
	    iavf_deferred_transmit, txr);
	if (txr->txr_si == NULL)
		goto destroy_pcq;

	snprintf(txr->txr_name, sizeof(txr->txr_name), "%s-tx%d",
	    device_xname(sc->sc_dev), qid);

	iavf_evcnt_attach(&txr->txr_defragged,
	    txr->txr_name, "m_defrag successed");
	iavf_evcnt_attach(&txr->txr_defrag_failed,
	    txr->txr_name, "m_defrag failed");
	iavf_evcnt_attach(&txr->txr_pcqdrop,
	    txr->txr_name, "Dropped in pcq");
	iavf_evcnt_attach(&txr->txr_transmitdef,
	    txr->txr_name, "Deferred transmit");
	iavf_evcnt_attach(&txr->txr_watchdogto,
	    txr->txr_name, "Watchdog timedout on queue");
	iavf_evcnt_attach(&txr->txr_defer,
	    txr->txr_name, "Handled queue in softint/workqueue");

	evcnt_attach_dynamic(&txr->txr_intr, EVCNT_TYPE_INTR, NULL,
	    txr->txr_name, "Interrupt on queue");

	txr->txr_qid = qid;
	txr->txr_sc = sc;
	txr->txr_maps = maps;
	txr->txr_prod = txr->txr_cons = 0;
	txr->txr_tail = I40E_QTX_TAIL1(qid);
	mutex_init(&txr->txr_lock, MUTEX_DEFAULT, IPL_NET);

	return txr;
destroy_pcq:
	pcq_destroy(txr->txr_intrq);
destroy_maps:
	for (i = 0; i < sc->sc_tx_ring_ndescs; i++) {
		if (maps[i].txm_map == NULL)
			continue;
		bus_dmamap_destroy(sc->sc_dmat, maps[i].txm_map);
	}

	iavf_dmamem_free(sc->sc_dmat, &txr->txr_mem);
free_maps:
	kmem_free(maps, sizeof(maps[0]) * sc->sc_tx_ring_ndescs);
free_txr:
	kmem_free(txr, sizeof(*txr));
	return NULL;
}

static void
iavf_txr_free(struct iavf_softc *sc, struct iavf_tx_ring *txr)
{
	struct iavf_tx_map *maps;
	unsigned int i;

	maps = txr->txr_maps;
	if (maps != NULL) {
		for (i = 0; i < sc->sc_tx_ring_ndescs; i++) {
			if (maps[i].txm_map == NULL)
				continue;
			bus_dmamap_destroy(sc->sc_dmat, maps[i].txm_map);
		}
		kmem_free(txr->txr_maps,
		    sizeof(maps[0]) * sc->sc_tx_ring_ndescs);
		txr->txr_maps = NULL;
	}

	evcnt_detach(&txr->txr_defragged);
	evcnt_detach(&txr->txr_defrag_failed);
	evcnt_detach(&txr->txr_pcqdrop);
	evcnt_detach(&txr->txr_transmitdef);
	evcnt_detach(&txr->txr_watchdogto);
	evcnt_detach(&txr->txr_defer);
	evcnt_detach(&txr->txr_intr);

	iavf_dmamem_free(sc->sc_dmat, &txr->txr_mem);
	softint_disestablish(txr->txr_si);
	pcq_destroy(txr->txr_intrq);
	mutex_destroy(&txr->txr_lock);
	kmem_free(txr, sizeof(*txr));
}

static struct iavf_rx_ring *
iavf_rxr_alloc(struct iavf_softc *sc, unsigned int qid)
{
	struct iavf_rx_ring *rxr;
	struct iavf_rx_map *maps;
	unsigned int i;
	int error;

	rxr = kmem_zalloc(sizeof(*rxr), KM_NOSLEEP);
	if (rxr == NULL)
		return NULL;

	maps = kmem_zalloc(sizeof(maps[0]) * sc->sc_rx_ring_ndescs,
	    KM_NOSLEEP);
	if (maps == NULL)
		goto free_rxr;

	if (iavf_dmamem_alloc(sc->sc_dmat, &rxr->rxr_mem,
	    sizeof(struct ixl_rx_rd_desc_32) * sc->sc_rx_ring_ndescs,
	    IAVF_RX_QUEUE_ALIGN) != 0)
		goto free_maps;

	for (i = 0; i < sc->sc_rx_ring_ndescs; i++) {
		error = bus_dmamap_create(sc->sc_dmat, IAVF_MCLBYTES,
		    1, IAVF_MCLBYTES, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &maps[i].rxm_map);
		if (error)
			goto destroy_maps;
	}

	snprintf(rxr->rxr_name, sizeof(rxr->rxr_name), "%s-rx%d",
	    device_xname(sc->sc_dev), qid);

	iavf_evcnt_attach(&rxr->rxr_mgethdr_failed,
	    rxr->rxr_name, "MGETHDR failed");
	iavf_evcnt_attach(&rxr->rxr_mgetcl_failed,
	    rxr->rxr_name, "MCLGET failed");
	iavf_evcnt_attach(&rxr->rxr_mbuf_load_failed,
	    rxr->rxr_name, "bus_dmamap_load_mbuf failed");
	iavf_evcnt_attach(&rxr->rxr_defer,
	    rxr->rxr_name, "Handled queue in softint/workqueue");

	evcnt_attach_dynamic(&rxr->rxr_intr, EVCNT_TYPE_INTR, NULL,
	    rxr->rxr_name, "Interrupt on queue");

	rxr->rxr_qid = qid;
	rxr->rxr_sc = sc;
	rxr->rxr_cons = rxr->rxr_prod = 0;
	rxr->rxr_m_head = NULL;
	rxr->rxr_m_tail = &rxr->rxr_m_head;
	rxr->rxr_maps = maps;
	rxr->rxr_tail = I40E_QRX_TAIL1(qid);
	mutex_init(&rxr->rxr_lock, MUTEX_DEFAULT, IPL_NET);

	return rxr;

destroy_maps:
	for (i = 0; i < sc->sc_rx_ring_ndescs; i++) {
		if (maps[i].rxm_map == NULL)
			continue;
		bus_dmamap_destroy(sc->sc_dmat, maps[i].rxm_map);
	}
	iavf_dmamem_free(sc->sc_dmat, &rxr->rxr_mem);
free_maps:
	kmem_free(maps, sizeof(maps[0]) * sc->sc_rx_ring_ndescs);
free_rxr:
	kmem_free(rxr, sizeof(*rxr));

	return NULL;
}

static void
iavf_rxr_free(struct iavf_softc *sc, struct iavf_rx_ring *rxr)
{
	struct iavf_rx_map *maps;
	unsigned int i;

	maps = rxr->rxr_maps;
	if (maps != NULL) {
		for (i = 0; i < sc->sc_rx_ring_ndescs; i++) {
			if (maps[i].rxm_map == NULL)
				continue;
			bus_dmamap_destroy(sc->sc_dmat, maps[i].rxm_map);
		}
		kmem_free(maps, sizeof(maps[0]) * sc->sc_rx_ring_ndescs);
		rxr->rxr_maps = NULL;
	}

	evcnt_detach(&rxr->rxr_mgethdr_failed);
	evcnt_detach(&rxr->rxr_mgetcl_failed);
	evcnt_detach(&rxr->rxr_mbuf_load_failed);
	evcnt_detach(&rxr->rxr_defer);
	evcnt_detach(&rxr->rxr_intr);

	iavf_dmamem_free(sc->sc_dmat, &rxr->rxr_mem);
	mutex_destroy(&rxr->rxr_lock);
	kmem_free(rxr, sizeof(*rxr));
}

static int
iavf_queue_pairs_alloc(struct iavf_softc *sc)
{
	struct iavf_queue_pair *qp;
	unsigned int i, num;

	num = iavf_calc_queue_pair_size(sc);
	if (num <= 0) {
		return -1;
	}

	sc->sc_qps = kmem_zalloc(sizeof(sc->sc_qps[0]) * num, KM_NOSLEEP);
	if (sc->sc_qps == NULL) {
		return -1;
	}

	for (i = 0; i < num; i++) {
		qp = &sc->sc_qps[i];

		qp->qp_rxr = iavf_rxr_alloc(sc, i);
		qp->qp_txr = iavf_txr_alloc(sc, i);

		if (qp->qp_rxr == NULL || qp->qp_txr == NULL)
			goto free;

		qp->qp_si = softint_establish(SOFTINT_NET|SOFTINT_MPSAFE,
		    iavf_handle_queue, qp);
		if (qp->qp_si == NULL)
			goto free;
	}

	sc->sc_nqps_alloc = num;
	return 0;
free:
	for (i = 0; i < num; i++) {
		qp = &sc->sc_qps[i];

		if (qp->qp_si != NULL)
			softint_disestablish(qp->qp_si);
		if (qp->qp_rxr != NULL)
			iavf_rxr_free(sc, qp->qp_rxr);
		if (qp->qp_txr != NULL)
			iavf_txr_free(sc, qp->qp_txr);
	}

	kmem_free(sc->sc_qps, sizeof(sc->sc_qps[0]) * num);
	sc->sc_qps = NULL;

	return -1;
}

static void
iavf_queue_pairs_free(struct iavf_softc *sc)
{
	struct iavf_queue_pair *qp;
	unsigned int i;
	size_t sz;

	if (sc->sc_qps == NULL)
		return;

	for (i = 0; i < sc->sc_nqps_alloc; i++) {
		qp = &sc->sc_qps[i];

		if (qp->qp_si != NULL)
			softint_disestablish(qp->qp_si);
		if (qp->qp_rxr != NULL)
			iavf_rxr_free(sc, qp->qp_rxr);
		if (qp->qp_txr != NULL)
			iavf_txr_free(sc, qp->qp_txr);
	}

	sz = sizeof(sc->sc_qps[0]) * sc->sc_nqps_alloc;
	kmem_free(sc->sc_qps, sz);
	sc->sc_qps = NULL;
	sc->sc_nqps_alloc = 0;
}

static int
iavf_rxfill(struct iavf_softc *sc, struct iavf_rx_ring *rxr)
{
	struct ixl_rx_rd_desc_32 *ring, *rxd;
	struct iavf_rx_map *rxm;
	bus_dmamap_t map;
	struct mbuf *m;
	unsigned int slots, prod, mask;
	int error, post;

	slots = ixl_rxr_unrefreshed(rxr->rxr_prod, rxr->rxr_cons,
	    sc->sc_rx_ring_ndescs);

	if (slots == 0)
		return 0;

	error = 0;
	prod = rxr->rxr_prod;

	ring = IXL_DMA_KVA(&rxr->rxr_mem);
	mask = sc->sc_rx_ring_ndescs - 1;

	do {
		rxm = &rxr->rxr_maps[prod];

		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			rxr->rxr_mgethdr_failed.ev_count++;
			error = -1;
			break;
		}

		MCLGET(m, M_DONTWAIT);
		if (!ISSET(m->m_flags, M_EXT)) {
			rxr->rxr_mgetcl_failed.ev_count++;
			error = -1;
			m_freem(m);
			break;
		}

		m->m_len = m->m_pkthdr.len = MCLBYTES;
		m_adj(m, ETHER_ALIGN);

		map = rxm->rxm_map;

		if (bus_dmamap_load_mbuf(sc->sc_dmat, map, m,
		    BUS_DMA_READ|BUS_DMA_NOWAIT) != 0) {
			rxr->rxr_mbuf_load_failed.ev_count++;
			error = -1;
			m_freem(m);
			break;
		}

		rxm->rxm_m = m;

		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_PREREAD);

		rxd = &ring[prod];
		rxd->paddr = htole64(map->dm_segs[0].ds_addr);
		rxd->haddr = htole64(0);

		prod++;
		prod &= mask;
		post = 1;
	} while (--slots);

	if (post) {
		rxr->rxr_prod = prod;
		iavf_wr(sc, rxr->rxr_tail, prod);
	}

	return error;
}

static inline void
iavf_rx_csum(struct mbuf *m, uint64_t qword)
{
	int flags_mask;

	if (!ISSET(qword, IXL_RX_DESC_L3L4P)) {
		/* No L3 or L4 checksum was calculated */
		return;
	}

	switch (__SHIFTOUT(qword, IXL_RX_DESC_PTYPE_MASK)) {
	case IXL_RX_DESC_PTYPE_IPV4FRAG:
	case IXL_RX_DESC_PTYPE_IPV4:
	case IXL_RX_DESC_PTYPE_SCTPV4:
	case IXL_RX_DESC_PTYPE_ICMPV4:
		flags_mask = M_CSUM_IPv4 | M_CSUM_IPv4_BAD;
		break;
	case IXL_RX_DESC_PTYPE_TCPV4:
		flags_mask = M_CSUM_IPv4 | M_CSUM_IPv4_BAD;
		flags_mask |= M_CSUM_TCPv4 | M_CSUM_TCP_UDP_BAD;
		break;
	case IXL_RX_DESC_PTYPE_UDPV4:
		flags_mask = M_CSUM_IPv4 | M_CSUM_IPv4_BAD;
		flags_mask |= M_CSUM_UDPv4 | M_CSUM_TCP_UDP_BAD;
		break;
	case IXL_RX_DESC_PTYPE_TCPV6:
		flags_mask = M_CSUM_TCPv6 | M_CSUM_TCP_UDP_BAD;
		break;
	case IXL_RX_DESC_PTYPE_UDPV6:
		flags_mask = M_CSUM_UDPv6 | M_CSUM_TCP_UDP_BAD;
		break;
	default:
		flags_mask = 0;
	}

	m->m_pkthdr.csum_flags |= (flags_mask & (M_CSUM_IPv4 |
	    M_CSUM_TCPv4 | M_CSUM_TCPv6 | M_CSUM_UDPv4 | M_CSUM_UDPv6));

	if (ISSET(qword, IXL_RX_DESC_IPE)) {
		m->m_pkthdr.csum_flags |= (flags_mask & M_CSUM_IPv4_BAD);
	}

	if (ISSET(qword, IXL_RX_DESC_L4E)) {
		m->m_pkthdr.csum_flags |= (flags_mask & M_CSUM_TCP_UDP_BAD);
	}
}

static int
iavf_rxeof(struct iavf_softc *sc, struct iavf_rx_ring *rxr, u_int rxlimit,
    struct evcnt *ecnt)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ixl_rx_wb_desc_32 *ring, *rxd;
	struct iavf_rx_map *rxm;
	bus_dmamap_t map;
	unsigned int cons, prod;
	struct mbuf *m;
	uint64_t word, word0;
	unsigned int len;
	unsigned int mask;
	int done = 0, more = 0;

	KASSERT(mutex_owned(&rxr->rxr_lock));

	if (!ISSET(ifp->if_flags, IFF_RUNNING))
		return 0;

	prod = rxr->rxr_prod;
	cons = rxr->rxr_cons;

	if (cons == prod)
		return 0;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&rxr->rxr_mem),
	    0, IXL_DMA_LEN(&rxr->rxr_mem),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	ring = IXL_DMA_KVA(&rxr->rxr_mem);
	mask = sc->sc_rx_ring_ndescs - 1;

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);

	do {
		if (rxlimit-- <= 0) {
			more = 1;
			break;
		}

		rxd = &ring[cons];

		word = le64toh(rxd->qword1);

		if (!ISSET(word, IXL_RX_DESC_DD))
			break;

		rxm = &rxr->rxr_maps[cons];

		map = rxm->rxm_map;
		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, map);

		m = rxm->rxm_m;
		rxm->rxm_m = NULL;

		KASSERT(m != NULL);

		len = (word & IXL_RX_DESC_PLEN_MASK) >> IXL_RX_DESC_PLEN_SHIFT;
		m->m_len = len;
		m->m_pkthdr.len = 0;

		m->m_next = NULL;
		*rxr->rxr_m_tail = m;
		rxr->rxr_m_tail = &m->m_next;

		m = rxr->rxr_m_head;
		m->m_pkthdr.len += len;

		if (ISSET(word, IXL_RX_DESC_EOP)) {
			word0 = le64toh(rxd->qword0);

			if (ISSET(word, IXL_RX_DESC_L2TAG1P)) {
				uint16_t vtag;
				vtag = __SHIFTOUT(word0, IXL_RX_DESC_L2TAG1_MASK);
				vlan_set_tag(m, le16toh(vtag));
			}

			if ((ifp->if_capenable & IAVF_IFCAP_RXCSUM) != 0)
				iavf_rx_csum(m, word);

			if (!ISSET(word,
			    IXL_RX_DESC_RXE | IXL_RX_DESC_OVERSIZE)) {
				m_set_rcvif(m, ifp);
				if_statinc_ref(nsr, if_ipackets);
				if_statadd_ref(nsr, if_ibytes,
				    m->m_pkthdr.len);
				if_percpuq_enqueue(sc->sc_ipq, m);
			} else {
				if_statinc_ref(nsr, if_ierrors);
				m_freem(m);
			}

			rxr->rxr_m_head = NULL;
			rxr->rxr_m_tail = &rxr->rxr_m_head;
		}

		cons++;
		cons &= mask;

		done = 1;
	} while (cons != prod);

	if (done) {
		ecnt->ev_count++;
		rxr->rxr_cons = cons;
		if (iavf_rxfill(sc, rxr) == -1)
			if_statinc_ref(nsr, if_iqdrops);
	}

	IF_STAT_PUTREF(ifp);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&rxr->rxr_mem),
	    0, IXL_DMA_LEN(&rxr->rxr_mem),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	return more;
}

static void
iavf_rxr_clean(struct iavf_softc *sc, struct iavf_rx_ring *rxr)
{
	struct iavf_rx_map *maps, *rxm;
	bus_dmamap_t map;
	unsigned int i;

	KASSERT(mutex_owned(&rxr->rxr_lock));

	maps = rxr->rxr_maps;
	for (i = 0; i < sc->sc_rx_ring_ndescs; i++) {
		rxm = &maps[i];

		if (rxm->rxm_m == NULL)
			continue;

		map = rxm->rxm_map;
		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, map);

		m_freem(rxm->rxm_m);
		rxm->rxm_m = NULL;
	}

	m_freem(rxr->rxr_m_head);
	rxr->rxr_m_head = NULL;
	rxr->rxr_m_tail = &rxr->rxr_m_head;

	memset(IXL_DMA_KVA(&rxr->rxr_mem), 0, IXL_DMA_LEN(&rxr->rxr_mem));
	rxr->rxr_prod = rxr->rxr_cons = 0;
}

static int
iavf_txeof(struct iavf_softc *sc, struct iavf_tx_ring *txr, u_int txlimit,
    struct evcnt *ecnt)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ixl_tx_desc *ring, *txd;
	struct iavf_tx_map *txm;
	struct mbuf *m;
	bus_dmamap_t map;
	unsigned int cons, prod, last;
	unsigned int mask;
	uint64_t dtype;
	int done = 0, more = 0;

	KASSERT(mutex_owned(&txr->txr_lock));

	prod = txr->txr_prod;
	cons = txr->txr_cons;

	if (cons == prod)
		return 0;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&txr->txr_mem),
	    0, IXL_DMA_LEN(&txr->txr_mem), BUS_DMASYNC_POSTREAD);

	ring = IXL_DMA_KVA(&txr->txr_mem);
	mask = sc->sc_tx_ring_ndescs - 1;

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);

	do {
		if (txlimit-- <= 0) {
			more = 1;
			break;
		}

		txm = &txr->txr_maps[cons];
		last = txm->txm_eop;
		txd = &ring[last];

		dtype = txd->cmd & htole64(IXL_TX_DESC_DTYPE_MASK);
		if (dtype != htole64(IXL_TX_DESC_DTYPE_DONE))
			break;

		map = txm->txm_map;

		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, map);

		m = txm->txm_m;
		if (m != NULL) {
			if_statinc_ref(nsr, if_opackets);
			if_statadd_ref(nsr, if_obytes, m->m_pkthdr.len);
			if (ISSET(m->m_flags, M_MCAST))
				if_statinc_ref(nsr, if_omcasts);
			m_freem(m);
		}

		txm->txm_m = NULL;
		txm->txm_eop = -1;

		cons = last + 1;
		cons &= mask;
		done = 1;
	} while (cons != prod);

	IF_STAT_PUTREF(ifp);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&txr->txr_mem),
	    0, IXL_DMA_LEN(&txr->txr_mem), BUS_DMASYNC_PREREAD);

	txr->txr_cons = cons;

	if (done) {
		ecnt->ev_count++;
		softint_schedule(txr->txr_si);
		if (txr->txr_qid == 0) {
			CLR(ifp->if_flags, IFF_OACTIVE);
			if_schedule_deferred_start(ifp);
		}
	}

	if (txr->txr_cons == txr->txr_prod) {
		txr->txr_watchdog = IAVF_WATCHDOG_STOP;
	}

	return more;
}

static inline int
iavf_load_mbuf(bus_dma_tag_t dmat, bus_dmamap_t map, struct mbuf **m0,
    struct iavf_tx_ring *txr)
{
	struct mbuf *m;
	int error;

	KASSERT(mutex_owned(&txr->txr_lock));

	m = *m0;

	error = bus_dmamap_load_mbuf(dmat, map, m,
	    BUS_DMA_STREAMING|BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	if (error != EFBIG)
		return error;

	m = m_defrag(m, M_DONTWAIT);
	if (m != NULL) {
		*m0 = m;
		txr->txr_defragged.ev_count++;
		error = bus_dmamap_load_mbuf(dmat, map, m,
		    BUS_DMA_STREAMING|BUS_DMA_WRITE|BUS_DMA_NOWAIT);
	} else {
		txr->txr_defrag_failed.ev_count++;
		error = ENOBUFS;
	}

	return error;
}

static inline int
iavf_tx_setup_offloads(struct mbuf *m, uint64_t *cmd_txd)
{
	struct ether_header *eh;
	size_t len;
	uint64_t cmd;

	cmd = 0;

	eh = mtod(m, struct ether_header *);
	switch (htons(eh->ether_type)) {
	case ETHERTYPE_IP:
	case ETHERTYPE_IPV6:
		len = ETHER_HDR_LEN;
		break;
	case ETHERTYPE_VLAN:
		len = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
		break;
	default:
		len = 0;
	}
	cmd |= ((len >> 1) << IXL_TX_DESC_MACLEN_SHIFT);

	if (m->m_pkthdr.csum_flags &
	    (M_CSUM_TSOv4 | M_CSUM_TCPv4 | M_CSUM_UDPv4)) {
		cmd |= IXL_TX_DESC_CMD_IIPT_IPV4;
	}
	if (m->m_pkthdr.csum_flags & M_CSUM_IPv4) {
		cmd |= IXL_TX_DESC_CMD_IIPT_IPV4_CSUM;
	}

	if (m->m_pkthdr.csum_flags &
	    (M_CSUM_TSOv6 | M_CSUM_TCPv6 | M_CSUM_UDPv6)) {
		cmd |= IXL_TX_DESC_CMD_IIPT_IPV6;
	}

	switch (cmd & IXL_TX_DESC_CMD_IIPT_MASK) {
	case IXL_TX_DESC_CMD_IIPT_IPV4:
	case IXL_TX_DESC_CMD_IIPT_IPV4_CSUM:
		len = M_CSUM_DATA_IPv4_IPHL(m->m_pkthdr.csum_data);
		break;
	case IXL_TX_DESC_CMD_IIPT_IPV6:
		len = M_CSUM_DATA_IPv6_IPHL(m->m_pkthdr.csum_data);
		break;
	default:
		len = 0;
	}
	cmd |= ((len >> 2) << IXL_TX_DESC_IPLEN_SHIFT);

	if (m->m_pkthdr.csum_flags &
	    (M_CSUM_TSOv4 | M_CSUM_TSOv6 | M_CSUM_TCPv4 | M_CSUM_TCPv6)) {
		len = sizeof(struct tcphdr);
		cmd |= IXL_TX_DESC_CMD_L4T_EOFT_TCP;
	} else if (m->m_pkthdr.csum_flags & (M_CSUM_UDPv4 | M_CSUM_UDPv6)) {
		len = sizeof(struct udphdr);
		cmd |= IXL_TX_DESC_CMD_L4T_EOFT_UDP;
	} else {
		len = 0;
	}
	cmd |= ((len >> 2) << IXL_TX_DESC_L4LEN_SHIFT);

	*cmd_txd |= cmd;
	return 0;
}

static void
iavf_tx_common_locked(struct ifnet *ifp, struct iavf_tx_ring *txr,
    bool is_transmit)
{
	struct iavf_softc *sc;
	struct ixl_tx_desc *ring, *txd;
	struct iavf_tx_map *txm;
	bus_dmamap_t map;
	struct mbuf *m;
	unsigned int prod, free, last, i;
	unsigned int mask;
	uint64_t cmd, cmd_txd;
	int post = 0;

	KASSERT(mutex_owned(&txr->txr_lock));

	sc = ifp->if_softc;

	if (!ISSET(ifp->if_flags, IFF_RUNNING)
	    || (!is_transmit && ISSET(ifp->if_flags, IFF_OACTIVE))) {
		if (!is_transmit)
			IFQ_PURGE(&ifp->if_snd);
		return;
	}

	prod = txr->txr_prod;
	free = txr->txr_cons;

	if (free <= prod)
		free += sc->sc_tx_ring_ndescs;
	free -= prod;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&txr->txr_mem),
	    0, IXL_DMA_LEN(&txr->txr_mem), BUS_DMASYNC_POSTWRITE);

	ring = IXL_DMA_KVA(&txr->txr_mem);
	mask = sc->sc_tx_ring_ndescs - 1;
	last = prod;
	cmd = 0;
	txd = NULL;

	for (;;) {
		if (free < IAVF_TX_PKT_DESCS) {
			if (!is_transmit)
				SET(ifp->if_flags, IFF_OACTIVE);
			break;
		}

		if (is_transmit)
			m = pcq_get(txr->txr_intrq);
		else
			IFQ_DEQUEUE(&ifp->if_snd, m);

		if (m == NULL)
			break;

		txm = &txr->txr_maps[prod];
		map = txm->txm_map;

		if (iavf_load_mbuf(sc->sc_dmat, map, &m, txr) != 0) {
			if_statinc(ifp, if_oerrors);
			m_freem(m);
			continue;
		}

		cmd_txd = 0;
		if (m->m_pkthdr.csum_flags & IAVF_CSUM_ALL_OFFLOAD) {
			iavf_tx_setup_offloads(m, &cmd_txd);
		}
		if (vlan_has_tag(m)) {
			uint16_t vtag;
			vtag = htole16(vlan_get_tag(m));
			cmd_txd |= IXL_TX_DESC_CMD_IL2TAG1 |
			    ((uint64_t)vtag << IXL_TX_DESC_L2TAG1_SHIFT);
		}

		bus_dmamap_sync(sc->sc_dmat, map, 0,
		    map->dm_mapsize, BUS_DMASYNC_PREWRITE);

		for (i = 0; i < (unsigned int)map->dm_nsegs; i++) {
			txd = &ring[prod];

			cmd = (uint64_t)map->dm_segs[i].ds_len <<
			    IXL_TX_DESC_BSIZE_SHIFT;
			cmd |= IXL_TX_DESC_DTYPE_DATA|IXL_TX_DESC_CMD_ICRC|
			    cmd_txd;

			txd->addr = htole64(map->dm_segs[i].ds_addr);
			txd->cmd = htole64(cmd);

			last = prod;
			prod++;
			prod &= mask;
		}

		cmd |= IXL_TX_DESC_CMD_EOP|IXL_TX_DESC_CMD_RS;
		txd->cmd = htole64(cmd);
		txm->txm_m = m;
		txm->txm_eop = last;

		bpf_mtap(ifp, m, BPF_D_OUT);
		free -= i;
		post = 1;
	}

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&txr->txr_mem),
	    0, IXL_DMA_LEN(&txr->txr_mem), BUS_DMASYNC_PREWRITE);

	if (post) {
		txr->txr_prod = prod;
		iavf_wr(sc, txr->txr_tail, prod);
		txr->txr_watchdog = IAVF_WATCHDOG_TICKS;
	}
}

static inline int
iavf_handle_queue_common(struct iavf_softc *sc, struct iavf_queue_pair *qp,
    u_int txlimit, struct evcnt *txevcnt,
    u_int rxlimit, struct evcnt *rxevcnt)
{
	struct iavf_tx_ring *txr;
	struct iavf_rx_ring *rxr;
	int txmore, rxmore;
	int rv;

	txr = qp->qp_txr;
	rxr = qp->qp_rxr;

	mutex_enter(&txr->txr_lock);
	txmore = iavf_txeof(sc, txr, txlimit, txevcnt);
	mutex_exit(&txr->txr_lock);

	mutex_enter(&rxr->rxr_lock);
	rxmore = iavf_rxeof(sc, rxr, rxlimit, rxevcnt);
	mutex_exit(&rxr->rxr_lock);

	rv = txmore | (rxmore << 1);

	return rv;
}

static void
iavf_sched_handle_queue(struct iavf_softc *sc, struct iavf_queue_pair *qp)
{

	if (qp->qp_workqueue)
		workqueue_enqueue(sc->sc_workq_txrx, &qp->qp_work, NULL);
	else
		softint_schedule(qp->qp_si);
}

static void
iavf_start(struct ifnet *ifp)
{
	struct iavf_softc *sc;
	struct iavf_tx_ring *txr;

	sc = ifp->if_softc;
	txr = sc->sc_qps[0].qp_txr;

	mutex_enter(&txr->txr_lock);
	iavf_tx_common_locked(ifp, txr, false);
	mutex_exit(&txr->txr_lock);

}

static inline unsigned int
iavf_select_txqueue(struct iavf_softc *sc, struct mbuf *m)
{
	u_int cpuid;

	cpuid = cpu_index(curcpu());

	return (unsigned int)(cpuid % sc->sc_nqueue_pairs);
}

static int
iavf_transmit(struct ifnet *ifp, struct mbuf *m)
{
	struct iavf_softc *sc;
	struct iavf_tx_ring *txr;
	unsigned int qid;

	sc = ifp->if_softc;
	qid = iavf_select_txqueue(sc, m);

	txr = sc->sc_qps[qid].qp_txr;

	if (__predict_false(!pcq_put(txr->txr_intrq, m))) {
		mutex_enter(&txr->txr_lock);
		txr->txr_pcqdrop.ev_count++;
		mutex_exit(&txr->txr_lock);

		m_freem(m);
		return ENOBUFS;
	}

	if (mutex_tryenter(&txr->txr_lock)) {
		iavf_tx_common_locked(ifp, txr, true);
		mutex_exit(&txr->txr_lock);
	} else {
		kpreempt_disable();
		softint_schedule(txr->txr_si);
		kpreempt_enable();
	}
	return 0;
}

static void
iavf_deferred_transmit(void *xtxr)
{
	struct iavf_tx_ring *txr;
	struct iavf_softc *sc;
	struct ifnet *ifp;

	txr = xtxr;
	sc = txr->txr_sc;
	ifp = &sc->sc_ec.ec_if;

	mutex_enter(&txr->txr_lock);
	txr->txr_transmitdef.ev_count++;
	if (pcq_peek(txr->txr_intrq) != NULL)
		iavf_tx_common_locked(ifp, txr, true);
	mutex_exit(&txr->txr_lock);
}

static void
iavf_txr_clean(struct iavf_softc *sc, struct iavf_tx_ring *txr)
{
	struct iavf_tx_map *maps, *txm;
	bus_dmamap_t map;
	unsigned int i;

	KASSERT(mutex_owned(&txr->txr_lock));

	maps = txr->txr_maps;
	for (i = 0; i < sc->sc_tx_ring_ndescs; i++) {
		txm = &maps[i];

		if (txm->txm_m == NULL)
			continue;

		map = txm->txm_map;
		bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, map);

		m_freem(txm->txm_m);
		txm->txm_m = NULL;
	}

	memset(IXL_DMA_KVA(&txr->txr_mem), 0, IXL_DMA_LEN(&txr->txr_mem));
	txr->txr_prod = txr->txr_cons = 0;
}

static int
iavf_intr(void *xsc)
{
	struct iavf_softc *sc = xsc;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct iavf_rx_ring *rxr;
	struct iavf_tx_ring *txr;
	uint32_t icr;
	unsigned int i;

	/* read I40E_VFINT_ICR_ENA1 to clear status */
	(void)iavf_rd(sc, I40E_VFINT_ICR0_ENA1);

	iavf_intr_enable(sc);
	icr = iavf_rd(sc, I40E_VFINT_ICR01);

	if (icr == IAVF_REG_VFR) {
		log(LOG_INFO, "%s: VF reset in progress\n",
		    ifp->if_xname);
		iavf_work_set(&sc->sc_reset_task, iavf_reset_start, sc);
		iavf_work_add(sc->sc_workq, &sc->sc_reset_task);
		return 1;
	}

	if (ISSET(icr, I40E_VFINT_ICR01_ADMINQ_MASK)) {
		mutex_enter(&sc->sc_adminq_lock);
		iavf_atq_done(sc);
		iavf_arq(sc);
		mutex_exit(&sc->sc_adminq_lock);
	}

	if (ISSET(icr, I40E_VFINT_ICR01_QUEUE_0_MASK)) {
		for (i = 0; i < sc->sc_nqueue_pairs; i++) {
			rxr = sc->sc_qps[i].qp_rxr;
			txr = sc->sc_qps[i].qp_txr;

			mutex_enter(&rxr->rxr_lock);
			while (iavf_rxeof(sc, rxr, UINT_MAX,
			    &rxr->rxr_intr) != 0) {
				/* do nothing */
			}
			mutex_exit(&rxr->rxr_lock);

			mutex_enter(&txr->txr_lock);
			while (iavf_txeof(sc, txr, UINT_MAX,
			    &txr->txr_intr) != 0) {
				/* do nothing */
			}
			mutex_exit(&txr->txr_lock);
		}
	}

	return 0;
}

static int
iavf_queue_intr(void *xqp)
{
	struct iavf_queue_pair *qp = xqp;
	struct iavf_tx_ring *txr;
	struct iavf_rx_ring *rxr;
	struct iavf_softc *sc;
	unsigned int qid;
	u_int txlimit, rxlimit;
	int more;

	txr = qp->qp_txr;
	rxr = qp->qp_rxr;
	sc = txr->txr_sc;
	qid = txr->txr_qid;

	txlimit = sc->sc_tx_intr_process_limit;
	rxlimit = sc->sc_rx_intr_process_limit;
	qp->qp_workqueue = sc->sc_txrx_workqueue;

	more = iavf_handle_queue_common(sc, qp,
	    txlimit, &txr->txr_intr, rxlimit, &rxr->rxr_intr);

	if (more != 0) {
		iavf_sched_handle_queue(sc, qp);
	} else {
		/* for ALTQ */
		if (txr->txr_qid == 0)
			if_schedule_deferred_start(&sc->sc_ec.ec_if);
		softint_schedule(txr->txr_si);

		iavf_queue_intr_enable(sc, qid);
	}

	return 0;
}

static void
iavf_handle_queue_wk(struct work *wk, void *xsc __unused)
{
	struct iavf_queue_pair *qp;

	qp = container_of(wk, struct iavf_queue_pair, qp_work);
	iavf_handle_queue(qp);
}

static void
iavf_handle_queue(void *xqp)
{
	struct iavf_queue_pair *qp = xqp;
	struct iavf_tx_ring *txr;
	struct iavf_rx_ring *rxr;
	struct iavf_softc *sc;
	unsigned int qid;
	u_int txlimit, rxlimit;
	int more;

	txr = qp->qp_txr;
	rxr = qp->qp_rxr;
	sc = txr->txr_sc;
	qid = txr->txr_qid;

	txlimit = sc->sc_tx_process_limit;
	rxlimit = sc->sc_rx_process_limit;

	more = iavf_handle_queue_common(sc, qp,
	    txlimit, &txr->txr_defer, rxlimit, &rxr->rxr_defer);

	if (more != 0)
		iavf_sched_handle_queue(sc, qp);
	else
		iavf_queue_intr_enable(sc, qid);
}

static void
iavf_tick(void *xsc)
{
	struct iavf_softc *sc;
	unsigned int i;
	int timedout;

	sc = xsc;
	timedout = 0;

	mutex_enter(&sc->sc_cfg_lock);

	if (sc->sc_resetting) {
		iavf_work_add(sc->sc_workq, &sc->sc_reset_task);
		mutex_exit(&sc->sc_cfg_lock);
		return;
	}

	iavf_get_stats(sc);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		timedout |= iavf_watchdog(sc->sc_qps[i].qp_txr);
	}

	if (timedout != 0) {
		iavf_work_add(sc->sc_workq, &sc->sc_wdto_task);
	} else {
		callout_schedule(&sc->sc_tick, IAVF_TICK_INTERVAL);
	}

	mutex_exit(&sc->sc_cfg_lock);
}

static void
iavf_tick_halt(void *unused __unused)
{

	/* do nothing */
}

static void
iavf_reset_request(void *xsc)
{
	struct iavf_softc *sc = xsc;

	iavf_reset_vf(sc);
	iavf_reset_start(sc);
}

static void
iavf_reset_start(void *xsc)
{
	struct iavf_softc *sc = xsc;
	struct ifnet *ifp = &sc->sc_ec.ec_if;

	mutex_enter(&sc->sc_cfg_lock);

	if (sc->sc_resetting)
		goto do_reset;

	sc->sc_resetting = true;
	if_link_state_change(ifp, LINK_STATE_DOWN);

	if (ISSET(ifp->if_flags, IFF_RUNNING)) {
		iavf_stop_locked(sc);
		sc->sc_reset_up = true;
	}

	memcpy(sc->sc_enaddr_reset, sc->sc_enaddr, ETHER_ADDR_LEN);

do_reset:
	iavf_work_set(&sc->sc_reset_task, iavf_reset, sc);

	mutex_exit(&sc->sc_cfg_lock);

	iavf_reset((void *)sc);
}

static void
iavf_reset(void *xsc)
{
	struct iavf_softc *sc = xsc;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ixl_aq_buf *aqb;
	bool realloc_qps, realloc_intrs;

	mutex_enter(&sc->sc_cfg_lock);

	mutex_enter(&sc->sc_adminq_lock);
	iavf_cleanup_admin_queue(sc);
	mutex_exit(&sc->sc_adminq_lock);

	sc->sc_major_ver = UINT_MAX;
	sc->sc_minor_ver = UINT_MAX;
	sc->sc_got_vf_resources = 0;
	sc->sc_got_irq_map = 0;

	aqb = iavf_aqb_get(sc, &sc->sc_atq_idle);
	if (aqb == NULL)
		goto failed;

	if (iavf_wait_active(sc) != 0) {
		log(LOG_WARNING, "%s: VF reset timed out\n",
		    ifp->if_xname);
		goto failed;
	}

	if (!iavf_arq_fill(sc)) {
		log(LOG_ERR, "%s: unable to fill arq descriptors\n",
		    ifp->if_xname);
		goto failed;
	}

	if (iavf_init_admin_queue(sc) != 0) {
		log(LOG_ERR, "%s: unable to initialize admin queue\n",
		    ifp->if_xname);
		goto failed;
	}

	if (iavf_get_version(sc, aqb) != 0) {
		log(LOG_ERR, "%s: unable to get VF interface version\n",
		    ifp->if_xname);
		goto failed;
	}

	if (iavf_get_vf_resources(sc, aqb) != 0) {
		log(LOG_ERR, "%s: timed out waiting for VF resources\n",
		    ifp->if_xname);
		goto failed;
	}

	if (sc->sc_nqps_alloc < iavf_calc_queue_pair_size(sc)) {
		realloc_qps = true;
	} else {
		realloc_qps = false;
	}

	if (sc->sc_nintrs < iavf_calc_msix_count(sc)) {
		realloc_intrs = true;
	} else {
		realloc_intrs = false;
	}

	if (realloc_qps || realloc_intrs)
		iavf_teardown_interrupts(sc);

	if (realloc_qps) {
		iavf_queue_pairs_free(sc);
		if (iavf_queue_pairs_alloc(sc) != 0) {
			log(LOG_ERR, "%s: failed to allocate queue pairs\n",
			    ifp->if_xname);
			goto failed;
		}
	}

	if (realloc_qps || realloc_intrs) {
		if (iavf_setup_interrupts(sc) != 0) {
			sc->sc_nintrs = 0;
			log(LOG_ERR, "%s: failed to allocate interrupts\n",
			    ifp->if_xname);
			goto failed;
		}
		log(LOG_INFO, "%s: reallocated queues\n", ifp->if_xname);
	}

	if (iavf_config_irq_map(sc, aqb) != 0) {
		log(LOG_ERR, "%s: timed out configuring IRQ map\n",
		   ifp->if_xname);
		goto failed;
	}

	mutex_enter(&sc->sc_adminq_lock);
	iavf_aqb_put_locked(&sc->sc_atq_idle, aqb);
	mutex_exit(&sc->sc_adminq_lock);

	iavf_reset_finish(sc);

	mutex_exit(&sc->sc_cfg_lock);
	return;

failed:
	mutex_enter(&sc->sc_adminq_lock);
	iavf_cleanup_admin_queue(sc);
	if (aqb != NULL) {
		iavf_aqb_put_locked(&sc->sc_atq_idle, aqb);
	}
	mutex_exit(&sc->sc_adminq_lock);
	callout_schedule(&sc->sc_tick, IAVF_TICK_INTERVAL);
	mutex_exit(&sc->sc_cfg_lock);
}

static void
iavf_reset_finish(struct iavf_softc *sc)
{
	struct ethercom *ec = &sc->sc_ec;
	struct ether_multi *enm;
	struct ether_multistep step;
	struct ifnet *ifp = &ec->ec_if;
	struct vlanid_list *vlanidp;
	uint8_t enaddr_prev[ETHER_ADDR_LEN], enaddr_next[ETHER_ADDR_LEN];

	KASSERT(mutex_owned(&sc->sc_cfg_lock));

	callout_stop(&sc->sc_tick);

	iavf_intr_enable(sc);

	if (!iavf_is_etheranyaddr(sc->sc_enaddr_added)) {
		iavf_eth_addr(sc, sc->sc_enaddr_added, IAVF_VC_OP_ADD_ETH_ADDR);
	}

	ETHER_LOCK(ec);
	if (!ISSET(ifp->if_flags, IFF_ALLMULTI)) {
		for (ETHER_FIRST_MULTI(step, ec, enm); enm != NULL;
		    ETHER_NEXT_MULTI(step, enm)) {
			iavf_add_multi(sc, enm->enm_addrlo, enm->enm_addrhi);
		}
	}

	SIMPLEQ_FOREACH(vlanidp, &ec->ec_vids, vid_list) {
		ETHER_UNLOCK(ec);
		iavf_config_vlan_id(sc, vlanidp->vid, IAVF_VC_OP_ADD_VLAN);
		ETHER_LOCK(ec);
	}
	ETHER_UNLOCK(ec);

	if (memcmp(sc->sc_enaddr, sc->sc_enaddr_reset, ETHER_ADDR_LEN) != 0) {
		memcpy(enaddr_prev, sc->sc_enaddr_reset, sizeof(enaddr_prev));
		memcpy(enaddr_next, sc->sc_enaddr, sizeof(enaddr_next));
		log(LOG_INFO, "%s: Ethernet address changed to %s\n",
		    ifp->if_xname, ether_sprintf(enaddr_next));

		mutex_exit(&sc->sc_cfg_lock);
		IFNET_LOCK(ifp);
		kpreempt_disable();
		/*XXX we need an API to change ethernet address. */
		iavf_replace_lla(ifp, enaddr_prev, enaddr_next);
		kpreempt_enable();
		IFNET_UNLOCK(ifp);
		mutex_enter(&sc->sc_cfg_lock);
	}

	sc->sc_resetting = false;

	if (sc->sc_reset_up) {
		iavf_init_locked(sc);
	}

	if (sc->sc_link_state != LINK_STATE_DOWN) {
		if_link_state_change(ifp, sc->sc_link_state);
	}

}

static int
iavf_dmamem_alloc(bus_dma_tag_t dmat, struct ixl_dmamem *ixm,
    bus_size_t size, bus_size_t align)
{
	ixm->ixm_size = size;

	if (bus_dmamap_create(dmat, ixm->ixm_size, 1,
	    ixm->ixm_size, 0,
	    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW,
	    &ixm->ixm_map) != 0)
		return 1;
	if (bus_dmamem_alloc(dmat, ixm->ixm_size,
	    align, 0, &ixm->ixm_seg, 1, &ixm->ixm_nsegs,
	    BUS_DMA_WAITOK) != 0)
		goto destroy;
	if (bus_dmamem_map(dmat, &ixm->ixm_seg, ixm->ixm_nsegs,
	    ixm->ixm_size, &ixm->ixm_kva, BUS_DMA_WAITOK) != 0)
		goto free;
	if (bus_dmamap_load(dmat, ixm->ixm_map, ixm->ixm_kva,
	    ixm->ixm_size, NULL, BUS_DMA_WAITOK) != 0)
		goto unmap;

	memset(ixm->ixm_kva, 0, ixm->ixm_size);

	return 0;
unmap:
	bus_dmamem_unmap(dmat, ixm->ixm_kva, ixm->ixm_size);
free:
	bus_dmamem_free(dmat, &ixm->ixm_seg, 1);
destroy:
	bus_dmamap_destroy(dmat, ixm->ixm_map);
	return 1;
}

static void
iavf_dmamem_free(bus_dma_tag_t dmat, struct ixl_dmamem *ixm)
{

	bus_dmamap_unload(dmat, ixm->ixm_map);
	bus_dmamem_unmap(dmat, ixm->ixm_kva, ixm->ixm_size);
	bus_dmamem_free(dmat, &ixm->ixm_seg, 1);
	bus_dmamap_destroy(dmat, ixm->ixm_map);
}

static struct ixl_aq_buf *
iavf_aqb_alloc(bus_dma_tag_t dmat, size_t buflen)
{
	struct ixl_aq_buf *aqb;

	aqb = kmem_alloc(sizeof(*aqb), KM_NOSLEEP);
	if (aqb == NULL)
		return NULL;

	aqb->aqb_size = buflen;

	if (bus_dmamap_create(dmat, aqb->aqb_size, 1,
	    aqb->aqb_size, 0,
	    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW, &aqb->aqb_map) != 0)
		goto free;
	if (bus_dmamem_alloc(dmat, aqb->aqb_size,
	    IAVF_AQ_ALIGN, 0, &aqb->aqb_seg, 1, &aqb->aqb_nsegs,
	    BUS_DMA_WAITOK) != 0)
		goto destroy;
	if (bus_dmamem_map(dmat, &aqb->aqb_seg, aqb->aqb_nsegs,
	    aqb->aqb_size, &aqb->aqb_data, BUS_DMA_WAITOK) != 0)
		goto dma_free;
	if (bus_dmamap_load(dmat, aqb->aqb_map, aqb->aqb_data,
	    aqb->aqb_size, NULL, BUS_DMA_WAITOK) != 0)
		goto unmap;

	return aqb;
unmap:
	bus_dmamem_unmap(dmat, aqb->aqb_data, aqb->aqb_size);
dma_free:
	bus_dmamem_free(dmat, &aqb->aqb_seg, 1);
destroy:
	bus_dmamap_destroy(dmat, aqb->aqb_map);
free:
	kmem_free(aqb, sizeof(*aqb));

	return NULL;
}

static void
iavf_aqb_free(bus_dma_tag_t dmat, struct ixl_aq_buf *aqb)
{

	bus_dmamap_unload(dmat, aqb->aqb_map);
	bus_dmamem_unmap(dmat, aqb->aqb_data, aqb->aqb_size);
	bus_dmamem_free(dmat, &aqb->aqb_seg, 1);
	bus_dmamap_destroy(dmat, aqb->aqb_map);
	kmem_free(aqb, sizeof(*aqb));
}

static struct ixl_aq_buf *
iavf_aqb_get_locked(struct ixl_aq_bufs *q)
{
	struct ixl_aq_buf *aqb;

	aqb = SIMPLEQ_FIRST(q);
	if (aqb != NULL) {
		SIMPLEQ_REMOVE(q, aqb, ixl_aq_buf, aqb_entry);
	}

	return aqb;
}

static struct ixl_aq_buf *
iavf_aqb_get(struct iavf_softc *sc, struct ixl_aq_bufs *q)
{
	struct ixl_aq_buf *aqb;

	if (q != NULL) {
		mutex_enter(&sc->sc_adminq_lock);
		aqb = iavf_aqb_get_locked(q);
		mutex_exit(&sc->sc_adminq_lock);
	} else {
		aqb = NULL;
	}

	if (aqb == NULL) {
		aqb = iavf_aqb_alloc(sc->sc_dmat, IAVF_AQ_BUFLEN);
	}

	return aqb;
}

static void
iavf_aqb_put_locked(struct ixl_aq_bufs *q, struct ixl_aq_buf *aqb)
{

	SIMPLEQ_INSERT_TAIL(q, aqb, aqb_entry);
}

static void
iavf_aqb_clean(struct ixl_aq_bufs *q, bus_dma_tag_t dmat)
{
	struct ixl_aq_buf *aqb;

	while ((aqb = SIMPLEQ_FIRST(q)) != NULL) {
		SIMPLEQ_REMOVE(q, aqb, ixl_aq_buf, aqb_entry);
		iavf_aqb_free(dmat, aqb);
	}
}

static const char *
iavf_aq_vc_opcode_str(const struct ixl_aq_desc *iaq)
{

	switch (iavf_aq_vc_get_opcode(iaq)) {
	case IAVF_VC_OP_VERSION:
		return "GET_VERSION";
	case IAVF_VC_OP_RESET_VF:
		return "RESET_VF";
	case IAVF_VC_OP_GET_VF_RESOURCES:
		return "GET_VF_RESOURCES";
	case IAVF_VC_OP_CONFIG_TX_QUEUE:
		return "CONFIG_TX_QUEUE";
	case IAVF_VC_OP_CONFIG_RX_QUEUE:
		return "CONFIG_RX_QUEUE";
	case IAVF_VC_OP_CONFIG_VSI_QUEUES:
		return "CONFIG_VSI_QUEUES";
	case IAVF_VC_OP_CONFIG_IRQ_MAP:
		return "CONFIG_IRQ_MAP";
	case IAVF_VC_OP_ENABLE_QUEUES:
		return "ENABLE_QUEUES";
	case IAVF_VC_OP_DISABLE_QUEUES:
		return "DISABLE_QUEUES";
	case IAVF_VC_OP_ADD_ETH_ADDR:
		return "ADD_ETH_ADDR";
	case IAVF_VC_OP_DEL_ETH_ADDR:
		return "DEL_ETH_ADDR";
	case IAVF_VC_OP_CONFIG_PROMISC:
		return "CONFIG_PROMISC";
	case IAVF_VC_OP_GET_STATS:
		return "GET_STATS";
	case IAVF_VC_OP_EVENT:
		return "EVENT";
	case IAVF_VC_OP_CONFIG_RSS_KEY:
		return "CONFIG_RSS_KEY";
	case IAVF_VC_OP_CONFIG_RSS_LUT:
		return "CONFIG_RSS_LUT";
	case IAVF_VC_OP_GET_RSS_HENA_CAPS:
		return "GET_RS_HENA_CAPS";
	case IAVF_VC_OP_SET_RSS_HENA:
		return "SET_RSS_HENA";
	case IAVF_VC_OP_ENABLE_VLAN_STRIP:
		return "ENABLE_VLAN_STRIPPING";
	case IAVF_VC_OP_DISABLE_VLAN_STRIP:
		return "DISABLE_VLAN_STRIPPING";
	case IAVF_VC_OP_REQUEST_QUEUES:
		return "REQUEST_QUEUES";
	}

	return "unknown";
}

static void
iavf_aq_dump(const struct iavf_softc *sc, const struct ixl_aq_desc *iaq,
    const char *msg)
{
	char	 buf[512];
	size_t	 len;

	len = sizeof(buf);
	buf[--len] = '\0';

	device_printf(sc->sc_dev, "%s\n", msg);
	snprintb(buf, len, IXL_AQ_FLAGS_FMT, le16toh(iaq->iaq_flags));
	device_printf(sc->sc_dev, "flags %s opcode %04x\n",
	    buf, le16toh(iaq->iaq_opcode));
	device_printf(sc->sc_dev, "datalen %u retval %u\n",
	    le16toh(iaq->iaq_datalen), le16toh(iaq->iaq_retval));
	device_printf(sc->sc_dev, "vc-opcode %u (%s)\n",
	    iavf_aq_vc_get_opcode(iaq),
	    iavf_aq_vc_opcode_str(iaq));
	device_printf(sc->sc_dev, "vc-retval %u\n",
	    iavf_aq_vc_get_retval(iaq));
	device_printf(sc->sc_dev, "cookie %016" PRIx64 "\n", iaq->iaq_cookie);
	device_printf(sc->sc_dev, "%08x %08x %08x %08x\n",
	    le32toh(iaq->iaq_param[0]), le32toh(iaq->iaq_param[1]),
	    le32toh(iaq->iaq_param[2]), le32toh(iaq->iaq_param[3]));
}

static int
iavf_arq_fill(struct iavf_softc *sc)
{
	struct ixl_aq_buf *aqb;
	struct ixl_aq_desc *arq, *iaq;
	unsigned int prod = sc->sc_arq_prod;
	unsigned int n;
	int filled;

	n = ixl_rxr_unrefreshed(sc->sc_arq_prod, sc->sc_arq_cons,
	    IAVF_AQ_NUM);

	if (__predict_false(n <= 0))
		return 0;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
	    0, IXL_DMA_LEN(&sc->sc_arq),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	arq = IXL_DMA_KVA(&sc->sc_arq);

	do {
		iaq = &arq[prod];

		if (ixl_aq_has_dva(iaq)) {
			/* already filled */
			break;
		}

		aqb = iavf_aqb_get_locked(&sc->sc_arq_idle);
		if (aqb == NULL)
			break;

		memset(aqb->aqb_data, 0, aqb->aqb_size);

		bus_dmamap_sync(sc->sc_dmat, aqb->aqb_map, 0,
		    aqb->aqb_size, BUS_DMASYNC_PREREAD);

		iaq->iaq_flags = htole16(IXL_AQ_BUF |
		    (aqb->aqb_size > I40E_AQ_LARGE_BUF ?
		    IXL_AQ_LB : 0));
		iaq->iaq_opcode = 0;
		iaq->iaq_datalen = htole16(aqb->aqb_size);
		iaq->iaq_retval = 0;
		iaq->iaq_cookie = 0;
		iaq->iaq_param[0] = 0;
		iaq->iaq_param[1] = 0;
		ixl_aq_dva(iaq, IXL_AQB_DVA(aqb));
		iavf_aqb_put_locked(&sc->sc_arq_live, aqb);

		prod++;
		prod &= IAVF_AQ_MASK;
		filled = 1;
	} while (--n);

	sc->sc_arq_prod = prod;

	if (filled) {
		bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
		    0, IXL_DMA_LEN(&sc->sc_arq),
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
		iavf_wr(sc, sc->sc_aq_regs->arq_tail, sc->sc_arq_prod);
	}

	return filled;
}

static int
iavf_arq_wait(struct iavf_softc *sc, uint32_t opcode)
{
	int error;

	KASSERT(mutex_owned(&sc->sc_adminq_lock));

	while ((error = cv_timedwait(&sc->sc_adminq_cv,
	    &sc->sc_adminq_lock, mstohz(IAVF_EXEC_TIMEOUT))) == 0) {
		if (opcode == sc->sc_arq_opcode)
			break;
	}

	if (error != 0 &&
	    atomic_load_relaxed(&sc->sc_debuglevel) >= 2)
		device_printf(sc->sc_dev, "cv_timedwait error=%d\n", error);

	return error;
}

static void
iavf_arq_refill(void *xsc)
{
	struct iavf_softc *sc = xsc;
	struct ixl_aq_bufs aqbs;
	struct ixl_aq_buf *aqb;
	unsigned int n, i;

	mutex_enter(&sc->sc_adminq_lock);
	iavf_arq_fill(sc);
	n = ixl_rxr_unrefreshed(sc->sc_arq_prod, sc->sc_arq_cons,
	    IAVF_AQ_NUM);
	mutex_exit(&sc->sc_adminq_lock);

	if (n == 0)
		return;

	if (atomic_load_relaxed(&sc->sc_debuglevel) >= 1)
		device_printf(sc->sc_dev, "Allocate %d bufs for arq\n", n);

	SIMPLEQ_INIT(&aqbs);
	for (i = 0; i < n; i++) {
		aqb = iavf_aqb_get(sc, NULL);
		if (aqb == NULL)
			continue;
		SIMPLEQ_INSERT_TAIL(&aqbs, aqb, aqb_entry);
	}

	mutex_enter(&sc->sc_adminq_lock);
	while ((aqb = SIMPLEQ_FIRST(&aqbs)) != NULL) {
		SIMPLEQ_REMOVE(&aqbs, aqb, ixl_aq_buf, aqb_entry);
		iavf_aqb_put_locked(&sc->sc_arq_idle, aqb);
	}
	iavf_arq_fill(sc);
	mutex_exit(&sc->sc_adminq_lock);
}

static uint32_t
iavf_process_arq(struct iavf_softc *sc, struct ixl_aq_desc *iaq,
    struct ixl_aq_buf *aqb)
{
	uint32_t vc_retval, vc_opcode;
	int dbg;

	dbg = atomic_load_relaxed(&sc->sc_debuglevel);
	if (dbg >= 3)
		iavf_aq_dump(sc, iaq, "arq proc");

	if (dbg >= 2) {
		vc_retval = iavf_aq_vc_get_retval(iaq);
		if (vc_retval != IAVF_VC_RC_SUCCESS) {
			device_printf(sc->sc_dev, "%s failed=%d(arq)\n",
			    iavf_aq_vc_opcode_str(iaq), vc_retval);
		}
	}

	vc_opcode = iavf_aq_vc_get_opcode(iaq);
	switch (vc_opcode) {
	case IAVF_VC_OP_VERSION:
		iavf_process_version(sc, iaq, aqb);
		break;
	case IAVF_VC_OP_GET_VF_RESOURCES:
		iavf_process_vf_resources(sc, iaq, aqb);
		break;
	case IAVF_VC_OP_CONFIG_IRQ_MAP:
		iavf_process_irq_map(sc, iaq);
		break;
	case IAVF_VC_OP_EVENT:
		iavf_process_vc_event(sc, iaq, aqb);
		break;
	case IAVF_VC_OP_GET_STATS:
		iavf_process_stats(sc, iaq, aqb);
		break;
	case IAVF_VC_OP_REQUEST_QUEUES:
		iavf_process_req_queues(sc, iaq, aqb);
		break;
	}

	return vc_opcode;
}

static int
iavf_arq_poll(struct iavf_softc *sc, uint32_t wait_opcode, int retry)
{
	struct ixl_aq_desc *arq, *iaq;
	struct ixl_aq_buf *aqb;
	unsigned int cons = sc->sc_arq_cons;
	unsigned int prod;
	uint32_t vc_opcode;
	bool received;
	int i;

	for (i = 0, received = false; i < retry && !received; i++) {
		prod = iavf_rd(sc, sc->sc_aq_regs->arq_head);
		prod &= sc->sc_aq_regs->arq_head_mask;

		if (prod == cons) {
			delaymsec(1);
			continue;
		}

		if (prod >= IAVF_AQ_NUM) {
			return EIO;
		}

		arq = IXL_DMA_KVA(&sc->sc_arq);

		bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
		    0, IXL_DMA_LEN(&sc->sc_arq),
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		do {
			iaq = &arq[cons];
			aqb = iavf_aqb_get_locked(&sc->sc_arq_live);
			KASSERT(aqb != NULL);

			bus_dmamap_sync(sc->sc_dmat, aqb->aqb_map, 0,
			    IAVF_AQ_BUFLEN, BUS_DMASYNC_POSTREAD);

			vc_opcode = iavf_process_arq(sc, iaq, aqb);

			if (vc_opcode == wait_opcode)
				received = true;

			memset(iaq, 0, sizeof(*iaq));
			iavf_aqb_put_locked(&sc->sc_arq_idle, aqb);

			cons++;
			cons &= IAVF_AQ_MASK;

		} while (cons != prod);

		bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
		    0, IXL_DMA_LEN(&sc->sc_arq),
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		sc->sc_arq_cons = cons;
		iavf_arq_fill(sc);

	}

	if (!received)
		return ETIMEDOUT;

	return 0;
}

static int
iavf_arq(struct iavf_softc *sc)
{
	struct ixl_aq_desc *arq, *iaq;
	struct ixl_aq_buf *aqb;
	unsigned int cons = sc->sc_arq_cons;
	unsigned int prod;
	uint32_t vc_opcode;

	KASSERT(mutex_owned(&sc->sc_adminq_lock));

	prod = iavf_rd(sc, sc->sc_aq_regs->arq_head);
	prod &= sc->sc_aq_regs->arq_head_mask;

	/* broken value at resetting */
	if (prod >= IAVF_AQ_NUM) {
		iavf_work_set(&sc->sc_reset_task, iavf_reset_start, sc);
		iavf_work_add(sc->sc_workq, &sc->sc_reset_task);
		return 0;
	}

	if (cons == prod)
		return 0;

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_arq),
	    0, IXL_DMA_LEN(&sc->sc_arq),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	arq = IXL_DMA_KVA(&sc->sc_arq);

	do {
		iaq = &arq[cons];
		aqb = iavf_aqb_get_locked(&sc->sc_arq_live);

		KASSERT(aqb != NULL);

		bus_dmamap_sync(sc->sc_dmat, aqb->aqb_map, 0, IAVF_AQ_BUFLEN,
		    BUS_DMASYNC_POSTREAD);

		vc_opcode = iavf_process_arq(sc, iaq, aqb);

		switch (vc_opcode) {
		case IAVF_VC_OP_CONFIG_TX_QUEUE:
		case IAVF_VC_OP_CONFIG_RX_QUEUE:
		case IAVF_VC_OP_CONFIG_VSI_QUEUES:
		case IAVF_VC_OP_ENABLE_QUEUES:
		case IAVF_VC_OP_DISABLE_QUEUES:
		case IAVF_VC_OP_GET_RSS_HENA_CAPS:
		case IAVF_VC_OP_SET_RSS_HENA:
		case IAVF_VC_OP_ADD_ETH_ADDR:
		case IAVF_VC_OP_DEL_ETH_ADDR:
		case IAVF_VC_OP_CONFIG_PROMISC:
		case IAVF_VC_OP_ADD_VLAN:
		case IAVF_VC_OP_DEL_VLAN:
		case IAVF_VC_OP_ENABLE_VLAN_STRIP:
		case IAVF_VC_OP_DISABLE_VLAN_STRIP:
		case IAVF_VC_OP_CONFIG_RSS_KEY:
		case IAVF_VC_OP_CONFIG_RSS_LUT:
			sc->sc_arq_retval = iavf_aq_vc_get_retval(iaq);
			sc->sc_arq_opcode = vc_opcode;
			cv_signal(&sc->sc_adminq_cv);
			break;
		}

		memset(iaq, 0, sizeof(*iaq));
		iavf_aqb_put_locked(&sc->sc_arq_idle, aqb);

		cons++;
		cons &= IAVF_AQ_MASK;
	} while (cons != prod);

	sc->sc_arq_cons = cons;
	iavf_work_add(sc->sc_workq, &sc->sc_arq_refill);

	return 1;
}

static int
iavf_atq_post(struct iavf_softc *sc, struct ixl_aq_desc *iaq,
    struct ixl_aq_buf *aqb)
{
	struct ixl_aq_desc *atq, *slot;
	unsigned int prod;

	atq = IXL_DMA_KVA(&sc->sc_atq);
	prod = sc->sc_atq_prod;
	slot = &atq[prod];

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq), BUS_DMASYNC_POSTWRITE);

	*slot = *iaq;
	slot->iaq_flags |= htole16(IXL_AQ_SI);
	if (aqb != NULL) {
		ixl_aq_dva(slot, IXL_AQB_DVA(aqb));
		bus_dmamap_sync(sc->sc_dmat, IXL_AQB_MAP(aqb),
		    0, IXL_AQB_LEN(aqb), BUS_DMASYNC_PREWRITE);
		iavf_aqb_put_locked(&sc->sc_atq_live, aqb);
	} else {
		ixl_aq_dva(slot, (bus_addr_t)0);
	}

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq), BUS_DMASYNC_PREWRITE);

	if (atomic_load_relaxed(&sc->sc_debuglevel) >= 3)
		iavf_aq_dump(sc, slot, "post");

	prod++;
	prod &= IAVF_AQ_MASK;
	sc->sc_atq_prod = prod;
	iavf_wr(sc, sc->sc_aq_regs->atq_tail, prod);
	return prod;
}

static int
iavf_atq_poll(struct iavf_softc *sc, unsigned int tm)
{
	struct ixl_aq_desc *atq, *slot;
	struct ixl_aq_desc iaq;
	unsigned int prod;
	unsigned int t;
	int dbg;

	dbg = atomic_load_relaxed(&sc->sc_debuglevel);
	atq = IXL_DMA_KVA(&sc->sc_atq);
	prod = sc->sc_atq_prod;
	slot = &atq[prod];
	t = 0;

	while (iavf_rd(sc, sc->sc_aq_regs->atq_head) != prod) {
		delaymsec(1);

		if (t++ > tm) {
			if (dbg >= 2) {
				device_printf(sc->sc_dev,
				    "atq timedout\n");
			}
			return ETIMEDOUT;
		}
	}

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq), BUS_DMASYNC_POSTREAD);
	iaq = *slot;
	memset(slot, 0, sizeof(*slot));
	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq), BUS_DMASYNC_PREREAD);

	if (iaq.iaq_retval != htole16(IXL_AQ_RC_OK)) {
		if (dbg >= 2) {
			device_printf(sc->sc_dev,
			    "atq retcode=0x%04x\n", le16toh(iaq.iaq_retval));
		}
		return EIO;
	}

	return 0;
}

static void
iavf_atq_done(struct iavf_softc *sc)
{
	struct ixl_aq_desc *atq, *slot;
	struct ixl_aq_buf *aqb;
	unsigned int cons;
	unsigned int prod;

	KASSERT(mutex_owned(&sc->sc_adminq_lock));

	prod = sc->sc_atq_prod;
	cons = sc->sc_atq_cons;

	if (prod == cons)
		return;

	atq = IXL_DMA_KVA(&sc->sc_atq);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq),
	    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

	do {
		slot = &atq[cons];
		if (!ISSET(slot->iaq_flags, htole16(IXL_AQ_DD)))
			break;

		if (ixl_aq_has_dva(slot) &&
		    (aqb = iavf_aqb_get_locked(&sc->sc_atq_live)) != NULL) {
			bus_dmamap_sync(sc->sc_dmat, IXL_AQB_MAP(aqb),
			    0, IXL_AQB_LEN(aqb), BUS_DMASYNC_POSTWRITE);
			iavf_aqb_put_locked(&sc->sc_atq_idle, aqb);
		}

		memset(slot, 0, sizeof(*slot));

		cons++;
		cons &= IAVF_AQ_MASK;
	} while (cons != prod);

	bus_dmamap_sync(sc->sc_dmat, IXL_DMA_MAP(&sc->sc_atq),
	    0, IXL_DMA_LEN(&sc->sc_atq),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	sc->sc_atq_cons = cons;
}

static int
iavf_adminq_poll(struct iavf_softc *sc, struct ixl_aq_desc *iaq,
    struct ixl_aq_buf *aqb, int retry)
{
	int error;

	mutex_enter(&sc->sc_adminq_lock);
	error = iavf_adminq_poll_locked(sc, iaq, aqb, retry);
	mutex_exit(&sc->sc_adminq_lock);

	return error;
}

static int
iavf_adminq_poll_locked(struct iavf_softc *sc,
    struct ixl_aq_desc *iaq, struct ixl_aq_buf *aqb, int retry)
{
	uint32_t opcode;
	int error;

	KASSERT(!sc->sc_attached || mutex_owned(&sc->sc_adminq_lock));

	opcode = iavf_aq_vc_get_opcode(iaq);

	iavf_atq_post(sc, iaq, aqb);

	error = iavf_atq_poll(sc, retry);

	/*
	 * collect the aqb used in the current command and
	 * added to sc_atq_live at iavf_atq_post(),
	 * whether or not the command succeeded.
	*/
	if (aqb != NULL) {
		(void)iavf_aqb_get_locked(&sc->sc_atq_live);
		bus_dmamap_sync(sc->sc_dmat, IXL_AQB_MAP(aqb),
		    0, IXL_AQB_LEN(aqb), BUS_DMASYNC_POSTWRITE);
	}

	if (error)
		return error;

	error = iavf_arq_poll(sc, opcode, retry);

	if (error != 0 &&
	    atomic_load_relaxed(&sc->sc_debuglevel) >= 1) {
		device_printf(sc->sc_dev, "%s failed=%d(polling)\n",
		    iavf_aq_vc_opcode_str(iaq), error);
	}

	return error;
}

static int
iavf_adminq_exec(struct iavf_softc *sc, struct ixl_aq_desc *iaq,
    struct ixl_aq_buf *aqb)
{
	int error;
	uint32_t opcode;

	opcode = iavf_aq_vc_get_opcode(iaq);

	mutex_enter(&sc->sc_adminq_lock);
	iavf_atq_post(sc, iaq, aqb);

	error = iavf_arq_wait(sc, opcode);
	if (error == 0) {
		error = sc->sc_arq_retval;
		if (error != IAVF_VC_RC_SUCCESS &&
		    atomic_load_relaxed(&sc->sc_debuglevel) >= 1) {
			device_printf(sc->sc_dev, "%s failed=%d\n",
			    iavf_aq_vc_opcode_str(iaq), error);
		}
	}

	mutex_exit(&sc->sc_adminq_lock);
	return error;
}

static void
iavf_process_version(struct iavf_softc *sc, struct ixl_aq_desc *iaq,
   struct ixl_aq_buf *aqb)
{
	struct iavf_vc_version_info *ver;

	ver = (struct iavf_vc_version_info *)aqb->aqb_data;
	sc->sc_major_ver = le32toh(ver->major);
	sc->sc_minor_ver = le32toh(ver->minor);
}

static void
iavf_process_vf_resources(struct iavf_softc *sc, struct ixl_aq_desc *iaq,
    struct ixl_aq_buf *aqb)
{
	struct iavf_vc_vf_resource *vf_res;
	struct iavf_vc_vsi_resource *vsi_res;
	uint8_t *enaddr;
	int mtu, dbg;
	char buf[512];

	dbg = atomic_load_relaxed(&sc->sc_debuglevel);
	sc->sc_got_vf_resources = 1;

	vf_res = aqb->aqb_data;
	sc->sc_max_vectors = le16toh(vf_res->max_vectors);
	if (le16toh(vf_res->num_vsis) == 0) {
		if (dbg >= 1) {
			device_printf(sc->sc_dev, "no vsi available\n");
		}
		return;
	}
	sc->sc_vf_cap = le32toh(vf_res->offload_flags);
	if (dbg >= 2) {
		snprintb(buf, sizeof(buf),
		    IAVF_VC_OFFLOAD_FMT, sc->sc_vf_cap);
		device_printf(sc->sc_dev, "VF cap=%s\n", buf);
	}

	mtu = le16toh(vf_res->max_mtu);
	if (IAVF_MIN_MTU < mtu && mtu < IAVF_MAX_MTU) {
		sc->sc_max_mtu = MIN(IAVF_MAX_MTU, mtu);
	}

	vsi_res = &vf_res->vsi_res[0];
	sc->sc_vsi_id = le16toh(vsi_res->vsi_id);
	sc->sc_vf_id = le32toh(iaq->iaq_param[0]);
	sc->sc_qset_handle = le16toh(vsi_res->qset_handle);
	sc->sc_nqps_vsi = le16toh(vsi_res->num_queue_pairs);
	if (!iavf_is_etheranyaddr(vsi_res->default_mac)) {
		enaddr = vsi_res->default_mac;
	} else {
		enaddr = sc->sc_enaddr_fake;
	}
	memcpy(sc->sc_enaddr, enaddr, ETHER_ADDR_LEN);
}

static void
iavf_process_irq_map(struct iavf_softc *sc, struct ixl_aq_desc *iaq)
{
	uint32_t retval;

	retval = iavf_aq_vc_get_retval(iaq);
	if (retval != IAVF_VC_RC_SUCCESS) {
		return;
	}

	sc->sc_got_irq_map = 1;
}

static void
iavf_process_vc_event(struct iavf_softc *sc, struct ixl_aq_desc *iaq,
    struct ixl_aq_buf *aqb)
{
	struct iavf_vc_pf_event *event;
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	const struct iavf_link_speed *speed;
	int link;

	event = aqb->aqb_data;
	switch (event->event) {
	case IAVF_VC_EVENT_LINK_CHANGE:
		sc->sc_media_status = IFM_AVALID;
		sc->sc_media_active = IFM_ETHER;
		link = LINK_STATE_DOWN;
		if (event->link_status) {
			link = LINK_STATE_UP;
			sc->sc_media_status |= IFM_ACTIVE;
			sc->sc_media_active |= IFM_FDX;

			ifp->if_baudrate = 0;
			speed = iavf_find_link_speed(sc, event->link_speed);
			if (speed != NULL) {
				sc->sc_media_active |= speed->media;
				ifp->if_baudrate = speed->baudrate;
			}
		}

		if (sc->sc_link_state != link) {
			sc->sc_link_state = link;
			if (sc->sc_attached) {
				if_link_state_change(ifp, link);
			}
		}
		break;
	case IAVF_VC_EVENT_RESET_IMPENDING:
		log(LOG_INFO, "%s: Reset warning received from the PF\n",
		    ifp->if_xname);
		iavf_work_set(&sc->sc_reset_task, iavf_reset_request, sc);
		iavf_work_add(sc->sc_workq, &sc->sc_reset_task);
		break;
	}
}

static void
iavf_process_stats(struct iavf_softc *sc, struct ixl_aq_desc *iaq,
    struct ixl_aq_buf *aqb)
{
	struct iavf_stat_counters *isc;
	struct i40e_eth_stats *st;

	KASSERT(mutex_owned(&sc->sc_adminq_lock));

	st = aqb->aqb_data;
	isc = &sc->sc_stat_counters;

	isc->isc_rx_bytes.ev_count = st->rx_bytes;
	isc->isc_rx_unicast.ev_count = st->rx_unicast;
	isc->isc_rx_multicast.ev_count = st->rx_multicast;
	isc->isc_rx_broadcast.ev_count = st->rx_broadcast;
	isc->isc_rx_discards.ev_count = st->rx_discards;
	isc->isc_rx_unknown_protocol.ev_count = st->rx_unknown_protocol;

	isc->isc_tx_bytes.ev_count = st->tx_bytes;
	isc->isc_tx_unicast.ev_count = st->tx_unicast;
	isc->isc_tx_multicast.ev_count = st->tx_multicast;
	isc->isc_tx_broadcast.ev_count = st->tx_broadcast;
	isc->isc_tx_discards.ev_count = st->tx_discards;
	isc->isc_tx_errors.ev_count = st->tx_errors;
}

static void
iavf_process_req_queues(struct iavf_softc *sc, struct ixl_aq_desc *iaq,
    struct ixl_aq_buf *aqb)
{
	struct iavf_vc_res_request *req;
	struct ifnet *ifp;
	uint32_t vc_retval;

	ifp = &sc->sc_ec.ec_if;
	req = aqb->aqb_data;

	vc_retval = iavf_aq_vc_get_retval(iaq);
	if (vc_retval != IAVF_VC_RC_SUCCESS) {
		return;
	}

	if (sc->sc_nqps_req < req->num_queue_pairs) {
		log(LOG_INFO,
		    "%s: requested %d queues, but only %d left.\n",
		    ifp->if_xname,
		    sc->sc_nqps_req, req->num_queue_pairs);
	}

	if (sc->sc_nqps_vsi < req->num_queue_pairs) {
		if (!sc->sc_req_queues_retried) {
			/* req->num_queue_pairs indicates max qps */
			sc->sc_nqps_req = req->num_queue_pairs;

			sc->sc_req_queues_retried = true;
			iavf_work_add(sc->sc_workq, &sc->sc_req_queues_task);
		}
	}
}

static int
iavf_get_version(struct iavf_softc *sc, struct ixl_aq_buf *aqb)
{
	struct ixl_aq_desc iaq;
	struct iavf_vc_version_info *ver;
	int error;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_VERSION);
	iaq.iaq_datalen = htole16(sizeof(struct iavf_vc_version_info));

	ver = IXL_AQB_KVA(aqb);
	ver->major = htole32(IAVF_VF_MAJOR);
	ver->minor = htole32(IAVF_VF_MINOR);

	sc->sc_major_ver = UINT_MAX;
	sc->sc_minor_ver = UINT_MAX;

	if (sc->sc_attached) {
		error = iavf_adminq_poll(sc, &iaq, aqb, 250);
	} else {
		error = iavf_adminq_poll_locked(sc, &iaq, aqb, 250);
	}

	if (error)
		return -1;

	return 0;
}

static int
iavf_get_vf_resources(struct iavf_softc *sc, struct ixl_aq_buf *aqb)
{
	struct ixl_aq_desc iaq;
	uint32_t *cap, cap0;
	int error;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_GET_VF_RESOURCES);

	if (sc->sc_major_ver > 0) {
		cap0 = IAVF_VC_OFFLOAD_L2 |
		    IAVF_VC_OFFLOAD_VLAN |
		    IAVF_VC_OFFLOAD_RSS_PF |
		    IAVF_VC_OFFLOAD_REQ_QUEUES;

		cap = IXL_AQB_KVA(aqb);
		*cap = htole32(cap0);
		iaq.iaq_datalen = htole16(sizeof(*cap));
	}

	sc->sc_got_vf_resources = 0;
	if (sc->sc_attached) {
		error = iavf_adminq_poll(sc, &iaq, aqb, 250);
	} else {
		error = iavf_adminq_poll_locked(sc, &iaq, aqb, 250);
	}

	if (error)
		return -1;
	return 0;
}

static int
iavf_get_stats(struct iavf_softc *sc)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_buf *aqb;
	struct iavf_vc_queue_select *qsel;
	int error;

	mutex_enter(&sc->sc_adminq_lock);
	aqb = iavf_aqb_get_locked(&sc->sc_atq_idle);
	mutex_exit(&sc->sc_adminq_lock);

	if (aqb == NULL)
		return ENOMEM;

	qsel = IXL_AQB_KVA(aqb);
	memset(qsel, 0, sizeof(*qsel));
	qsel->vsi_id = htole16(sc->sc_vsi_id);

	memset(&iaq, 0, sizeof(iaq));

	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_GET_STATS);
	iaq.iaq_datalen = htole16(sizeof(*qsel));

	if (atomic_load_relaxed(&sc->sc_debuglevel) >= 3) {
		device_printf(sc->sc_dev, "post GET_STATS command\n");
	}

	mutex_enter(&sc->sc_adminq_lock);
	error = iavf_atq_post(sc, &iaq, aqb);
	mutex_exit(&sc->sc_adminq_lock);

	return error;
}

static int
iavf_config_irq_map(struct iavf_softc *sc, struct ixl_aq_buf *aqb)
{
	struct ixl_aq_desc iaq;
	struct iavf_vc_vector_map *vec;
	struct iavf_vc_irq_map_info *map;
	struct iavf_rx_ring *rxr;
	struct iavf_tx_ring *txr;
	unsigned int num_vec;
	int error;

	map = IXL_AQB_KVA(aqb);
	vec = map->vecmap;
	num_vec = 0;

	if (sc->sc_nintrs == 1) {
		vec[0].vsi_id = htole16(sc->sc_vsi_id);
		vec[0].vector_id = htole16(0);
		vec[0].rxq_map = htole16(iavf_allqueues(sc));
		vec[0].txq_map = htole16(iavf_allqueues(sc));
		vec[0].rxitr_idx = htole16(IAVF_NOITR);
		vec[0].rxitr_idx = htole16(IAVF_NOITR);
		num_vec = 1;
	} else if (sc->sc_nintrs > 1) {
		KASSERT(sc->sc_nqps_alloc >= (sc->sc_nintrs - 1));
		for (; num_vec < (sc->sc_nintrs - 1); num_vec++) {
			rxr = sc->sc_qps[num_vec].qp_rxr;
			txr = sc->sc_qps[num_vec].qp_txr;

			vec[num_vec].vsi_id = htole16(sc->sc_vsi_id);
			vec[num_vec].vector_id = htole16(num_vec + 1);
			vec[num_vec].rxq_map = htole16(__BIT(rxr->rxr_qid));
			vec[num_vec].txq_map = htole16(__BIT(txr->txr_qid));
			vec[num_vec].rxitr_idx = htole16(IAVF_ITR_RX);
			vec[num_vec].txitr_idx = htole16(IAVF_ITR_TX);
		}

		vec[num_vec].vsi_id = htole16(sc->sc_vsi_id);
		vec[num_vec].vector_id = htole16(0);
		vec[num_vec].rxq_map = htole16(0);
		vec[num_vec].txq_map = htole16(0);
		num_vec++;
	}

	map->num_vectors = htole16(num_vec);

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_CONFIG_IRQ_MAP);
	iaq.iaq_datalen = htole16(sizeof(*map) + sizeof(*vec) * num_vec);

	if (sc->sc_attached) {
		error = iavf_adminq_poll(sc, &iaq, aqb, 250);
	} else {
		error = iavf_adminq_poll_locked(sc, &iaq, aqb, 250);
	}

	if (error)
		return -1;

	return 0;
}

static int
iavf_config_vsi_queues(struct iavf_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct ixl_aq_desc iaq;
	struct ixl_aq_buf *aqb;
	struct iavf_vc_queue_config_info *config;
	struct iavf_vc_txq_info *txq;
	struct iavf_vc_rxq_info *rxq;
	struct iavf_rx_ring *rxr;
	struct iavf_tx_ring *txr;
	uint32_t rxmtu_max;
	unsigned int i;
	int error;

	rxmtu_max = ifp->if_mtu + IAVF_MTU_ETHERLEN;

	aqb = iavf_aqb_get(sc, &sc->sc_atq_idle);

	if (aqb == NULL)
		return -1;

	config = IXL_AQB_KVA(aqb);
	memset(config, 0, sizeof(*config));
	config->vsi_id = htole16(sc->sc_vsi_id);
	config->num_queue_pairs = htole16(sc->sc_nqueue_pairs);

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		rxr = sc->sc_qps[i].qp_rxr;
		txr = sc->sc_qps[i].qp_txr;

		txq = &config->qpair[i].txq;
		txq->vsi_id = htole16(sc->sc_vsi_id);
		txq->queue_id = htole16(txr->txr_qid);
		txq->ring_len = htole16(sc->sc_tx_ring_ndescs);
		txq->headwb_ena = 0;
		txq->dma_ring_addr = htole64(IXL_DMA_DVA(&txr->txr_mem));
		txq->dma_headwb_addr = 0;

		rxq = &config->qpair[i].rxq;
		rxq->vsi_id = htole16(sc->sc_vsi_id);
		rxq->queue_id = htole16(rxr->rxr_qid);
		rxq->ring_len = htole16(sc->sc_rx_ring_ndescs);
		rxq->splithdr_ena = 0;
		rxq->databuf_size = htole32(IAVF_MCLBYTES);
		rxq->max_pkt_size = htole32(rxmtu_max);
		rxq->dma_ring_addr = htole64(IXL_DMA_DVA(&rxr->rxr_mem));
		rxq->rx_split_pos = 0;
	}

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_CONFIG_VSI_QUEUES);
	iaq.iaq_datalen = htole16(sizeof(*config) +
	    sizeof(config->qpair[0]) * sc->sc_nqueue_pairs);

	error = iavf_adminq_exec(sc, &iaq, aqb);
	if (error != IAVF_VC_RC_SUCCESS) {
		return -1;
	}

	return 0;
}

static int
iavf_config_hena(struct iavf_softc *sc)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_buf *aqb;
	uint64_t *caps;
	int error;

	aqb = iavf_aqb_get(sc, &sc->sc_atq_idle);

	if (aqb == NULL)
		return -1;

	caps = IXL_AQB_KVA(aqb);
	if (sc->sc_mac_type == I40E_MAC_X722_VF)
		*caps = IXL_RSS_HENA_DEFAULT_X722;
	else
		*caps = IXL_RSS_HENA_DEFAULT_XL710;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_SET_RSS_HENA);
	iaq.iaq_datalen = htole16(sizeof(*caps));

	error = iavf_adminq_exec(sc, &iaq, aqb);
	if (error != IAVF_VC_RC_SUCCESS) {
		return -1;
	}

	return 0;
}

static inline void
iavf_get_default_rss_key(uint8_t *buf, size_t len)
{
	uint8_t rss_seed[RSS_KEYSIZE];
	size_t cplen;

	cplen = MIN(len, sizeof(rss_seed));
	rss_getkey(rss_seed);

	memcpy(buf, rss_seed, cplen);
	if (cplen < len)
		memset(buf + cplen, 0, len - cplen);
}

static int
iavf_config_rss_key(struct iavf_softc *sc)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_buf *aqb;
	struct iavf_vc_rss_key *rss_key;
	size_t key_len;
	int rv;

	aqb = iavf_aqb_get(sc, &sc->sc_atq_idle);
	if (aqb == NULL)
		return -1;

	rss_key = IXL_AQB_KVA(aqb);
	rss_key->vsi_id = htole16(sc->sc_vsi_id);
	key_len = IXL_RSS_KEY_SIZE;
	iavf_get_default_rss_key(rss_key->key, key_len);
	rss_key->key_len = key_len;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_CONFIG_RSS_KEY);
	iaq.iaq_datalen = htole16(sizeof(*rss_key) - sizeof(rss_key->pad)
	    + (sizeof(rss_key->key[0]) * key_len));

	rv = iavf_adminq_exec(sc, &iaq, aqb);
	if (rv != IAVF_VC_RC_SUCCESS) {
		return -1;
	}

	return 0;
}

static int
iavf_config_rss_lut(struct iavf_softc *sc)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_buf *aqb;
	struct iavf_vc_rss_lut *rss_lut;
	uint8_t *lut, v;
	int rv, i;

	aqb = iavf_aqb_get(sc, &sc->sc_atq_idle);
	if (aqb == NULL)
		return -1;

	rss_lut = IXL_AQB_KVA(aqb);
	rss_lut->vsi_id = htole16(sc->sc_vsi_id);
	rss_lut->lut_entries = htole16(IXL_RSS_VSI_LUT_SIZE);

	lut = rss_lut->lut;
	for (i = 0; i < IXL_RSS_VSI_LUT_SIZE; i++)  {
		v = i % sc->sc_nqueue_pairs;
		v &= IAVF_RSS_VSI_LUT_ENTRY_MASK;
		lut[i] = v;
	}

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_CONFIG_RSS_LUT);
	iaq.iaq_datalen = htole16(sizeof(*rss_lut) - sizeof(rss_lut->pad)
	    + (sizeof(rss_lut->lut[0]) * IXL_RSS_VSI_LUT_SIZE));

	rv = iavf_adminq_exec(sc, &iaq, aqb);
	if (rv != IAVF_VC_RC_SUCCESS) {
		return -1;
	}

	return 0;
}

static int
iavf_queue_select(struct iavf_softc *sc, int opcode)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_buf *aqb;
	struct iavf_vc_queue_select *qsel;
	int error;

	aqb = iavf_aqb_get(sc, &sc->sc_atq_idle);
	if (aqb == NULL)
		return -1;

	qsel = IXL_AQB_KVA(aqb);
	qsel->vsi_id = htole16(sc->sc_vsi_id);
	qsel->rx_queues = htole32(iavf_allqueues(sc));
	qsel->tx_queues = htole32(iavf_allqueues(sc));

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, opcode);
	iaq.iaq_datalen = htole16(sizeof(*qsel));

	error = iavf_adminq_exec(sc, &iaq, aqb);
	if (error != IAVF_VC_RC_SUCCESS) {
		return -1;
	}

	return 0;
}

static int
iavf_request_queues(struct iavf_softc *sc, unsigned int req_num)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_buf *aqb;
	struct iavf_vc_res_request *req;
	int rv;

	aqb = iavf_aqb_get(sc, &sc->sc_atq_idle);
	if (aqb == NULL)
		return ENOMEM;

	req = IXL_AQB_KVA(aqb);
	req->num_queue_pairs = req_num;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_REQUEST_QUEUES);
	iaq.iaq_datalen = htole16(sizeof(*req));

	mutex_enter(&sc->sc_adminq_lock);
	rv = iavf_atq_post(sc, &iaq, aqb);
	mutex_exit(&sc->sc_adminq_lock);

	return rv;
}

static int
iavf_reset_vf(struct iavf_softc *sc)
{
	struct ixl_aq_desc iaq;
	int error;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_RESET_VF);
	iaq.iaq_datalen = htole16(0);

	iavf_wr(sc, I40E_VFGEN_RSTAT, IAVF_VFR_INPROGRESS);

	mutex_enter(&sc->sc_adminq_lock);
	error = iavf_atq_post(sc, &iaq, NULL);
	mutex_exit(&sc->sc_adminq_lock);

	return error;
}

static int
iavf_eth_addr(struct iavf_softc *sc, const uint8_t *addr, uint32_t opcode)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_buf *aqb;
	struct iavf_vc_eth_addr_list *addrs;
	struct iavf_vc_eth_addr *vcaddr;
	int rv;

	KASSERT(sc->sc_attached);
	KASSERT(opcode == IAVF_VC_OP_ADD_ETH_ADDR ||
	    opcode == IAVF_VC_OP_DEL_ETH_ADDR);

	aqb = iavf_aqb_get(sc, &sc->sc_atq_idle);
	if (aqb == NULL)
		return -1;

	addrs = IXL_AQB_KVA(aqb);
	addrs->vsi_id = htole16(sc->sc_vsi_id);
	addrs->num_elements = htole16(1);
	vcaddr = addrs->list;
	memcpy(vcaddr->addr, addr, ETHER_ADDR_LEN);

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, opcode);
	iaq.iaq_datalen = htole16(sizeof(*addrs) + sizeof(*vcaddr));

	if (sc->sc_resetting) {
		mutex_enter(&sc->sc_adminq_lock);
		rv = iavf_adminq_poll_locked(sc, &iaq, aqb, 250);
		iavf_aqb_put_locked(&sc->sc_atq_idle, aqb);
		mutex_exit(&sc->sc_adminq_lock);
	} else {
		rv = iavf_adminq_exec(sc, &iaq, aqb);
	}

	if (rv != IAVF_VC_RC_SUCCESS) {
		return -1;
	}

	return 0;
}

static int
iavf_config_promisc_mode(struct iavf_softc *sc, int unicast, int multicast)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_buf *aqb;
	struct iavf_vc_promisc_info *promisc;
	int flags;

	KASSERT(sc->sc_attached);

	aqb = iavf_aqb_get(sc, &sc->sc_atq_idle);
	if (aqb == NULL)
		return -1;

	flags = 0;
	if (unicast)
		flags |= IAVF_FLAG_VF_UNICAST_PROMISC;
	if (multicast)
		flags |= IAVF_FLAG_VF_MULTICAST_PROMISC;

	promisc = IXL_AQB_KVA(aqb);
	promisc->vsi_id = htole16(sc->sc_vsi_id);
	promisc->flags = htole16(flags);

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, IAVF_VC_OP_CONFIG_PROMISC);
	iaq.iaq_datalen = htole16(sizeof(*promisc));

	if (iavf_adminq_exec(sc, &iaq, aqb) != IAVF_VC_RC_SUCCESS) {
		return -1;
	}

	return 0;
}

static int
iavf_config_vlan_stripping(struct iavf_softc *sc, int eccap)
{
	struct ixl_aq_desc iaq;
	uint32_t opcode;

	opcode = ISSET(eccap, ETHERCAP_VLAN_HWTAGGING) ?
	    IAVF_VC_OP_ENABLE_VLAN_STRIP : IAVF_VC_OP_DISABLE_VLAN_STRIP;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, opcode);
	iaq.iaq_datalen = htole16(0);

	if (iavf_adminq_exec(sc, &iaq, NULL) != IAVF_VC_RC_SUCCESS) {
		return -1;
	}

	return 0;
}

static int
iavf_config_vlan_id(struct iavf_softc *sc, uint16_t vid, uint32_t opcode)
{
	struct ixl_aq_desc iaq;
	struct ixl_aq_buf *aqb;
	struct iavf_vc_vlan_filter *vfilter;
	int rv;

	KASSERT(opcode == IAVF_VC_OP_ADD_VLAN || opcode == IAVF_VC_OP_DEL_VLAN);

	aqb = iavf_aqb_get(sc, &sc->sc_atq_idle);

	if (aqb == NULL)
		return -1;

	vfilter = IXL_AQB_KVA(aqb);
	vfilter->vsi_id = htole16(sc->sc_vsi_id);
	vfilter->num_vlan_id = htole16(1);
	vfilter->vlan_id[0] = vid;

	memset(&iaq, 0, sizeof(iaq));
	iaq.iaq_flags = htole16(IXL_AQ_BUF | IXL_AQ_RD);
	iaq.iaq_opcode = htole16(IAVF_AQ_OP_SEND_TO_PF);
	iavf_aq_vc_set_opcode(&iaq, opcode);
	iaq.iaq_datalen = htole16(sizeof(*vfilter) + sizeof(vid));

	if (sc->sc_resetting) {
		mutex_enter(&sc->sc_adminq_lock);
		rv = iavf_adminq_poll_locked(sc, &iaq, aqb, 250);
		iavf_aqb_put_locked(&sc->sc_atq_idle, aqb);
		mutex_exit(&sc->sc_adminq_lock);
	} else {
		rv = iavf_adminq_exec(sc, &iaq, aqb);
	}

	if (rv != IAVF_VC_RC_SUCCESS) {
		return -1;
	}

	return 0;
}

static void
iavf_post_request_queues(void *xsc)
{
	struct iavf_softc *sc;
	struct ifnet *ifp;

	sc = xsc;
	ifp = &sc->sc_ec.ec_if;

	if (!ISSET(sc->sc_vf_cap, IAVF_VC_OFFLOAD_REQ_QUEUES)) {
		log(LOG_DEBUG, "%s: the VF has no REQ_QUEUES capability\n",
		    ifp->if_xname);
		return;
	}

	log(LOG_INFO, "%s: try to change the number of queue pairs"
	    " (vsi %u, %u allocated, request %u)\n",
	    ifp->if_xname,
	    sc->sc_nqps_vsi, sc->sc_nqps_alloc, sc->sc_nqps_req);
	iavf_request_queues(sc, sc->sc_nqps_req);
}

static bool
iavf_sysctlnode_is_rx(struct sysctlnode *node)
{

	if (strstr(node->sysctl_parent->sysctl_name, "rx") != NULL)
		return true;

	return false;
}

static int
iavf_sysctl_itr_handler(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct iavf_softc *sc = (struct iavf_softc *)node.sysctl_data;
	uint32_t newitr, *itrptr;
	unsigned int i;
	int itr, error;

	if (iavf_sysctlnode_is_rx(&node)) {
		itrptr = &sc->sc_rx_itr;
		itr = IAVF_ITR_RX;
	} else {
		itrptr = &sc->sc_tx_itr;
		itr = IAVF_ITR_TX;
	}

	newitr = *itrptr;
	node.sysctl_data = &newitr;
	node.sysctl_size = sizeof(newitr);

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (newitr > 0x07FF)
		return EINVAL;

	*itrptr = newitr;

	for (i = 0; i < sc->sc_nqueue_pairs; i++) {
		iavf_wr(sc, I40E_VFINT_ITRN1(itr, i), *itrptr);
	}
	iavf_wr(sc, I40E_VFINT_ITR01(itr), *itrptr);

	return 0;
}

static void
iavf_workq_work(struct work *wk, void *context)
{
	struct iavf_work *work;

	work = container_of(wk, struct iavf_work, ixw_cookie);

	atomic_swap_uint(&work->ixw_added, 0);
	work->ixw_func(work->ixw_arg);
}

static struct workqueue *
iavf_workq_create(const char *name, pri_t prio, int ipl, int flags)
{
	struct workqueue *wq;
	int error;

	error = workqueue_create(&wq, name, iavf_workq_work, NULL,
	    prio, ipl, flags);

	if (error)
		return NULL;

	return wq;
}

static void
iavf_workq_destroy(struct workqueue *wq)
{

	workqueue_destroy(wq);
}

static int
iavf_work_set(struct iavf_work *work, void (*func)(void *), void *arg)
{

	if (work->ixw_added != 0)
		return -1;

	memset(work, 0, sizeof(*work));
	work->ixw_func = func;
	work->ixw_arg = arg;

	return 0;
}

static void
iavf_work_add(struct workqueue *wq, struct iavf_work *work)
{
	if (atomic_cas_uint(&work->ixw_added, 0, 1) != 0)
		return;

	kpreempt_disable();
	workqueue_enqueue(wq, &work->ixw_cookie, NULL);
	kpreempt_enable();
}

static void
iavf_work_wait(struct workqueue *wq, struct iavf_work *work)
{

	workqueue_wait(wq, &work->ixw_cookie);
}

static void
iavf_evcnt_attach(struct evcnt *ec,
    const char *n0, const char *n1)
{

	evcnt_attach_dynamic(ec, EVCNT_TYPE_MISC,
	    NULL, n0, n1);
}

MODULE(MODULE_CLASS_DRIVER, if_iavf, "pci");

#ifdef _MODULE
#include "ioconf.c"
#endif

#ifdef _MODULE
static void
iavf_parse_modprop(prop_dictionary_t dict)
{
	prop_object_t obj;
	int64_t val;
	uint32_t n;

	if (dict == NULL)
		return;

	obj = prop_dictionary_get(dict, "debug_level");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_NUMBER) {
		val = prop_number_signed_value((prop_number_t)obj);

		if (val > 0) {
			iavf_params.debug = val;
			printf("iavf: debug level=%d\n", iavf_params.debug);
		}
	}

	obj = prop_dictionary_get(dict, "max_qps");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_NUMBER) {
		val = prop_number_signed_value((prop_number_t)obj);

		if (val < 1 || val > I40E_MAX_VF_QUEUES) {
			printf("iavf: invalid queue size(1 <= n <= %d)",
			    I40E_MAX_VF_QUEUES);
		} else {
			iavf_params.max_qps = val;
			printf("iavf: request queue pair = %u\n",
			    iavf_params.max_qps);
		}
	}

	obj = prop_dictionary_get(dict, "tx_itr");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_NUMBER) {
		val = prop_number_signed_value((prop_number_t)obj);
		if (val > 0x07FF) {
			printf("iavf: TX ITR too big (%" PRId64 " <= %d)",
			    val, 0x7FF);
		} else {
			iavf_params.tx_itr = val;
			printf("iavf: TX ITR = 0x%" PRIx32,
			    iavf_params.tx_itr);
		}
	}

	obj = prop_dictionary_get(dict, "rx_itr");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_NUMBER) {
		val = prop_number_signed_value((prop_number_t)obj);
		if (val > 0x07FF) {
			printf("iavf: RX ITR too big (%" PRId64 " <= %d)",
			    val, 0x7FF);
		} else {
			iavf_params.rx_itr = val;
			printf("iavf: RX ITR = 0x%" PRIx32,
			    iavf_params.rx_itr);
		}
	}

	obj = prop_dictionary_get(dict, "tx_ndescs");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_NUMBER) {
		val = prop_number_signed_value((prop_number_t)obj);
		n = 1U << (fls32(val) - 1);
		if (val != (int64_t) n) {
			printf("iavf: TX desc invalid size"
			    "(%" PRId64 " != %" PRIu32 ")\n", val, n);
		} else if (val > (8192 - 32)) {
			printf("iavf: Tx desc too big (%" PRId64 " > %d)",
			    val, (8192 - 32));
		} else {
			iavf_params.tx_ndescs = val;
			printf("iavf: TX descriptors = 0x%04x",
			    iavf_params.tx_ndescs);
		}
	}

	obj = prop_dictionary_get(dict, "rx_ndescs");
	if (obj != NULL && prop_object_type(obj) == PROP_TYPE_NUMBER) {
		val = prop_number_signed_value((prop_number_t)obj);
		n = 1U << (fls32(val) - 1);
		if (val != (int64_t) n) {
			printf("iavf: RX desc invalid size"
			    "(%" PRId64 " != %" PRIu32 ")\n", val, n);
		} else if (val > (8192 - 32)) {
			printf("iavf: Rx desc too big (%" PRId64 " > %d)",
			    val, (8192 - 32));
		} else {
			iavf_params.rx_ndescs = val;
			printf("iavf: RX descriptors = 0x%04x",
			    iavf_params.rx_ndescs);
		}
	}
}
#endif

static int
if_iavf_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		iavf_parse_modprop((prop_dictionary_t)opaque);
		error = config_init_component(cfdriver_ioconf_if_iavf,
		    cfattach_ioconf_if_iavf, cfdata_ioconf_if_iavf);
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(cfdriver_ioconf_if_iavf,
		    cfattach_ioconf_if_iavf, cfdata_ioconf_if_iavf);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
