/*	$NetBSD: if_ievar.h,v 1.16 2019/09/13 07:55:06 msaitoh Exp $	*/

/*
 * Machine-dependent glue for the Intel Ethernet (ie) driver.
 */

#define	MXFRAMES	128	/* max number of frames to allow for receive */
#define	MXRXBUF 	192	/* max number of buffers to allocate */
#define	IE_RBUF_SIZE	256	/* size of each buffer, MUST BE POWER OF TWO */
#define	NTXBUF		2	/* number of transmit buffer/command pairs */
#define	IE_TBUF_SIZE	(3*512)	/* length of transmit buffer */

enum ie_hardware {
	IE_VME,			/* multibus to VME ie card */
	IE_OBIO,		/* on board */
	IE_VME3E,		/* sun 3e VME card */
	IE_UNKNOWN
};

/*
 * Ethernet status, per interface.
 *
 * hardware addresses/sizes to know (all KVA):
 *   sc_iobase = base of chip's 24 bit address space
 *   sc_maddr  = base address of chip RAM as stored in ie_base of iscp
 *   sc_msize  = size of chip's RAM
 *   sc_reg    = address of card dependent registers
 *
 * the chip uses two types of pointers: 16 bit and 24 bit
 *   16 bit pointers are offsets from sc_maddr/ie_base
 *      KVA(16 bit offset) = offset + sc_maddr
 *   24 bit pointers are offset from sc_iobase in KVA
 *      KVA(24 bit address) = address + sc_iobase
 *
 * on the vme/multibus we have the page map to control where ram appears
 * in the address space.   we choose to have RAM start at 0 in the
 * 24 bit address space.   this means that sc_iobase == sc_maddr!
 * to get the phyiscal address of the board's RAM you must take the
 * top 12 bits of the physical address of the register address
 * and or in the 4 bits from the status word as bits 17-20 (remember that
 * the board ignores the chip's top 4 address lines).
 * For example:
 *   if the register is @ 0xffe88000, then the top 12 bits are 0xffe00000.
 *   to get the 4 bits from the status word just do status & IEVME_HADDR.
 *   suppose the value is "4".   Then just shift it left 16 bits to get
 *   it into bits 17-20 (e.g. 0x40000).    Then or it to get the
 *   address of RAM (in our example: 0xffe40000).   see the attach routine!
 *
 * In the onboard ie interface, the 24 bit address space is hardwired
 * to be 0xff000000 -> 0xffffffff of KVA.   this means that sc_iobase
 * will be 0xff000000.   sc_maddr will be where ever we allocate RAM
 * in KVA.    note that since the SCP is at a fixed address it means
 * that we have to use some memory at a fixed KVA for the SCP.
 * The Sun PROM leaves a page for us at the end of KVA space.
 */
struct ie_softc {
	device_t sc_dev;	/* device structure */

	struct ethercom sc_ethercom;/* system ethercom structure */
#define	sc_if	sc_ethercom.ec_if 		/* network-visible interface */

	/* XXX: This is used only during attach. */
	uint8_t sc_addr[ETHER_ADDR_LEN];
	uint8_t sc_pad1[2];

	int     sc_debug;	/* See IEDEBUG */

	/* card dependent functions: */
	void    (*reset_586)(struct ie_softc *);
	void    (*chan_attn)(struct ie_softc *);
	void    (*run_586)  (struct ie_softc *);
	void	*(*sc_memcpy)(void *, const void *, size_t);
	void	*(*sc_memset)(void *, int, size_t);

	void *sc_iobase;	/* KVA of base of 24bit addr space */
	void *sc_maddr;	/* KVA of base of chip's RAM */
	u_int   sc_msize;	/* how much RAM we have/use */
	void *sc_reg;		/* KVA of card's register */

	enum ie_hardware hard_type;	/* card type */

	int     want_mcsetup;	/* flag for multicast setup */
	u_short     promisc;	/* are we in promisc mode? */

	int ntxbuf;       /* number of tx frames/buffers */
	int nframes;      /* number of recv frames in use */
	int nrxbuf;       /* number of recv buffs in use */

	/*
	 * pointers to the 3 major control structures
	 */
	volatile struct ie_sys_conf_ptr *scp;
	volatile struct ie_int_sys_conf_ptr *iscp;
	volatile struct ie_sys_ctl_block *scb;

	/*
	 * pointer and size of a block of KVA where the buffers
	 * are to be allocated from
	 */
	uint8_t *buf_area;
	int     buf_area_sz;

	/*
	 * Transmit commands, descriptors, and buffers
	 */
	volatile struct ie_xmit_cmd *xmit_cmds[NTXBUF];
	volatile struct ie_xmit_buf *xmit_buffs[NTXBUF];
	char *xmit_cbuffs[NTXBUF];
	int xmit_busy;
	int xmit_free;
	int xchead, xctail;

	/*
	 * Receive frames, descriptors, and buffers
	 */
	volatile struct ie_recv_frame_desc *rframes[MXFRAMES];
	volatile struct ie_recv_buf_desc *rbuffs[MXRXBUF];
	char *cbuffs[MXRXBUF];
	int     rfhead, rftail, rbhead, rbtail;

	/* Multi-cast stuff */
	int     mcast_count;
	struct ie_en_addr mcast_addrs[MAXMCAST + 1];
};


extern void    ie_attach(struct ie_softc *);
extern int  ie_intr(void *);
