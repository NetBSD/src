/*	$NetBSD: if_cnmacvar.h,v 1.5 2022/09/18 11:38:48 thorpej Exp $	*/

#undef DEBUG
#ifdef DEBUG
#define	dprintf printf
#else
#define	dprintf(...)
#endif

#define	IS_MAC_MULTICASTBIT(addr) \
        ((addr)[0] & 0x01)

#define	SEND_QUEUE_SIZE		(32)
#define	GATHER_QUEUE_SIZE	(1024)
#define	FREE_QUEUE_SIZE		GATHER_QUEUE_SIZE
#define	RECV_QUEUE_SIZE		(GATHER_QUEUE_SIZE * 2)

/* Number of mbufs per port to keep in the packet pool */
#define	CNMAC_MBUFS_PER_PORT	256

/* XXX MUST BE REPLACED WITH BUS_DMA!!! */
paddr_t kvtophys(vaddr_t);
/* XXX MUST BE REPLACED WITH BUS_DMA!!! */

struct _send_queue_entry;
struct octsmi_softc;
struct octgmx_port_softc;
struct octipd_softc;
struct octpip_softc;
struct octpko_softc;
struct octpow_softc;

extern struct octpow_softc	octpow_softc;

struct cnmac_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_regt;
	bus_dma_tag_t		sc_dmat;

	void			*sc_ih;

	struct octpip_softc	*sc_pip;
	struct octipd_softc	*sc_ipd;
	struct octpko_softc	*sc_pko;
	struct octsmi_softc	*sc_smi;
	struct octgmx_softc	*sc_gmx;
	struct octgmx_port_softc
				*sc_gmx_port;
	struct octpow_softc	*sc_pow;

	struct ethercom		sc_ethercom;
	struct mii_data		sc_mii;

	void			*sc_sdhook;

	struct callout		sc_tick_misc_ch;
	struct callout		sc_tick_free_ch;

	int64_t			sc_soft_req_cnt;
	int64_t			sc_soft_req_thresh;
	int64_t			sc_hard_done_cnt;
	int			sc_flush;
	int			sc_prefetch;
	SIMPLEQ_HEAD(, _send_queue_entry)
				sc_sendq;
	uint64_t		sc_ext_callback_cnt;
	bool			sc_txbusy;

	uint32_t		sc_port;
	uint32_t		sc_port_type;
	uint32_t		sc_init_flag;
	int			sc_powgroup;

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

	struct octfau_desc	sc_fau_done;
	struct octpko_cmdptr_desc
				sc_cmdptr;

	size_t			sc_ip_offset;

	struct timeval		sc_rxerr_log_last;

	int			sc_quirks;
};
