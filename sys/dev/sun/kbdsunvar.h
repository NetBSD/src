/*
 * Keyboard driver - middle layer for sun keyboard off a serial line.
 * This code is used by kbd_zs and sunkbd drivers.
 */

/*
 * How many input characters we can buffer.
 * The port-specific var.h may override this.
 * Note: must be a power of two!
 */
#define	KBD_RX_RING_SIZE	256
#define KBD_RX_RING_MASK	(KBD_RX_RING_SIZE - 1)

/*
 * Output buffer.  Only need a few chars.
 */
#define	KBD_TX_RING_SIZE	16
#define KBD_TX_RING_MASK	(KBD_TX_RING_SIZE - 1)
/*
 * Keyboard serial line speed defaults to 1200 bps.
 */
#define KBD_DEFAULT_BPS		1200
#define KBD_RESET_TIMO		1000 /* mS. */


struct kbd_sun_softc {
	/* upper layer (also inherits struct device) */
	struct kbd_softc k_kbd;

	union {
		void *ku_priv;
		struct zs_chanstate *ku_cs;
	} k_u;
#define k_priv	k_u.ku_priv
#define	k_cs	k_u.ku_cs

	/*
	 * The deviopen and deviclose routines are provided by the
	 * underlying lower level driver and used as a back door when
	 * opening and closing the internal device.
	 */
	int (*k_deviopen)(struct device *, int);
	int (*k_deviclose)(struct device *, int);

	/*
	 * Callback provided by the lower layer (actual device driver).
	 * Middle layer uses it to send commands to sun keyboard.
	 */
	void (*k_write_data)(struct kbd_sun_softc *, int);

	/* Was initialized once. */
	int k_isopen;

	/*
	 * Magic sequence stuff (Stop-A, aka L1-A).
	 * XXX: convert to cnmagic(9).
	 */
	char k_magic1_down;
	u_char k_magic1;	/* L1 */
	u_char k_magic2;	/* A */

	/* Autorepeat for sun keyboards is handled in software */
	int k_repeat_start; 	/* initial delay */
	int k_repeat_step;  	/* inter-char delay */
	int k_repeatsym;	/* repeating symbol */
	int k_repeating;	/* callout is active (use callout_active?) */
	struct callout k_repeat_ch;

	/* Expecting ID or layout byte from keyboard */
	int k_expect;
#define	KBD_EXPECT_IDCODE	1
#define	KBD_EXPECT_LAYOUT	2

	/* Flags to communicate with kbd_softint() */
	volatile int k_intr_flags;
#define	INTR_RX_OVERRUN 1
#define INTR_TX_EMPTY   2
#define INTR_ST_CHECK   4

	/* Transmit state */
	volatile int k_txflags;
#define	K_TXBUSY 1
#define K_TXWANT 2

	/*
	 * The transmit ring buffer.
	 */
	volatile u_int k_tbget;	/* transmit buffer `get' index */
	volatile u_int k_tbput;	/* transmit buffer `put' index */
	u_char k_tbuf[KBD_TX_RING_SIZE]; /* data */

	/*
	 * The receive ring buffer.
	 */
	u_int k_rbget;		/* ring buffer `get' index */
	volatile u_int k_rbput; /* ring buffer `put' index */
	u_short	k_rbuf[KBD_RX_RING_SIZE]; /* rr1, data pairs */
};

/* Middle layer methods exported to the upper layer. */
extern struct kbd_ops kbd_ops_sun;

/* Methods for lower layer to call. */
extern void	kbd_sun_input(struct kbd_sun_softc *k, int);
extern void	kbd_sun_output(struct kbd_sun_softc *k, int c);
extern void	kbd_sun_start_tx(struct kbd_sun_softc *k);
