/*	$NetBSD: if_cnmac.c,v 1.29 2022/09/29 07:00:46 skrll Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cnmac.c,v 1.29 2022/09/29 07:00:46 skrll Exp $");

/*
 * If no free send buffer is available, free all the sent buffers and bail out.
 */
#define CNMAC_SEND_QUEUE_CHECK

/* XXX XXX XXX XXX XXX XXX */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/conf.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/route.h>
#include <net/bpf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>

#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/endian.h>
#include <machine/locore.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <mips/cpuregs.h>

#include <mips/cavium/octeonreg.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/include/iobusvar.h>

#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_faureg.h>
#include <mips/cavium/dev/octeon_fpareg.h>
#include <mips/cavium/dev/octeon_gmxreg.h>
#include <mips/cavium/dev/octeon_pipreg.h>
#include <mips/cavium/dev/octeon_powreg.h>
#include <mips/cavium/dev/octeon_fauvar.h>
#include <mips/cavium/dev/octeon_fpavar.h>
#include <mips/cavium/dev/octeon_gmxvar.h>
#include <mips/cavium/dev/octeon_ipdvar.h>
#include <mips/cavium/dev/octeon_pipvar.h>
#include <mips/cavium/dev/octeon_pkovar.h>
#include <mips/cavium/dev/octeon_powvar.h>
#include <mips/cavium/dev/octeon_smivar.h>

#include <mips/cavium/dev/if_cnmacvar.h>

/*
 * Set the PKO to think command buffers are an odd length.  This makes it so we
 * never have to divide a comamnd across two buffers.
 */
#define OCTEON_POOL_NWORDS_CMD	\
	    (((uint32_t)OCTEON_POOL_SIZE_CMD / sizeof(uint64_t)) - 1)
#define FPA_COMMAND_BUFFER_POOL_NWORDS	OCTEON_POOL_NWORDS_CMD	/* XXX */

static void	cnmac_buf_init(struct cnmac_softc *);

static int	cnmac_match(device_t, struct cfdata *, void *);
static void	cnmac_attach(device_t, device_t, void *);
static void	cnmac_pip_init(struct cnmac_softc *);
static void	cnmac_ipd_init(struct cnmac_softc *);
static void	cnmac_pko_init(struct cnmac_softc *);

static void	cnmac_board_mac_addr(uint8_t *, size_t, struct cnmac_softc *);

static int	cnmac_mii_readreg(device_t, int, int, uint16_t *);
static int	cnmac_mii_writereg(device_t, int, int, uint16_t);
static void	cnmac_mii_statchg(struct ifnet *);

static int	cnmac_mediainit(struct cnmac_softc *);
static void	cnmac_mediastatus(struct ifnet *, struct ifmediareq *);

static inline void cnmac_send_queue_flush_prefetch(struct cnmac_softc *);
static inline void cnmac_send_queue_flush_fetch(struct cnmac_softc *);
static inline void cnmac_send_queue_flush(struct cnmac_softc *);
static inline void cnmac_send_queue_flush_sync(struct cnmac_softc *);
static void cnmac_send_queue_check_and_flush(struct cnmac_softc *);
static inline int cnmac_send_queue_is_full(struct cnmac_softc *);
static inline void cnmac_send_queue_add(struct cnmac_softc *, struct mbuf *,
    uint64_t *);
static inline void cnmac_send_queue_del(struct cnmac_softc *, struct mbuf **,
    uint64_t **);
static inline int cnmac_buf_free_work(struct cnmac_softc *, uint64_t *);
static inline void cnmac_buf_ext_free(struct mbuf *, void *, size_t, void *);

static int	cnmac_ioctl(struct ifnet *, u_long, void *);
static void	cnmac_watchdog(struct ifnet *);
static int	cnmac_init(struct ifnet *);
static void	cnmac_stop(struct ifnet *, int);
static void	cnmac_start(struct ifnet *);

static inline int cnmac_send_cmd(struct cnmac_softc *, uint64_t, uint64_t,
    int *);
static inline uint64_t	cnmac_send_makecmd_w1(int, paddr_t);
static inline uint64_t	cnmac_send_makecmd_w0(uint64_t, uint64_t, size_t, int,
    int);
static inline int cnmac_send_makecmd_gbuf(struct cnmac_softc *, struct mbuf *,
    uint64_t *, int *);
static inline int cnmac_send_makecmd(struct cnmac_softc *, struct mbuf *,
    uint64_t *, uint64_t *, uint64_t *);
static inline int cnmac_send_buf(struct cnmac_softc *, struct mbuf *,
    uint64_t *, int *);
static inline int cnmac_send(struct cnmac_softc *, struct mbuf *, int *);

static int	cnmac_reset(struct cnmac_softc *);
static int	cnmac_configure(struct cnmac_softc *);
static int	cnmac_configure_common(struct cnmac_softc *);

static void	cnmac_tick_free(void *);
static void	cnmac_tick_misc(void *);

static inline int cnmac_recv_mbuf(struct cnmac_softc *, uint64_t *,
    struct mbuf **);
static inline int cnmac_recv_check(struct cnmac_softc *, uint64_t);
static inline int cnmac_recv(struct cnmac_softc *, uint64_t *);
static int	cnmac_intr(void *);

/* device parameters */
int		cnmac_param_pko_cmd_w0_n2 = 1;

CFATTACH_DECL_NEW(cnmac, sizeof(struct cnmac_softc),
    cnmac_match, cnmac_attach, NULL, NULL);

/* ---- buffer management */

static const struct cnmac_pool_param {
	int			poolno;
	size_t			size;
	size_t			nelems;
} cnmac_pool_params[] = {
#define	_ENTRY(x)	{ OCTEON_POOL_NO_##x, OCTEON_POOL_SIZE_##x, OCTEON_POOL_NELEMS_##x }
	_ENTRY(PKT),
	_ENTRY(WQE),
	_ENTRY(CMD),
	_ENTRY(SG)
#undef	_ENTRY
};
struct octfpa_buf	*cnmac_pools[FPA_NPOOLS];
#define	cnmac_fb_pkt	cnmac_pools[OCTEON_POOL_NO_PKT]
#define	cnmac_fb_wqe	cnmac_pools[OCTEON_POOL_NO_WQE]
#define	cnmac_fb_cmd	cnmac_pools[OCTEON_POOL_NO_CMD]
#define	cnmac_fb_sg	cnmac_pools[OCTEON_POOL_NO_SG]

static int	cnmac_npowgroups = 0;

static void
cnmac_buf_init(struct cnmac_softc *sc)
{
	static int once;
	int i;
	const struct cnmac_pool_param *pp;
	struct octfpa_buf *fb;

	if (once == 1)
		return;
	once = 1;

	for (i = 0; i < (int)__arraycount(cnmac_pool_params); i++) {
		pp = &cnmac_pool_params[i];
		octfpa_buf_init(pp->poolno, pp->size, pp->nelems, &fb);
		cnmac_pools[i] = fb;
	}
}

/* ---- autoconf */

static int
cnmac_match(device_t parent, struct cfdata *match, void *aux)
{
	struct octgmx_attach_args *ga = aux;

	if (strcmp(match->cf_name, ga->ga_name) != 0) {
		return 0;
	}
	return 1;
}

static void
cnmac_attach(device_t parent, device_t self, void *aux)
{
	struct cnmac_softc *sc = device_private(self);
	struct octgmx_attach_args *ga = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	prop_dictionary_t dict;
	prop_object_t clk;
	uint8_t enaddr[ETHER_ADDR_LEN];

	if (cnmac_npowgroups >= OCTEON_POW_GROUP_MAX) {
		printf(": out of POW groups\n");
	}

	sc->sc_dev = self;
	sc->sc_regt = ga->ga_regt;
	sc->sc_port = ga->ga_portno;
	sc->sc_port_type = ga->ga_port_type;
	sc->sc_gmx = ga->ga_gmx;
	sc->sc_gmx_port = ga->ga_gmx_port;
	sc->sc_smi = ga->ga_smi;
	sc->sc_powgroup = cnmac_npowgroups++;

	if (sc->sc_port >= CVMSEG_LM_ETHER_COUNT) {
		/*
		 * If we got here, increase CVMSEG_LM_ETHER_COUNT
		 * in octeonvar.h .
		 */
		printf("%s: ERROR out of CVMSEG LM buffers\n",
		    device_xname(self));
		return;
	}

	sc->sc_init_flag = 0;
	/*
	 * XXXUEBAYASI
	 * Setting PIP_IP_OFFSET[OFFSET] to 8 causes panic ... why???
	 */
	sc->sc_ip_offset = 0/* XXX */;

	if (MIPS_PRID_IMPL(mips_options.mips_cpu_id) <= MIPS_CN30XX) {
		SET(sc->sc_quirks, CNMAC_QUIRKS_NO_PRE_ALIGN);
		SET(sc->sc_quirks, CNMAC_QUIRKS_NO_RX_INBND);
	}

	cnmac_board_mac_addr(enaddr, sizeof(enaddr), sc);
	printf("%s: Ethernet address %s\n", device_xname(self),
	    ether_sprintf(enaddr));

	SIMPLEQ_INIT(&sc->sc_sendq);
	sc->sc_soft_req_thresh = 15/* XXX */;
	sc->sc_ext_callback_cnt = 0;

	octgmx_stats_init(sc->sc_gmx_port);

	callout_init(&sc->sc_tick_misc_ch, 0);
	callout_setfunc(&sc->sc_tick_misc_ch, cnmac_tick_misc, sc);

	callout_init(&sc->sc_tick_free_ch, 0);
	callout_setfunc(&sc->sc_tick_free_ch, cnmac_tick_free, sc);

	const int dv_unit = device_unit(self);
	octfau_op_init(&sc->sc_fau_done,
	    OCTEON_CVMSEG_ETHER_OFFSET(dv_unit, csm_ether_fau_done),
	    OCT_FAU_REG_ADDR_END - (8 * (dv_unit + 1))/* XXX */);
	octfau_op_set_8(&sc->sc_fau_done, 0);

	cnmac_pip_init(sc);
	cnmac_ipd_init(sc);
	cnmac_pko_init(sc);

	cnmac_configure_common(sc);

	sc->sc_gmx_port->sc_ipd = sc->sc_ipd;
	sc->sc_gmx_port->sc_port_mii = &sc->sc_mii;
	sc->sc_gmx_port->sc_port_ec = &sc->sc_ethercom;
	/* XXX */
	sc->sc_gmx_port->sc_quirks = sc->sc_quirks;

	/* XXX */
	sc->sc_pow = &octpow_softc;

	cnmac_mediainit(sc);

	strncpy(ifp->if_xname, device_xname(self), sizeof(ifp->if_xname));
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = cnmac_ioctl;
	ifp->if_start = cnmac_start;
	ifp->if_watchdog = cnmac_watchdog;
	ifp->if_init = cnmac_init;
	ifp->if_stop = cnmac_stop;
	IFQ_SET_MAXLEN(&ifp->if_snd, uimax(GATHER_QUEUE_SIZE, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);


	ifp->if_capabilities =
#if 0	/* XXX: no tx checksum yet */
	    IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
	    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx |
	    IFCAP_CSUM_TCPv6_Tx | IFCAP_CSUM_TCPv6_Rx |
	    IFCAP_CSUM_UDPv6_Tx | IFCAP_CSUM_UDPv6_Rx;
#else
	    IFCAP_CSUM_IPv4_Rx | IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |
	    IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx;
#endif

	/* 802.1Q VLAN-sized frames are supported */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	octgmx_set_mac_addr(sc->sc_gmx_port, enaddr);

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);
	octgmx_set_filter(sc->sc_gmx_port);

#if 1
	cnmac_buf_init(sc);
#endif

	sc->sc_ih = octeon_intr_establish(POW_WORKQ_IRQ(sc->sc_powgroup),
	    IPL_NET, cnmac_intr, sc);
	if (sc->sc_ih == NULL)
		panic("%s: could not set up interrupt", device_xname(self));

	dict = device_properties(sc->sc_gmx->sc_dev);

	clk = prop_dictionary_get(dict, "rgmii-tx");
	if (clk)
		sc->sc_gmx_port->sc_clk_tx_setting =
		    prop_number_signed_value(clk);
	clk = prop_dictionary_get(dict, "rgmii-rx");
	if (clk)
		sc->sc_gmx_port->sc_clk_rx_setting =
		    prop_number_signed_value(clk);
}

/* ---- submodules */

/* XXX */
static void
cnmac_pip_init(struct cnmac_softc *sc)
{
	struct octpip_attach_args pip_aa;

	pip_aa.aa_port = sc->sc_port;
	pip_aa.aa_regt = sc->sc_regt;
	pip_aa.aa_tag_type = POW_TAG_TYPE_ORDERED/* XXX */;
	pip_aa.aa_receive_group = sc->sc_powgroup;
	pip_aa.aa_ip_offset = sc->sc_ip_offset;
	octpip_init(&pip_aa, &sc->sc_pip);
	octpip_port_config(sc->sc_pip);
}

/* XXX */
static void
cnmac_ipd_init(struct cnmac_softc *sc)
{
	struct octipd_attach_args ipd_aa;

	ipd_aa.aa_port = sc->sc_port;
	ipd_aa.aa_regt = sc->sc_regt;
	ipd_aa.aa_first_mbuff_skip = 184/* XXX */;
	ipd_aa.aa_not_first_mbuff_skip = 0/* XXX */;
	octipd_init(&ipd_aa, &sc->sc_ipd);
}

/* XXX */
static void
cnmac_pko_init(struct cnmac_softc *sc)
{
	struct octpko_attach_args pko_aa;

	pko_aa.aa_port = sc->sc_port;
	pko_aa.aa_regt = sc->sc_regt;
	pko_aa.aa_cmdptr = &sc->sc_cmdptr;
	pko_aa.aa_cmd_buf_pool = OCTEON_POOL_NO_CMD;
	pko_aa.aa_cmd_buf_size = OCTEON_POOL_NWORDS_CMD;
	octpko_init(&pko_aa, &sc->sc_pko);
}

/* ---- XXX */

#define	ADDR2UINT64(u, a) \
	do { \
		u = \
		    (((uint64_t)a[0] << 40) | ((uint64_t)a[1] << 32) | \
		     ((uint64_t)a[2] << 24) | ((uint64_t)a[3] << 16) | \
		     ((uint64_t)a[4] <<	 8) | ((uint64_t)a[5] <<  0)); \
	} while (0)
#define	UINT642ADDR(a, u) \
	do { \
		a[0] = (uint8_t)((u) >> 40); a[1] = (uint8_t)((u) >> 32); \
		a[2] = (uint8_t)((u) >> 24); a[3] = (uint8_t)((u) >> 16); \
		a[4] = (uint8_t)((u) >>	 8); a[5] = (uint8_t)((u) >>  0); \
	} while (0)

static void
cnmac_board_mac_addr(uint8_t *enaddr, size_t size, struct cnmac_softc *sc)
{
	prop_dictionary_t dict;
	prop_data_t ea;

	dict = device_properties(sc->sc_dev);
	KASSERT(dict != NULL);
	ea = prop_dictionary_get(dict, "mac-address");
	KASSERT(ea != NULL);
	memcpy(enaddr, prop_data_value(ea), size);
}

/* ---- media */

static int
cnmac_mii_readreg(device_t self, int phy_addr, int reg, uint16_t *val)
{
	struct cnmac_softc *sc = device_private(self);

	return octsmi_read(sc->sc_smi, phy_addr, reg, val);
}

static int
cnmac_mii_writereg(device_t self, int phy_addr, int reg, uint16_t val)
{
	struct cnmac_softc *sc = device_private(self);

	return octsmi_write(sc->sc_smi, phy_addr, reg, val);
}

static void
cnmac_mii_statchg(struct ifnet *ifp)
{
	struct cnmac_softc *sc = ifp->if_softc;

	octpko_port_enable(sc->sc_pko, 0);
	octgmx_port_enable(sc->sc_gmx_port, 0);

	cnmac_reset(sc);

	if (ISSET(ifp->if_flags, IFF_RUNNING))
		octgmx_set_filter(sc->sc_gmx_port);

	octpko_port_enable(sc->sc_pko, 1);
	octgmx_port_enable(sc->sc_gmx_port, 1);
}

static int
cnmac_mediainit(struct cnmac_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mii_data *mii = &sc->sc_mii;
	prop_object_t phy;

	mii->mii_ifp = ifp;
	mii->mii_readreg = cnmac_mii_readreg;
	mii->mii_writereg = cnmac_mii_writereg;
	mii->mii_statchg = cnmac_mii_statchg;
	sc->sc_ethercom.ec_mii = mii;

	/* Initialize ifmedia structures. */
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, cnmac_mediastatus);

	phy = prop_dictionary_get(device_properties(sc->sc_dev), "phy-addr");
	KASSERT(phy != NULL);

	mii_attach(sc->sc_dev, mii, 0xffffffff, prop_number_signed_value(phy),
	    MII_OFFSET_ANY, MIIF_DOPAUSE);

	/* XXX XXX XXX */
	if (LIST_FIRST(&mii->mii_phys) != NULL) {
		/* XXX XXX XXX */
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);
		/* XXX XXX XXX */
	} else {
		/* XXX XXX XXX */
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_NONE,
		    MII_MEDIA_NONE, NULL);
		/* XXX XXX XXX */
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_NONE);
		/* XXX XXX XXX */
	}
	/* XXX XXX XXX */

	return 0;
}

static void
cnmac_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct cnmac_softc *sc = ifp->if_softc;

	mii_pollstat(&sc->sc_mii);

	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
	ifmr->ifm_active = (sc->sc_mii.mii_media_active & ~IFM_ETH_FMASK) |
	    sc->sc_gmx_port->sc_port_flowflags;
}

/* ---- send buffer garbage collection */

static inline void
cnmac_send_queue_flush_prefetch(struct cnmac_softc *sc)
{

	KASSERT(sc->sc_prefetch == 0);
	octfau_op_inc_fetch_8(&sc->sc_fau_done, 0);
	sc->sc_prefetch = 1;
}

static inline void
cnmac_send_queue_flush_fetch(struct cnmac_softc *sc)
{

	KASSERT(sc->sc_prefetch == 1);
	sc->sc_hard_done_cnt = octfau_op_inc_read_8(&sc->sc_fau_done);
	KASSERT(sc->sc_hard_done_cnt <= 0);
	sc->sc_prefetch = 0;
}

static inline void
cnmac_send_queue_flush(struct cnmac_softc *sc)
{
	const int64_t sent_count = sc->sc_hard_done_cnt;
	int i;

	KASSERT(sc->sc_flush == 0);
	KASSERT(sent_count <= 0);

	for (i = 0; i < 0 - sent_count; i++) {
		struct mbuf *m;
		uint64_t *gbuf;

		cnmac_send_queue_del(sc, &m, &gbuf);

		octfpa_buf_put(cnmac_fb_sg, gbuf);

		m_freem(m);

		sc->sc_txbusy = false;
	}

	octfau_op_inc_fetch_8(&sc->sc_fau_done, i);
	sc->sc_flush = i;
}

static inline void
cnmac_send_queue_flush_sync(struct cnmac_softc *sc)
{
	if (sc->sc_flush == 0)
		return;

	KASSERT(sc->sc_flush > 0);

	/* XXX XXX XXX */
	octfau_op_inc_read_8(&sc->sc_fau_done);
	sc->sc_soft_req_cnt -= sc->sc_flush;
	KASSERT(sc->sc_soft_req_cnt >= 0);
	/* XXX XXX XXX */

	sc->sc_flush = 0;
}

static inline int
cnmac_send_queue_is_full(struct cnmac_softc *sc)
{
#ifdef CNMAC_SEND_QUEUE_CHECK
	int64_t nofree_cnt;

	nofree_cnt = sc->sc_soft_req_cnt + sc->sc_hard_done_cnt;

	if (__predict_false(nofree_cnt == GATHER_QUEUE_SIZE - 1)) {
		cnmac_send_queue_flush(sc);
		cnmac_send_queue_flush_sync(sc);
		return 1;
	}

#endif
	return 0;
}

static void
cnmac_send_queue_check_and_flush(struct cnmac_softc *sc)
{
	int s;

	/* XXX XXX XXX */
	s = splnet();
	if (sc->sc_soft_req_cnt > 0) {
		cnmac_send_queue_flush_prefetch(sc);
		cnmac_send_queue_flush_fetch(sc);
		cnmac_send_queue_flush(sc);
		cnmac_send_queue_flush_sync(sc);
	}
	splx(s);
	/* XXX XXX XXX */
}

/*
 * (Ab)use m_nextpkt and m_paddr to maintain mbuf chain and pointer to gather
 * buffer.  Other mbuf members may be used by m_freem(), so don't touch them!
 */

struct _send_queue_entry {
	union {
		struct mbuf _sqe_s_mbuf;
		struct {
			char _sqe_s_entry_pad[offsetof(struct mbuf, m_nextpkt)];
			SIMPLEQ_ENTRY(_send_queue_entry) _sqe_s_entry_entry;
		} _sqe_s_entry;
		struct {
			char _sqe_s_gbuf_pad[offsetof(struct mbuf, m_paddr)];
			uint64_t *_sqe_s_gbuf_gbuf;
		} _sqe_s_gbuf;
	} _sqe_u;
#define	_sqe_entry	_sqe_u._sqe_s_entry._sqe_s_entry_entry
#define	_sqe_gbuf	_sqe_u._sqe_s_gbuf._sqe_s_gbuf_gbuf
};

static inline void
cnmac_send_queue_add(struct cnmac_softc *sc, struct mbuf *m,
    uint64_t *gbuf)
{
	struct _send_queue_entry *sqe = (struct _send_queue_entry *)m;

	sqe->_sqe_gbuf = gbuf;
	SIMPLEQ_INSERT_TAIL(&sc->sc_sendq, sqe, _sqe_entry);

	if ((m->m_flags & M_EXT) && m->m_ext.ext_free != NULL)
		sc->sc_ext_callback_cnt++;
}

static inline void
cnmac_send_queue_del(struct cnmac_softc *sc, struct mbuf **rm, uint64_t **rgbuf)
{
	struct _send_queue_entry *sqe;

	sqe = SIMPLEQ_FIRST(&sc->sc_sendq);
	KASSERT(sqe != NULL);
	SIMPLEQ_REMOVE_HEAD(&sc->sc_sendq, _sqe_entry);

	*rm = (void *)sqe;
	*rgbuf = sqe->_sqe_gbuf;

	if (((*rm)->m_flags & M_EXT) && (*rm)->m_ext.ext_free != NULL) {
		sc->sc_ext_callback_cnt--;
		KASSERT(sc->sc_ext_callback_cnt >= 0);
	}
}

static inline int
cnmac_buf_free_work(struct cnmac_softc *sc, uint64_t *work)
{

	/* XXX when jumbo frame */
	if (ISSET(work[2], PIP_WQE_WORD2_IP_BUFS)) {
		paddr_t addr;
		paddr_t start_buffer;

		addr = work[3] & PIP_WQE_WORD3_ADDR;
		start_buffer = addr & ~(2048 - 1);

		octfpa_buf_put_paddr(cnmac_fb_pkt, start_buffer);
	}

	octfpa_buf_put(cnmac_fb_wqe, work);

	return 0;
}

static inline void
cnmac_buf_ext_free(struct mbuf *m, void *buf, size_t size, void *arg)
{
	octfpa_buf_put(cnmac_fb_pkt, buf);

	KASSERT(m != NULL);

	pool_cache_put(mb_cache, m);
}

/* ---- ifnet interfaces */

static int
cnmac_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct cnmac_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

	s = splnet();
	switch (cmd) {
	case SIOCSIFMEDIA:
		/* Flow control requires full-duplex mode. */
		if (IFM_SUBTYPE(ifr->ifr_media) == IFM_AUTO ||
		    (ifr->ifr_media & IFM_FDX) == 0) {
			ifr->ifr_media &= ~IFM_ETH_FMASK;
		}
		if (IFM_SUBTYPE(ifr->ifr_media) != IFM_AUTO) {
			if ((ifr->ifr_media & IFM_ETH_FMASK) == IFM_FLOW) {
				ifr->ifr_media |=
				    IFM_ETH_TXPAUSE | IFM_ETH_RXPAUSE;
			}
			sc->sc_gmx_port->sc_port_flowflags =
				ifr->ifr_media & IFM_ETH_FMASK;
		}
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}

	if (error == ENETRESET) {
		if (ISSET(ifp->if_flags, IFF_RUNNING))
			octgmx_set_filter(sc->sc_gmx_port);
		error = 0;
	}

	cnmac_start(ifp);

	splx(s);

	return error;
}

/* ---- send (output) */

static inline uint64_t
cnmac_send_makecmd_w0(uint64_t fau0, uint64_t fau1, size_t len, int segs,
    int ipoffp1)
{

	return octpko_cmd_word0(
		OCT_FAU_OP_SIZE_64,		/* sz1 */
		OCT_FAU_OP_SIZE_64,		/* sz0 */
		1, fau1, 1, fau0,		/* s1, reg1, s0, reg0 */
		0,				/* le */
		cnmac_param_pko_cmd_w0_n2,	/* n2 */
		1, 0,				/* q, r */
		(segs == 1) ? 0 : 1,		/* g */
		0, 0, 1,			/* ipoffp1, ii, df */
		segs, (int)len);		/* segs, totalbytes */
}

static inline uint64_t
cnmac_send_makecmd_w1(int size, paddr_t addr)
{

	return octpko_cmd_word1(
		0, 0,				/* i, back */
		OCTEON_POOL_NO_SG,		/* pool */
		size, addr);			/* size, addr */
}

static inline int
cnmac_send_makecmd_gbuf(struct cnmac_softc *sc, struct mbuf *m0, uint64_t *gbuf,
    int *rsegs)
{
	struct mbuf *m;
	int segs = 0;
	uintptr_t laddr, rlen, nlen;

	for (m = m0; m != NULL; m = m->m_next) {

		if (__predict_false(m->m_len == 0))
			continue;

		/* Aligned 4k */
		laddr = (uintptr_t)m->m_data & (PAGE_SIZE - 1);

		if (laddr + m->m_len > PAGE_SIZE) {
			/* XXX XXX XXX */
			rlen = PAGE_SIZE - laddr;
			nlen = m->m_len - rlen;
			*(gbuf + segs) = cnmac_send_makecmd_w1(rlen,
			    kvtophys((vaddr_t)m->m_data));
			segs++;
			if (segs > 63) {
				return 1;
			}
			/* XXX XXX XXX */
		} else {
			rlen = 0;
			nlen = m->m_len;
		}

		*(gbuf + segs) = cnmac_send_makecmd_w1(nlen,
		    kvtophys((vaddr_t)(m->m_data + rlen)));
		segs++;
		if (segs > 63) {
			return 1;
		}
	}

	KASSERT(m == NULL);

	*rsegs = segs;

	return 0;
}

static inline int
cnmac_send_makecmd(struct cnmac_softc *sc, struct mbuf *m,
    uint64_t *gbuf, uint64_t *rpko_cmd_w0, uint64_t *rpko_cmd_w1)
{
	uint64_t pko_cmd_w0, pko_cmd_w1;
	int ipoffp1;
	int segs;
	int result = 0;

	if (cnmac_send_makecmd_gbuf(sc, m, gbuf, &segs)) {
		log(LOG_WARNING, "%s: there are a lot of number of segments"
		    " of transmission data", device_xname(sc->sc_dev));
		result = 1;
		goto done;
	}

	/*  Get the IP packet offset for TCP/UDP checksum offloading. */
	ipoffp1 = (m->m_pkthdr.csum_flags & (M_CSUM_TCPv4 | M_CSUM_UDPv4))
	    ? (ETHER_HDR_LEN + 1) : 0;

	/*
	 * segs == 1	-> link mode (single continuous buffer)
	 *		   WORD1[size] is number of bytes pointed by segment
	 *
	 * segs > 1	-> gather mode (scatter-gather buffer)
	 *		   WORD1[size] is number of segments
	 */
	pko_cmd_w0 = cnmac_send_makecmd_w0(sc->sc_fau_done.fd_regno,
	    0, m->m_pkthdr.len, segs, ipoffp1);
	if (segs == 1) {
		pko_cmd_w1 = cnmac_send_makecmd_w1(
		    m->m_pkthdr.len, kvtophys((vaddr_t)m->m_data));
	} else {
#ifdef __mips_n32
		KASSERT(MIPS_KSEG0_P(gbuf));
		pko_cmd_w1 = cnmac_send_makecmd_w1(segs,
		    MIPS_KSEG0_TO_PHYS(gbuf));
#else
		pko_cmd_w1 = cnmac_send_makecmd_w1(segs,
		    MIPS_XKPHYS_TO_PHYS(gbuf));
#endif
	}

	*rpko_cmd_w0 = pko_cmd_w0;
	*rpko_cmd_w1 = pko_cmd_w1;

done:
	return result;
}

static inline int
cnmac_send_cmd(struct cnmac_softc *sc, uint64_t pko_cmd_w0,
    uint64_t pko_cmd_w1, int *pwdc)
{
	uint64_t *cmdptr;
	int result = 0;

#ifdef __mips_n32
	KASSERT((sc->sc_cmdptr.cmdptr & ~MIPS_PHYS_MASK) == 0);
	cmdptr = (uint64_t *)MIPS_PHYS_TO_KSEG0(sc->sc_cmdptr.cmdptr);
#else
	cmdptr = (uint64_t *)MIPS_PHYS_TO_XKPHYS_CACHED(sc->sc_cmdptr.cmdptr);
#endif
	cmdptr += sc->sc_cmdptr.cmdptr_idx;

	KASSERT(cmdptr != NULL);

	*cmdptr++ = pko_cmd_w0;
	*cmdptr++ = pko_cmd_w1;

	KASSERT(sc->sc_cmdptr.cmdptr_idx + 2 <= FPA_COMMAND_BUFFER_POOL_NWORDS - 1);

	if (sc->sc_cmdptr.cmdptr_idx + 2 == FPA_COMMAND_BUFFER_POOL_NWORDS - 1) {
		paddr_t buf;

		buf = octfpa_buf_get_paddr(cnmac_fb_cmd);
		if (buf == 0) {
			log(LOG_WARNING,
			    "%s: can not allocate command buffer from free pool allocator\n",
			    device_xname(sc->sc_dev));
			result = 1;
			goto done;
		}
		*cmdptr++ = buf;
		sc->sc_cmdptr.cmdptr = (uint64_t)buf;
		sc->sc_cmdptr.cmdptr_idx = 0;
	} else {
		sc->sc_cmdptr.cmdptr_idx += 2;
	}

	*pwdc += 2;

done:
	return result;
}

static inline int
cnmac_send_buf(struct cnmac_softc *sc, struct mbuf *m, uint64_t *gbuf,
    int *pwdc)
{
	int result = 0, error;
	uint64_t pko_cmd_w0, pko_cmd_w1;

	error = cnmac_send_makecmd(sc, m, gbuf, &pko_cmd_w0, &pko_cmd_w1);
	if (error != 0) {
		/* Already logging */
		result = error;
		goto done;
	}

	error = cnmac_send_cmd(sc, pko_cmd_w0, pko_cmd_w1, pwdc);
	if (error != 0) {
		/* Already logging */
		result = error;
	}

done:
	return result;
}

static inline int
cnmac_send(struct cnmac_softc *sc, struct mbuf *m, int *pwdc)
{
	paddr_t gaddr = 0;
	uint64_t *gbuf = NULL;
	int result = 0, error;

	gaddr = octfpa_buf_get_paddr(cnmac_fb_sg);
	if (gaddr == 0) {
		log(LOG_WARNING, "%s: can not allocate gather buffer from "
		    "free pool allocator\n", device_xname(sc->sc_dev));
		result = 1;
		goto done;
	}

#ifdef __mips_n32
	KASSERT((gaddr & ~MIPS_PHYS_MASK) == 0);
	gbuf = (uint64_t *)(uintptr_t)MIPS_PHYS_TO_KSEG0(gaddr);
#else
	gbuf = (uint64_t *)(uintptr_t)MIPS_PHYS_TO_XKPHYS_CACHED(gaddr);
#endif

	KASSERT(gbuf != NULL);

	error = cnmac_send_buf(sc, m, gbuf, pwdc);
	if (error != 0) {
		/* Already logging */
		octfpa_buf_put_paddr(cnmac_fb_sg, gaddr);
		result = error;
		goto done;
	}

	cnmac_send_queue_add(sc, m, gbuf);

done:
	return result;
}

static void
cnmac_start(struct ifnet *ifp)
{
	struct cnmac_softc *sc = ifp->if_softc;
	struct mbuf *m;
	int wdc = 0;

	/*
	 * Performance tuning
	 * pre-send iobdma request
	 */
	cnmac_send_queue_flush_prefetch(sc);

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		goto last;

	if (sc->sc_txbusy)
		goto last;

	if (__predict_false(!octgmx_link_status(sc->sc_gmx_port)))
		goto last;

	for (;;) {
		IFQ_POLL(&ifp->if_snd, m);
		if (__predict_false(m == NULL))
			break;

		/* XXX XXX XXX */
		cnmac_send_queue_flush_fetch(sc);

		/*
		 * If no free send buffer is available, free all the sent
		 * buffers and bail out.
		 */
		if (cnmac_send_queue_is_full(sc)) {
			sc->sc_txbusy = true;
			if (wdc > 0)
				octpko_op_doorbell_write(sc->sc_port,
				    sc->sc_port, wdc);
			callout_schedule(&sc->sc_tick_free_ch, 1);
			return;
		}
		/* XXX XXX XXX */

		IFQ_DEQUEUE(&ifp->if_snd, m);

		bpf_mtap(ifp, m, BPF_D_OUT);

		/* XXX XXX XXX */
		if (sc->sc_soft_req_cnt > sc->sc_soft_req_thresh)
			cnmac_send_queue_flush(sc);
		if (cnmac_send(sc, m, &wdc)) {
			IF_DROP(&ifp->if_snd);
			m_freem(m);
			log(LOG_WARNING,
			  "%s: failed in the transmission of the packet\n",
			  device_xname(sc->sc_dev));
		} else
			sc->sc_soft_req_cnt++;

		if (sc->sc_flush)
			cnmac_send_queue_flush_sync(sc);
		/* XXX XXX XXX */

		/* Send next iobdma request */
		cnmac_send_queue_flush_prefetch(sc);
	}

	if (wdc > 0)
		octpko_op_doorbell_write(sc->sc_port, sc->sc_port, wdc);

last:
	cnmac_send_queue_flush_fetch(sc);
	callout_schedule(&sc->sc_tick_free_ch, 1);
}

static void
cnmac_watchdog(struct ifnet *ifp)
{
	struct cnmac_softc *sc = ifp->if_softc;

	printf("%s: device timeout\n", device_xname(sc->sc_dev));

	cnmac_configure(sc);

	SET(ifp->if_flags, IFF_RUNNING);
	sc->sc_txbusy = false;
	ifp->if_timer = 0;

	cnmac_start(ifp);
}

static int
cnmac_init(struct ifnet *ifp)
{
	struct cnmac_softc *sc = ifp->if_softc;

	/* XXX don't disable commonly used parts!!! XXX */
	if (sc->sc_init_flag == 0) {
		/* Cancel any pending I/O. */
		cnmac_stop(ifp, 0);

		/* Initialize the device */
		cnmac_configure(sc);

		octpko_enable(sc->sc_pko);
		octipd_enable(sc->sc_ipd);

		sc->sc_init_flag = 1;
	} else {
		octgmx_port_enable(sc->sc_gmx_port, 1);
	}
	mii_ifmedia_change(&sc->sc_mii);

	octgmx_set_filter(sc->sc_gmx_port);

	callout_schedule(&sc->sc_tick_misc_ch, hz);
	callout_schedule(&sc->sc_tick_free_ch, hz);

	SET(ifp->if_flags, IFF_RUNNING);
	sc->sc_txbusy = false;

	return 0;
}

static void
cnmac_stop(struct ifnet *ifp, int disable)
{
	struct cnmac_softc *sc = ifp->if_softc;

	callout_stop(&sc->sc_tick_misc_ch);
	callout_stop(&sc->sc_tick_free_ch);

	mii_down(&sc->sc_mii);

	octgmx_port_enable(sc->sc_gmx_port, 0);

	/* Mark the interface as down and cancel the watchdog timer. */
	CLR(ifp->if_flags, IFF_RUNNING);
	sc->sc_txbusy = false;
	ifp->if_timer = 0;
}

/* ---- misc */

static int
cnmac_reset(struct cnmac_softc *sc)
{
	octgmx_reset_speed(sc->sc_gmx_port);
	octgmx_reset_flowctl(sc->sc_gmx_port);
	octgmx_reset_timing(sc->sc_gmx_port);

	return 0;
}

static int
cnmac_configure(struct cnmac_softc *sc)
{
	octgmx_port_enable(sc->sc_gmx_port, 0);

	cnmac_reset(sc);

	cnmac_configure_common(sc);

	octpko_port_config(sc->sc_pko);
	octpko_port_enable(sc->sc_pko, 1);
	octpow_config(sc->sc_pow, sc->sc_powgroup);

	octgmx_tx_stats_rd_clr(sc->sc_gmx_port, 1);
	octgmx_rx_stats_rd_clr(sc->sc_gmx_port, 1);

	octgmx_port_enable(sc->sc_gmx_port, 1);

	return 0;
}

static int
cnmac_configure_common(struct cnmac_softc *sc)
{
	static int once;

	if (once == 1)
		return 0;
	once = 1;

	octipd_config(sc->sc_ipd);
	octpko_config(sc->sc_pko);

	return 0;
}

/* ---- receive (input) */

static inline int
cnmac_recv_mbuf(struct cnmac_softc *sc, uint64_t *work, struct mbuf **rm)
{
	struct mbuf *m;
	vaddr_t addr;
	vaddr_t ext_buf;
	size_t ext_size;
	uint64_t word1 = work[1];
	uint64_t word2 = work[2];
	uint64_t word3 = work[3];

	MGETHDR(m, M_NOWAIT, MT_DATA);
	if (m == NULL)
		return 1;

	octfpa_buf_put(cnmac_fb_wqe, work);

	if (__SHIFTOUT(word2, PIP_WQE_WORD2_IP_BUFS) != 1)
		panic("%s: expected one buffer, got %" PRId64, __func__,
		    __SHIFTOUT(word2, PIP_WQE_WORD2_IP_BUFS));


#ifdef __mips_n32
	KASSERT((word3 & ~MIPS_PHYS_MASK) == 0);
	addr = MIPS_PHYS_TO_KSEG0(word3 & PIP_WQE_WORD3_ADDR);
#else
	addr = MIPS_PHYS_TO_XKPHYS_CACHED(word3 & PIP_WQE_WORD3_ADDR);
#endif

	ext_size = OCTEON_POOL_SIZE_PKT;
	ext_buf = addr & ~(ext_size - 1);
	MEXTADD(m, ext_buf, ext_size, 0, cnmac_buf_ext_free, NULL);

	m->m_data = (void *)addr;
	m->m_len = m->m_pkthdr.len = (word1 & PIP_WQE_WORD1_LEN) >> 48;
	m_set_rcvif(m, &sc->sc_ethercom.ec_if);

	/* Not readonly buffer */
	m->m_flags |= M_EXT_RW;

	*rm = m;

	KASSERT(*rm != NULL);

	return 0;
}

static inline int
cnmac_recv_check(struct cnmac_softc *sc, uint64_t word2)
{
	static struct timeval rxerr_log_interval = { 0, 2500000 };
	uint64_t opecode;

	if (__predict_true(!ISSET(word2, PIP_WQE_WORD2_NOIP_RE)))
		return 0;

	opecode = word2 & PIP_WQE_WORD2_NOIP_OPECODE;
	if ((sc->sc_ethercom.ec_if.if_flags & IFF_DEBUG) &&
	    ratecheck(&sc->sc_rxerr_log_last, &rxerr_log_interval))
		log(LOG_DEBUG, "%s: rx error (%"PRId64")\n",
		    device_xname(sc->sc_dev), opecode);

	/* This error is harmless */
	if (opecode == PIP_WQE_WORD2_RE_OPCODE_OVRRUN)
		return 0;

	return 1;
}

static inline int
cnmac_recv(struct cnmac_softc *sc, uint64_t *work)
{
	struct ifnet *ifp;
	struct mbuf *m;
	uint64_t word2;

	KASSERT(sc != NULL);
	KASSERT(work != NULL);

	word2 = work[2];
	ifp = &sc->sc_ethercom.ec_if;

	KASSERT(ifp != NULL);

	if (!ISSET(ifp->if_flags, IFF_RUNNING))
		goto drop;

	if (__predict_false(cnmac_recv_check(sc, word2) != 0)) {
		if_statinc(ifp, if_ierrors);
		goto drop;
	}

	if (__predict_false(cnmac_recv_mbuf(sc, work, &m) != 0)) {
		if_statinc(ifp, if_ierrors);
		goto drop;
	}

	/* work[0] .. work[3] may not be valid any more */

	KASSERT(m != NULL);

	octipd_offload(word2, m->m_data, &m->m_pkthdr.csum_flags);

	if_percpuq_enqueue(ifp->if_percpuq, m);

	return 0;

drop:
	cnmac_buf_free_work(sc, work);
	return 1;
}

static int
cnmac_intr(void *arg)
{
	struct cnmac_softc *sc = arg;
	uint64_t *work;
	uint64_t wqmask = __BIT(sc->sc_powgroup);
	uint32_t coreid = 0;	/* XXX octeon_get_coreid() */
	uint32_t port;

	_POW_WR8(sc->sc_pow, POW_PP_GRP_MSK_OFFSET(coreid), wqmask);

	octpow_tag_sw_wait();
	octpow_work_request_async(OCTEON_CVMSEG_OFFSET(csm_pow_intr),
	    POW_NO_WAIT);

	for (;;) {
		work = (uint64_t *)octpow_work_response_async(
		    OCTEON_CVMSEG_OFFSET(csm_pow_intr));
		if (work == NULL)
			break;

		octpow_tag_sw_wait();
		octpow_work_request_async(OCTEON_CVMSEG_OFFSET(csm_pow_intr),
		    POW_NO_WAIT);

		port = __SHIFTOUT(work[1], PIP_WQE_WORD1_IPRT);
		if (port != sc->sc_port) {
			printf("%s: unexpected wqe port %u, should be %u\n",
			    device_xname(sc->sc_dev), port, sc->sc_port);
			goto wqe_error;
		}

		(void)cnmac_recv(sc, work);

		cnmac_send_queue_check_and_flush(sc);
	}

	_POW_WR8(sc->sc_pow, POW_WQ_INT_OFFSET, wqmask);

	return 1;

wqe_error:
	printf("word0: 0x%016" PRIx64 "\n", work[0]);
	printf("word1: 0x%016" PRIx64 "\n", work[1]);
	printf("word2: 0x%016" PRIx64 "\n", work[2]);
	printf("word3: 0x%016" PRIx64 "\n", work[3]);
	panic("wqe_error");
}

/* ---- tick */

/*
 * cnmac_tick_free
 *
 * => garbage collect send gather buffer / mbuf
 * => called at softclock
 */
static void
cnmac_tick_free(void *arg)
{
	struct cnmac_softc *sc = arg;
	int timo;

	cnmac_send_queue_check_and_flush(sc);

	timo = (sc->sc_ext_callback_cnt > 0) ? 1 : hz;
	callout_schedule(&sc->sc_tick_free_ch, timo);
}

/*
 * cnmac_tick_misc
 *
 * => collect statistics
 * => check link status
 * => called at softclock
 */
static void
cnmac_tick_misc(void *arg)
{
	struct cnmac_softc *sc = arg;
	struct ifnet *ifp;
	int s;

	s = splnet();

	ifp = &sc->sc_ethercom.ec_if;

	octgmx_stats(sc->sc_gmx_port);
	octpip_stats(sc->sc_pip, ifp, sc->sc_port);
	mii_tick(&sc->sc_mii);

	splx(s);

	callout_schedule(&sc->sc_tick_misc_ch, hz);
}
