/*
 * Ethernet software status per interface.
 */

/* Per interface statistics */
struct	lestats {
	long	lexints;	/* transmitter interrupts */
	long	lerints;	/* receiver interrupts */
	long	lerbufs;	/* total buffers received during interrupts */
	long	lerhits;	/* times current rbuf was full */
	long	lerscans;	/* rbufs scanned before finding first full */
};

/*
 * Each interface is referenced by a network interface structure,
 * le_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface,
 * its address, ...
 */

struct	le_softc {
	struct	device sc_dev;		/* base device */
	void    *sc_machdep;		/* machine dependent pointer */
	struct	evcnt sc_intrcnt;	/* # of interrupts, per le */
	struct	evcnt sc_errcnt;	/* # of errors, per le */
	struct	arpcom sc_ac;		/* common Ethernet structures */
#define	sc_if	sc_ac.ac_if		/* network-visible interface */
#define	sc_addr	sc_ac.ac_enaddr		/* hardware Ethernet address */
	volatile struct	lereg1 *sc_r1;	/* LANCE registers */
	volatile struct	lereg2 *sc_r2;	/* dual-port RAM */
	int	sc_rmd;			/* predicted next rmd to process */
	int	sc_runt;
	int	sc_jab;
	int	sc_merr;
	int	sc_babl;
	int	sc_cerr;
	int	sc_miss;
	int	sc_xint;
	int	sc_xown;
	int	sc_uflo;
	int	sc_rxlen;
	int	sc_rxoff;
	int	sc_txoff;
	int	sc_busy;
	short	sc_iflags;
	struct	lestats sc_lestats;	/* per interface statistics */
};
