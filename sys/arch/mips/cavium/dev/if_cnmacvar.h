/*	$NetBSD: if_cnmacvar.h,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

#undef DEBUG
#undef TENBASET_DBG
#undef REGISTER_DUMP
#ifdef DEBUG
#define dprintf printf
#else
#define dprintf(...)
#endif

#define IS_MAC_MULTICASTBIT(addr) \
        ((addr)[0] & 0x01)

#define SEND_QUEUE_SIZE		(32)
#define GATHER_QUEUE_SIZE	(1024)
#define FREE_QUEUE_SIZE		GATHER_QUEUE_SIZE
#define RECV_QUEUE_SIZE		(GATHER_QUEUE_SIZE * 2)

#ifdef OCTEON_ETH_FIXUP_ODD_NIBBLE_DYNAMIC
#define PROC_NIBBLE_SOFT_THRESHOLD 2000
#endif

/* XXX MUST BE REPLACED WITH BUS_DMA!!! */
paddr_t kvtophys(vaddr_t);
/* XXX MUST BE REPLACED WITH BUS_DMA!!! */

struct _send_queue_entry;
struct octeon_pow_softc;
struct octeon_pip_softc;
struct octeon_ipd_softc;
struct octeon_pko_softc;
struct octeon_asx_softc;
struct octeon_smi_softc;
struct octeon_gmx_port_softc;
struct octeon_pow_softc;

extern struct octeon_pow_softc	octeon_pow_softc;

struct octeon_eth_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_regt;
	bus_dma_tag_t		sc_dmat;

#if 1
	/* XXX backward compatibility; debugging purpose only */
	bus_space_handle_t      sc_gmx_regh;
	bus_space_handle_t      sc_gmx_port_regh;
	bus_space_handle_t      sc_asx_regh;
	bus_space_handle_t      sc_pow_regh;
	bus_space_handle_t      sc_fpa_regh;
	bus_space_handle_t      sc_smi_regh;
	bus_space_handle_t      sc_pip_regh;
#endif

	void			*sc_pow_recv_ih;
	struct octeon_pip_softc	*sc_pip;
	struct octeon_ipd_softc	*sc_ipd;
	struct octeon_pko_softc	*sc_pko;
	struct octeon_asx_softc	*sc_asx;
	struct octeon_smi_softc	*sc_smi;
	struct octeon_gmx_softc	*sc_gmx;
	struct octeon_gmx_port_softc
				*sc_gmx_port;
	struct octeon_pow_softc
				*sc_pow;

	struct ethercom		sc_ethercom;
	struct mii_data		sc_mii;

	void			*sc_sdhook;

	struct callout		sc_tick_misc_ch;
	struct callout		sc_tick_free_ch;

#ifdef OCTEON_ETH_INTR_FEEDBACK
	struct callout		sc_resume_ch;
#endif

	int64_t			sc_soft_req_cnt;
	int64_t			sc_soft_req_thresh;
	int64_t			sc_hard_done_cnt;
	int			sc_flush;
	int			sc_prefetch;
	SIMPLEQ_HEAD(, _send_queue_entry)
				sc_sendq;
	uint64_t		sc_ext_callback_cnt;

	uint32_t		sc_port;
	uint32_t		sc_port_type;
	uint32_t		sc_init_flag;

	/*
	 * Redirection - received (input) packets are redirected (directly sent)
	 * to another port.  Only meant to test hardware + driver performance.
	 *
	 *  0	- disabled
	 * >0	- redirected to ports that correspond to bits
	 *		0b001 (0x1)	- Port 0
	 *		0b010 (0x2)	- Port 1
	 *		0b100 (0x4)	- Port 2
	 */
	int			sc_redir;

	struct octeon_fau_desc	sc_fau_done;
	struct octeon_pko_cmdptr_desc
				sc_cmdptr;

	size_t			sc_ip_offset;

	struct timeval		sc_rate_recv_check_link_last;
	struct timeval		sc_rate_recv_check_link_cap;
	struct timeval		sc_rate_recv_check_jumbo_last;
	struct timeval		sc_rate_recv_check_jumbo_cap;
	struct timeval		sc_rate_recv_check_code_last;
	struct timeval		sc_rate_recv_check_code_cap;
	struct timeval		sc_rate_recv_fixup_odd_nibble_short_last;
	struct timeval		sc_rate_recv_fixup_odd_nibble_short_cap;
	struct timeval		sc_rate_recv_fixup_odd_nibble_preamble_last;
	struct timeval		sc_rate_recv_fixup_odd_nibble_preamble_cap;
	struct timeval		sc_rate_recv_fixup_odd_nibble_crc_last;
	struct timeval		sc_rate_recv_fixup_odd_nibble_crc_cap;
#ifdef OCTEON_ETH_DEBUG
	struct timeval		sc_rate_recv_fixup_odd_nibble_addr_last;
	struct timeval		sc_rate_recv_fixup_odd_nibble_addr_cap;
#endif
	int			sc_quirks;
#ifdef OCTEON_ETH_DEBUG
	struct evcnt		sc_ev_rx;
	struct evcnt		sc_ev_rxint;
	struct evcnt		sc_ev_rxrs;
	struct evcnt		sc_ev_rxbufpkalloc;
	struct evcnt		sc_ev_rxbufpkput;
	struct evcnt		sc_ev_rxbufwqalloc;
	struct evcnt		sc_ev_rxbufwqput;
	struct evcnt		sc_ev_rxerrcode;
	struct evcnt		sc_ev_rxerrfix;
	struct evcnt		sc_ev_rxerrjmb;
	struct evcnt		sc_ev_rxerrlink;
	struct evcnt		sc_ev_rxerroff;
	struct evcnt		sc_ev_rxonperrshort;
	struct evcnt		sc_ev_rxonperrpreamble;
	struct evcnt		sc_ev_rxonperrcrc;
	struct evcnt		sc_ev_rxonperraddress;
	struct evcnt		sc_ev_rxonponp;
	struct evcnt		sc_ev_rxonpok;
	struct evcnt		sc_ev_tx;
	struct evcnt		sc_ev_txadd;
	struct evcnt		sc_ev_txbufcballoc;
	struct evcnt		sc_ev_txbufcbget;
	struct evcnt		sc_ev_txbufgballoc;
	struct evcnt		sc_ev_txbufgbget;
	struct evcnt		sc_ev_txbufgbput;
	struct evcnt		sc_ev_txdel;
	struct evcnt		sc_ev_txerr;
	struct evcnt		sc_ev_txerrcmd;
	struct evcnt		sc_ev_txerrgbuf;
	struct evcnt		sc_ev_txerrlink;
	struct evcnt		sc_ev_txerrmkcmd;
#endif
};
