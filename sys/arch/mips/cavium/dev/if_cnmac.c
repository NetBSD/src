/*	$NetBSD: if_cnmac.c,v 1.23 2020/06/22 02:26:19 simonb Exp $	*/

#include <sys/cdefs.h>
#if 0
__KERNEL_RCSID(0, "$NetBSD: if_cnmac.c,v 1.23 2020/06/22 02:26:19 simonb Exp $");
#endif

#include "opt_octeon.h"

/*
 * If no free send buffer is available, free all the sent buffers and bail out.
 */
#define CNMAC_SEND_QUEUE_CHECK

/* XXX XXX XXX XXX XXX XXX */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
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
#include <net/if_dl.h>
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
#include <mips/cavium/include/iobusvar.h>

#include <mips/cavium/dev/octeon_ciureg.h>
#include <mips/cavium/dev/octeon_gmxreg.h>
#include <mips/cavium/dev/octeon_pipreg.h>
#include <mips/cavium/dev/octeon_powreg.h>
#include <mips/cavium/dev/octeon_faureg.h>
#include <mips/cavium/dev/octeon_fpareg.h>
#include <mips/cavium/dev/octeon_fpavar.h>
#include <mips/cavium/dev/octeon_gmxvar.h>
#include <mips/cavium/dev/octeon_fauvar.h>
#include <mips/cavium/dev/octeon_powvar.h>
#include <mips/cavium/dev/octeon_ipdvar.h>
#include <mips/cavium/dev/octeon_pipvar.h>
#include <mips/cavium/dev/octeon_pkovar.h>
#include <mips/cavium/dev/octeon_asxvar.h>
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
static void	cnmac_asx_init(struct cnmac_softc *);
static void	cnmac_smi_init(struct cnmac_softc *);

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
static inline int cnmac_send_queue_is_full(struct cnmac_softc *);
static inline void cnmac_send_queue_add(struct cnmac_softc *, struct mbuf *,
    uint64_t *);
static inline void cnmac_send_queue_del(struct cnmac_softc *, struct mbuf **,
    uint64_t **);
static inline int cnmac_buf_free_work(struct cnmac_softc *, uint64_t *,
    uint64_t);
static inline void cnmac_buf_ext_free_m(struct mbuf *, void *, size_t, void *);
static inline void cnmac_buf_ext_free_ext(struct mbuf *, void *, size_t,
    void *);

static int	cnmac_ioctl(struct ifnet *, u_long, void *);
static void	cnmac_watchdog(struct ifnet *);
static int	cnmac_init(struct ifnet *);
static void	cnmac_stop(struct ifnet *, int);
static void	cnmac_start(struct ifnet *);

static inline int cnmac_send_cmd(struct cnmac_softc *, uint64_t, uint64_t,
    int *);
static inline uint64_t	cnmac_send_makecmd_w1(int, paddr_t);
static inline uint64_t	cnmac_send_makecmd_w0(uint64_t, uint64_t, size_t, int);
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
static inline int cnmac_recv_check_code(struct cnmac_softc *, uint64_t);
static inline int cnmac_recv_check_jumbo(struct cnmac_softc *, uint64_t);
static inline int cnmac_recv_check_link(struct cnmac_softc *, uint64_t);
static inline int cnmac_recv_check(struct cnmac_softc *, uint64_t);
static inline int cnmac_recv(struct cnmac_softc *, uint64_t *);
static void	cnmac_recv_redir(struct ifnet *, struct mbuf *);
static inline void cnmac_recv_intr(void *, uint64_t *);

/* Device driver context */
static struct	cnmac_softc *cnmac_gsc[GMX_PORT_NUNITS];
static void	*cnmac_pow_recv_ih;

/* sysctl'able parameters */
int		cnmac_param_pko_cmd_w0_n2 = 1;
int		cnmac_param_pip_dyn_rs = 1;
int		cnmac_param_redir = 0;
int		cnmac_param_pktbuf = 0;
int		cnmac_param_rate = 0;
int		cnmac_param_intr = 0;

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
struct octfpa_buf	*cnmac_pools[8/* XXX */];
#define	cnmac_fb_pkt	cnmac_pools[OCTEON_POOL_NO_PKT]
#define	cnmac_fb_wqe	cnmac_pools[OCTEON_POOL_NO_WQE]
#define	cnmac_fb_cmd	cnmac_pools[OCTEON_POOL_NO_CMD]
#define	cnmac_fb_sg	cnmac_pools[OCTEON_POOL_NO_SG]

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

	sc->sc_dev = self;
	sc->sc_regt = ga->ga_regt;
	sc->sc_port = ga->ga_portno;
	sc->sc_port_type = ga->ga_port_type;
	sc->sc_gmx = ga->ga_gmx;
	sc->sc_gmx_port = ga->ga_gmx_port;

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
	printf("%s: Ethernet address %s\n", device_xname(sc->sc_dev),
	    ether_sprintf(enaddr));

	cnmac_gsc[sc->sc_port] = sc;

	SIMPLEQ_INIT(&sc->sc_sendq);
	sc->sc_soft_req_thresh = 15/* XXX */;
	sc->sc_ext_callback_cnt = 0;

	octgmx_stats_init(sc->sc_gmx_port);

	callout_init(&sc->sc_tick_misc_ch, 0);
	callout_init(&sc->sc_tick_free_ch, 0);

	octfau_op_init(&sc->sc_fau_done,
	    OCTEON_CVMSEG_ETHER_OFFSET(sc->sc_port, csm_ether_fau_done),
	    OCT_FAU_REG_ADDR_END - (8 * (sc->sc_port + 1))/* XXX */);
	octfau_op_set_8(&sc->sc_fau_done, 0);

	cnmac_pip_init(sc);
	cnmac_ipd_init(sc);
	cnmac_pko_init(sc);
	cnmac_asx_init(sc);
	cnmac_smi_init(sc);

	sc->sc_gmx_port->sc_ipd = sc->sc_ipd;
	sc->sc_gmx_port->sc_port_asx = sc->sc_asx;
	sc->sc_gmx_port->sc_port_mii = &sc->sc_mii;
	sc->sc_gmx_port->sc_port_ec = &sc->sc_ethercom;
	/* XXX */
	sc->sc_gmx_port->sc_quirks = sc->sc_quirks;

	/* XXX */
	sc->sc_pow = &octpow_softc;

	cnmac_mediainit(sc);

	strncpy(ifp->if_xname, device_xname(sc->sc_dev), sizeof(ifp->if_xname));
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = cnmac_ioctl;
	ifp->if_start = cnmac_start;
	ifp->if_watchdog = cnmac_watchdog;
	ifp->if_init = cnmac_init;
	ifp->if_stop = cnmac_stop;
	IFQ_SET_MAXLEN(&ifp->if_snd, uimax(GATHER_QUEUE_SIZE, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);

	/* XXX: not yet tx checksum */
	ifp->if_capabilities =
	    IFCAP_CSUM_IPv4_Rx | IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |
	    IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx;

	/* 802.1Q VLAN-sized frames are supported */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	octgmx_set_mac_addr(sc->sc_gmx_port, enaddr);

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);
	octgmx_set_filter(sc->sc_gmx_port);

	/* XXX */
	sc->sc_rate_recv_check_link_cap.tv_sec = 1;
	sc->sc_rate_recv_check_jumbo_cap.tv_sec = 1;
	sc->sc_rate_recv_check_code_cap.tv_sec = 1;
	sc->sc_rate_recv_fixup_odd_nibble_short_cap.tv_sec = 1;
	sc->sc_rate_recv_fixup_odd_nibble_preamble_cap.tv_sec = 1;
	sc->sc_rate_recv_fixup_odd_nibble_crc_cap.tv_sec = 1;
	/* XXX */

#if 1
	cnmac_buf_init(sc);
#endif

	if (cnmac_pow_recv_ih == NULL)
		cnmac_pow_recv_ih
		    = octpow_intr_establish(OCTEON_POW_GROUP_PIP,
			IPL_NET, cnmac_recv_intr, NULL, NULL);

	dict = device_properties(sc->sc_gmx->sc_dev);

	clk = prop_dictionary_get(dict, "rgmii-tx");
	KASSERT(clk != NULL);
	sc->sc_gmx_port->sc_clk_tx_setting = prop_number_signed_value(clk);
	clk = prop_dictionary_get(dict, "rgmii-rx");
	KASSERT(clk != NULL);
	sc->sc_gmx_port->sc_clk_rx_setting = prop_number_signed_value(clk);
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
	pip_aa.aa_receive_group = OCTEON_POW_GROUP_PIP;
	pip_aa.aa_ip_offset = sc->sc_ip_offset;
	octpip_init(&pip_aa, &sc->sc_pip);
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

/* XXX */
static void
cnmac_asx_init(struct cnmac_softc *sc)
{
	struct octasx_attach_args asx_aa;

	asx_aa.aa_port = sc->sc_port;
	asx_aa.aa_regt = sc->sc_regt;
	octasx_init(&asx_aa, &sc->sc_asx);
}

static void
cnmac_smi_init(struct cnmac_softc *sc)
{
	struct octsmi_attach_args smi_aa;

	smi_aa.aa_port = sc->sc_port;
	smi_aa.aa_regt = sc->sc_regt;
	octsmi_init(&smi_aa, &sc->sc_smi);
	octsmi_set_clock(sc->sc_smi, 0x1464ULL); /* XXX */
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
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
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

		CLR(ifp->if_flags, IFF_OACTIVE);
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
cnmac_buf_free_work(struct cnmac_softc *sc, uint64_t *work, uint64_t word2)
{
	/* XXX when jumbo frame */
	if (ISSET(word2, PIP_WQE_WORD2_IP_BUFS)) {
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
cnmac_buf_ext_free_m(struct mbuf *m, void *buf, size_t size, void *arg)
{
	uint64_t *work = (void *)arg;
	int s = splnet();

	octfpa_buf_put(cnmac_fb_wqe, work);

	KASSERT(m != NULL);

	pool_cache_put(mb_cache, m);

	splx(s);
}

static inline void
cnmac_buf_ext_free_ext(struct mbuf *m, void *buf, size_t size, void *arg)
{
	uint64_t *work = (void *)arg;
	int s = splnet();

	octfpa_buf_put(cnmac_fb_wqe, work);

	octfpa_buf_put(cnmac_fb_pkt, buf);

	KASSERT(m != NULL);

	pool_cache_put(mb_cache, m);

	splx(s);
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
		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			if (ISSET(ifp->if_flags, IFF_RUNNING))
				octgmx_set_filter(sc->sc_gmx_port);
			error = 0;
		}
		break;
	}
	cnmac_start(ifp);
	splx(s);

	return error;
}

/* ---- send (output) */

static inline uint64_t
cnmac_send_makecmd_w0(uint64_t fau0, uint64_t fau1, size_t len, int segs)
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
		FPA_GATHER_BUFFER_POOL,		/* pool */
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

#if 0
		KASSERT(((uint32_t)m->m_data & (PAGE_SIZE - 1))
		   == (kvtophys((vaddr_t)m->m_data) & (PAGE_SIZE - 1)));
#endif

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
	int segs;
	int result = 0;

	if (cnmac_send_makecmd_gbuf(sc, m, gbuf, &segs)) {
		log(LOG_WARNING, "%s: there are a lot of number of segments"
		    " of transmission data", device_xname(sc->sc_dev));
		result = 1;
		goto done;
	}

	/*
	 * segs == 1	-> link mode (single continuous buffer)
	 *		   WORD1[size] is number of bytes pointed by segment
	 *
	 * segs > 1	-> gather mode (scatter-gather buffer)
	 *		   WORD1[size] is number of segments
	 */
	pko_cmd_w0 = cnmac_send_makecmd_w0(sc->sc_fau_done.fd_regno,
	    0, m->m_pkthdr.len, segs);
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

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		goto last;

	/* XXX assume that OCTEON doesn't buffer packets */
	if (__predict_false(!octgmx_link_status(sc->sc_gmx_port))) {
		/* Dequeue and drop them */
		while (1) {
			IFQ_DEQUEUE(&ifp->if_snd, m);
			if (m == NULL)
				break;

			m_freem(m);
			IF_DROP(&ifp->if_snd);
		}
		goto last;
	}

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
			SET(ifp->if_flags, IFF_OACTIVE);
			if (wdc > 0)
				octpko_op_doorbell_write(sc->sc_port,
				    sc->sc_port, wdc);
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

/*
 * Don't schedule send-buffer-free callout every time - those buffers are freed
 * by "free tick".  This makes some packets like NFS slower.
 */
#ifdef CNMAC_USENFS
	if (__predict_false(sc->sc_ext_callback_cnt > 0)) {
		int timo;

		/* ??? */
		timo = hz - (100 * sc->sc_ext_callback_cnt);
		if (timo < 10)
			timo = 10;
		callout_schedule(&sc->sc_tick_free_ch, timo);
	}
#endif

last:
	cnmac_send_queue_flush_fetch(sc);
}

static void
cnmac_watchdog(struct ifnet *ifp)
{
	struct cnmac_softc *sc = ifp->if_softc;

	printf("%s: device timeout\n", device_xname(sc->sc_dev));

	cnmac_configure(sc);

	SET(ifp->if_flags, IFF_RUNNING);
	CLR(ifp->if_flags, IFF_OACTIVE);
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

	callout_reset(&sc->sc_tick_misc_ch, hz, cnmac_tick_misc, sc);
	callout_reset(&sc->sc_tick_free_ch, hz, cnmac_tick_free, sc);

	SET(ifp->if_flags, IFF_RUNNING);
	CLR(ifp->if_flags, IFF_OACTIVE);

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
	CLR(ifp->if_flags, IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

/* ---- misc */

#define PKO_INDEX_MASK	((1ULL << 12/* XXX */) - 1)

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
	octpip_port_config(sc->sc_pip);

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
#ifdef CNMAC_IPD_RED
	octipd_red(sc->sc_ipd, RECV_QUEUE_SIZE >> 2, RECV_QUEUE_SIZE >> 3);
#endif
	octpko_config(sc->sc_pko);

	octpow_config(sc->sc_pow, OCTEON_POW_GROUP_PIP);

	return 0;
}

/* ---- receive (input) */

static inline int
cnmac_recv_mbuf(struct cnmac_softc *sc, uint64_t *work, struct mbuf **rm)
{
	struct mbuf *m;
	void (*ext_free)(struct mbuf *, void *, size_t, void *);
	void *ext_buf;
	size_t ext_size;
	void *data;
	uint64_t word1 = work[1];
	uint64_t word2 = work[2];
	uint64_t word3 = work[3];

	MGETHDR(m, M_NOWAIT, MT_DATA);
	if (m == NULL)
		return 1;
	KASSERT(m != NULL);

	if ((word2 & PIP_WQE_WORD2_IP_BUFS) == 0) {
		/* Dynamic short */
		ext_free = cnmac_buf_ext_free_m;
		ext_buf = &work[4];
		ext_size = 96;

		data = &work[4 + sc->sc_ip_offset / sizeof(uint64_t)];
	} else {
		vaddr_t addr;
		vaddr_t start_buffer;

#ifdef __mips_n32
		KASSERT((word3 & ~MIPS_PHYS_MASK) == 0);
		addr = MIPS_PHYS_TO_KSEG0(word3 & PIP_WQE_WORD3_ADDR);
#else
		addr = MIPS_PHYS_TO_XKPHYS_CACHED(word3 & PIP_WQE_WORD3_ADDR);
#endif
		start_buffer = addr & ~(2048 - 1);

		ext_free = cnmac_buf_ext_free_ext;
		ext_buf = (void *)start_buffer;
		ext_size = 2048;

		data = (void *)addr;
	}

	/* Embed sc pointer into work[0] for _ext_free evcnt */
	work[0] = (uintptr_t)sc;

	MEXTADD(m, ext_buf, ext_size, 0, ext_free, work);
	KASSERT(ISSET(m->m_flags, M_EXT));

	m->m_data = data;
	m->m_len = m->m_pkthdr.len = (word1 & PIP_WQE_WORD1_LEN) >> 48;
	m_set_rcvif(m, &sc->sc_ethercom.ec_if);

	/* Not readonly buffer */
	m->m_flags |= M_EXT_RW;

	*rm = m;

	KASSERT(*rm != NULL);

	return 0;
}

static inline int
cnmac_recv_check_code(struct cnmac_softc *sc, uint64_t word2)
{
	uint64_t opecode = word2 & PIP_WQE_WORD2_NOIP_OPECODE;

	if (__predict_true(!ISSET(word2, PIP_WQE_WORD2_NOIP_RE)))
		return 0;

	/* This error is harmless */
	if (opecode == PIP_OVER_ERR)
		return 0;

	return 1;
}

static inline int
cnmac_recv_check_jumbo(struct cnmac_softc *sc, uint64_t word2)
{
	if (__predict_false((word2 & PIP_WQE_WORD2_IP_BUFS) > (1ULL << 56)))
		return 1;
	return 0;
}

static inline int
cnmac_recv_check_link(struct cnmac_softc *sc, uint64_t word2)
{
	if (__predict_false(!octgmx_link_status(sc->sc_gmx_port)))
		return 1;
	return 0;
}

static inline int
cnmac_recv_check(struct cnmac_softc *sc, uint64_t word2)
{
	if (__predict_false(cnmac_recv_check_link(sc, word2)) != 0) {
		if (ratecheck(&sc->sc_rate_recv_check_link_last,
		    &sc->sc_rate_recv_check_link_cap))
			log(LOG_DEBUG,
			    "%s: link is not up, the packet was dropped\n",
			    device_xname(sc->sc_dev));
		return 1;
	}

#if 0 /* XXX Performance tuning (Jumbo-frame is not supported yet!) */
	if (__predict_false(cnmac_recv_check_jumbo(sc, word2)) != 0) {
		/* XXX jumbo frame */
		if (ratecheck(&sc->sc_rate_recv_check_jumbo_last,
		    &sc->sc_rate_recv_check_jumbo_cap))
			log(LOG_DEBUG,
			    "jumbo frame was received\n");
		return 1;
	}
#endif

	if (__predict_false(cnmac_recv_check_code(sc, word2)) != 0) {

		if ((word2 & PIP_WQE_WORD2_NOIP_OPECODE) ==
				PIP_WQE_WORD2_RE_OPCODE_LENGTH) {
			/* No logging */
			/* XXX increment special error count */
		} else if ((word2 & PIP_WQE_WORD2_NOIP_OPECODE) ==
				PIP_WQE_WORD2_RE_OPCODE_PARTIAL) {
			/* Not an error, it's because of overload */
		} else {

			if (ratecheck(&sc->sc_rate_recv_check_code_last,
			    &sc->sc_rate_recv_check_code_cap))
				log(LOG_WARNING,
				    "%s: reception error, packet dropped "
				    "(error code = %" PRId64 ")\n",
				    device_xname(sc->sc_dev), word2 & PIP_WQE_WORD2_NOIP_OPECODE);
		}
		return 1;
	}

	return 0;
}

static inline int
cnmac_recv(struct cnmac_softc *sc, uint64_t *work)
{
	int result = 0;
	struct ifnet *ifp;
	struct mbuf *m;
	uint64_t word2;

	/* XXX XXX XXX */
	/*
	 * Performance tuning
	 * pre-send iobdma request
	 */
	if (sc->sc_soft_req_cnt > sc->sc_soft_req_thresh) {
		cnmac_send_queue_flush_prefetch(sc);
	}
	/* XXX XXX XXX */

	KASSERT(sc != NULL);
	KASSERT(work != NULL);

	word2 = work[2];
	ifp = &sc->sc_ethercom.ec_if;

	KASSERT(ifp != NULL);

	if (__predict_false(cnmac_recv_check(sc, word2) != 0)) {
		if_statinc(ifp, if_ierrors);
		result = 1;
		cnmac_buf_free_work(sc, work, word2);
		goto drop;
	}

	if (__predict_false(cnmac_recv_mbuf(sc, work, &m) != 0)) {
		if_statinc(ifp, if_ierrors);
		result = 1;
		cnmac_buf_free_work(sc, work, word2);
		goto drop;
	}

	/* work[0] .. work[3] may not be valid any more */

	KASSERT(m != NULL);

	octipd_offload(word2, m->m_data, &m->m_pkthdr.csum_flags);

	/* XXX XXX XXX */
	if (sc->sc_soft_req_cnt > sc->sc_soft_req_thresh) {
		cnmac_send_queue_flush_fetch(sc);
		cnmac_send_queue_flush(sc);
	}

	/* XXX XXX XXX */
	if (sc->sc_flush)
		cnmac_send_queue_flush_sync(sc);
	/* XXX XXX XXX */

	if_percpuq_enqueue(ifp->if_percpuq, m);

	return 0;

drop:
	/* XXX XXX XXX */
	if (sc->sc_soft_req_cnt > sc->sc_soft_req_thresh) {
		cnmac_send_queue_flush_fetch(sc);
	}
	/* XXX XXX XXX */

	return result;
}

static void
cnmac_recv_redir(struct ifnet *ifp, struct mbuf *m)
{
	struct cnmac_softc *rsc = ifp->if_softc;
	struct cnmac_softc *sc = NULL;
	int i, wdc = 0;

	for (i = 0; i < 3 /* XXX */; i++) {
		if (rsc->sc_redir & (1 << i))
			sc = cnmac_gsc[i];
	}

	if (sc == NULL) {
		m_freem(m);
		return;
	}
	cnmac_send_queue_flush_prefetch(sc);

	cnmac_send_queue_flush_fetch(sc);

	if (cnmac_send_queue_is_full(sc)) {
		m_freem(m);
		return;
	}
	if (sc->sc_soft_req_cnt > sc->sc_soft_req_thresh)
		cnmac_send_queue_flush(sc);

	if (cnmac_send(sc, m, &wdc)) {
		IF_DROP(&ifp->if_snd);
		m_freem(m);
	} else {
		octpko_op_doorbell_write(sc->sc_port, sc->sc_port, wdc);
		sc->sc_soft_req_cnt++;
	}

	if (sc->sc_flush)
		cnmac_send_queue_flush_sync(sc);
}

static inline void
cnmac_recv_intr(void *data, uint64_t *work)
{
	struct cnmac_softc *sc;
	int port;

	KASSERT(work != NULL);

	port = (work[1] & PIP_WQE_WORD1_IPRT) >> 42;

	KASSERT(port < GMX_PORT_NUNITS);

	sc = cnmac_gsc[port];

	KASSERT(sc != NULL);
	KASSERT(port == sc->sc_port);

	/* XXX process all work queue entries anyway */

	(void)cnmac_recv(sc, work);
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
	int s;

	s = splnet();
	/* XXX XXX XXX */
	if (sc->sc_soft_req_cnt > 0) {
		cnmac_send_queue_flush_prefetch(sc);
		cnmac_send_queue_flush_fetch(sc);
		cnmac_send_queue_flush(sc);
		cnmac_send_queue_flush_sync(sc);
	}
	/* XXX XXX XXX */

	/* XXX XXX XXX */
	/* ??? */
	timo = hz - (100 * sc->sc_ext_callback_cnt);
	if (timo < 10)
		 timo = 10;
	callout_schedule(&sc->sc_tick_free_ch, timo);
	/* XXX XXX XXX */
	splx(s);
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

/* ---- Odd nibble preamble workaround (software CRC processing) */

/* ---- sysctl */

static int	cnmac_sysctl_verify(SYSCTLFN_ARGS);
static int	cnmac_sysctl_pool(SYSCTLFN_ARGS);
static int	cnmac_sysctl_rd(SYSCTLFN_ARGS);

static int	cnmac_sysctl_pkocmdw0n2_num;
static int	cnmac_sysctl_pipdynrs_num;
static int	cnmac_sysctl_redir_num;
static int	cnmac_sysctl_pkt_pool_num;
static int	cnmac_sysctl_wqe_pool_num;
static int	cnmac_sysctl_cmd_pool_num;
static int	cnmac_sysctl_sg_pool_num;
static int	cnmac_sysctl_pktbuf_num;

/*
 * Set up sysctl(3) MIB, hw.cnmac.*.
 */
SYSCTL_SETUP(sysctl_octeon_eth, "sysctl cnmac subtree setup")
{
	int rc;
	int cnmac_sysctl_root_num;
	const struct sysctlnode *node;

	if ((rc = sysctl_createv(clog, 0, NULL, NULL,
		    0, CTLTYPE_NODE, "hw", NULL,
		    NULL, 0, NULL, 0, CTL_HW, CTL_EOL)) != 0) {
		goto err;
	}

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		    0, CTLTYPE_NODE, "cnmac",
		    SYSCTL_DESCR("cnmac interface controls"),
		    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}

	cnmac_sysctl_root_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		    CTLTYPE_INT, "pko_cmd_w0_n2",
		    SYSCTL_DESCR("PKO command WORD0 N2 bit"),
		    cnmac_sysctl_verify, 0,
		    &cnmac_param_pko_cmd_w0_n2,
		    0, CTL_HW, cnmac_sysctl_root_num, CTL_CREATE,
		    CTL_EOL)) != 0) {
		goto err;
	}

	cnmac_sysctl_pkocmdw0n2_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		    CTLTYPE_INT, "pip_dyn_rs",
		    SYSCTL_DESCR("PIP dynamic short in WQE"),
		    cnmac_sysctl_verify, 0,
		    &cnmac_param_pip_dyn_rs,
		    0, CTL_HW, cnmac_sysctl_root_num, CTL_CREATE,
		    CTL_EOL)) != 0) {
		goto err;
	}

	cnmac_sysctl_pipdynrs_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		    CTLTYPE_INT, "redir",
		    SYSCTL_DESCR("input port redirection"),
		    cnmac_sysctl_verify, 0,
		    &cnmac_param_redir,
		    0, CTL_HW, cnmac_sysctl_root_num, CTL_CREATE,
		    CTL_EOL)) != 0) {
		goto err;
	}

	cnmac_sysctl_redir_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		    CTLFLAG_PERMANENT,
		    CTLTYPE_INT, "pkt_pool",
		    SYSCTL_DESCR("packet pool available"),
		    cnmac_sysctl_pool, 0, NULL,
		    0, CTL_HW, cnmac_sysctl_root_num, CTL_CREATE,
		    CTL_EOL)) != 0) {
		goto err;
	}

	cnmac_sysctl_pkt_pool_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		    CTLFLAG_PERMANENT,
		    CTLTYPE_INT, "wqe_pool",
		    SYSCTL_DESCR("wqe pool available"),
		    cnmac_sysctl_pool, 0, NULL,
		    0, CTL_HW, cnmac_sysctl_root_num, CTL_CREATE,
		    CTL_EOL)) != 0) {
		goto err;
	}

	cnmac_sysctl_wqe_pool_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		    CTLFLAG_PERMANENT,
		    CTLTYPE_INT, "cmd_pool",
		    SYSCTL_DESCR("cmd pool available"),
		    cnmac_sysctl_pool, 0, NULL,
		    0, CTL_HW, cnmac_sysctl_root_num, CTL_CREATE,
		    CTL_EOL)) != 0) {
		goto err;
	}

	cnmac_sysctl_cmd_pool_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		    CTLFLAG_PERMANENT,
		    CTLTYPE_INT, "sg_pool",
		    SYSCTL_DESCR("sg pool available"),
		    cnmac_sysctl_pool, 0, NULL,
		    0, CTL_HW, cnmac_sysctl_root_num, CTL_CREATE,
		    CTL_EOL)) != 0) {
		goto err;
	}

	cnmac_sysctl_sg_pool_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
		    CTLFLAG_PERMANENT | CTLFLAG_READONLY,
		    CTLTYPE_INT, "pktbuf",
		    SYSCTL_DESCR("input packet buffer size on POW"),
		    cnmac_sysctl_rd, 0,
		    &cnmac_param_pktbuf,
		    0, CTL_HW, cnmac_sysctl_root_num, CTL_CREATE,
		    CTL_EOL)) != 0) {
		goto err;
	}

	cnmac_sysctl_pktbuf_num = node->sysctl_num;

	return;

err:
	aprint_error("%s: syctl_createv failed (rc = %d)\n", __func__, rc);
}

static int
cnmac_sysctl_verify(SYSCTLFN_ARGS)
{
	int error, v;
	struct sysctlnode node;
	struct cnmac_softc *sc;
	int i;
	int s;

	node = *rnode;
	v = *(int *)rnode->sysctl_data;
	node.sysctl_data = &v;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (node.sysctl_num == cnmac_sysctl_pkocmdw0n2_num) {
		if (v < 0 || v > 1)
			return EINVAL;
		*(int *)rnode->sysctl_data = v;
		return 0;
	}

	if (node.sysctl_num == cnmac_sysctl_pipdynrs_num) {
		if (v < 0 || v > 1)
			return EINVAL;
		*(int *)rnode->sysctl_data = v;
		s = splnet();
		for (i = 0; i < 3/* XXX */; i++) {
			sc = cnmac_gsc[i];	/* XXX */
			octpip_prt_cfg_enable(sc->sc_pip,
			    PIP_PRT_CFGN_DYN_RS, v);
		}
		splx(s);
		return 0;
	}

	if (node.sysctl_num == cnmac_sysctl_redir_num) {
		if (v & ~((0x7 << (4 * 0)) | (0x7 << (4 * 1)) | (0x7 << (4 * 2))))
			return EINVAL;
		*(int *)rnode->sysctl_data = v;
		s = splnet();
		for (i = 0; i < 3/* XXX */; i++) {
			struct ifnet *ifp;

			sc = cnmac_gsc[i];	/* XXX */
			ifp = &sc->sc_ethercom.ec_if;

			sc->sc_redir
			    = (cnmac_param_redir >> (4 * i)) & 0x7;
			if (sc->sc_redir == 0) {
				if (ISSET(ifp->if_flags, IFF_PROMISC)) {
					CLR(ifp->if_flags, IFF_PROMISC);
					cnmac_mii_statchg(ifp);
					/* octgmx_set_filter(sc->sc_gmx_port); */
				}
				ifp->_if_input = ether_input;
			}
			else {
				if (!ISSET(ifp->if_flags, IFF_PROMISC)) {
					SET(ifp->if_flags, IFF_PROMISC);
					cnmac_mii_statchg(ifp);
					/* octgmx_set_filter(sc->sc_gmx_port); */
				}
				ifp->_if_input = cnmac_recv_redir;
			}
		}
		splx(s);
		return 0;
	}

	return EINVAL;
}

static int
cnmac_sysctl_pool(SYSCTLFN_ARGS)
{
	int error, newval = 0;
	struct sysctlnode node;
	int s;

	node = *rnode;
	node.sysctl_data = &newval;
	s = splnet();
	if (node.sysctl_num == cnmac_sysctl_pkt_pool_num) {
		error = octfpa_available_fpa_pool(&newval,
		    OCTEON_POOL_NO_PKT);
	} else if (node.sysctl_num == cnmac_sysctl_wqe_pool_num) {
		error = octfpa_available_fpa_pool(&newval,
		    OCTEON_POOL_NO_WQE);
	} else if (node.sysctl_num == cnmac_sysctl_cmd_pool_num) {
		error = octfpa_available_fpa_pool(&newval,
		    OCTEON_POOL_NO_CMD);
	} else if (node.sysctl_num == cnmac_sysctl_sg_pool_num) {
		error = octfpa_available_fpa_pool(&newval,
		    OCTEON_POOL_NO_SG);
	} else {
		splx(s);
		return EINVAL;
	}
	splx(s);
	if (error)
		return error;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	return 0;
}

static int
cnmac_sysctl_rd(SYSCTLFN_ARGS)
{
	int error, v;
	struct sysctlnode node;
	int s;

	node = *rnode;
	v = *(int *)rnode->sysctl_data;
	node.sysctl_data = &v;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp != NULL)
		return error;

	if (node.sysctl_num == cnmac_sysctl_pktbuf_num) {
		uint64_t tmp;
		int n;

		s = splnet();
		tmp = octfpa_query(0);
		n = (int)tmp;
		splx(s);
		*(int *)rnode->sysctl_data = n;
		cnmac_param_pktbuf = n;
		*(int *)oldp = n;
		return 0;
	}

	return EINVAL;
}
