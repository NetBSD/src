
/*
 * Do an internal open.
 */
static void
zsiopen(tp)
	struct tty *tp;
{

	(void) zsparam(tp, &tp->t_termios);
	ttsetwater(tp);
	tp->t_state = TS_ISOPEN | TS_CARR_ON;
}

/*
 * Do an internal close.  Eventually we should shut off the chip when both
 * ports on it are closed.
 */
static void
zsiclose(tp)
	struct tty *tp;
{

	ttylclose(tp, 0);	/* ??? */
	ttyclose(tp);		/* ??? */
	tp->t_state = 0;
}


/*
 * Put a channel in a known state.  Interrupts may be left disabled
 * or enabled, as desired.  (Used only by kgdb?)
 */
static void
zs_reset(cs, inten, speed)
	struct zs_chanstate *cs;
	int inten, speed;
{
	int tconst;

	bcopy(zs_init_reg, cs->cs_preg, 16);	/* XXX */

	if (inten == 0) {
		cs->cs_preg[1] = 0;
	}

	tconst = BPS_TO_TCONST(cs->cs_pclk_div16, speed);
	cs->cs_preg[12] = tconst;
	cs->cs_preg[13] = tconst >> 8;

	zs_loadchannelregs(cs);
}

end_of_close()
{
	/* Reset the speed if we're doing kgdb on this port */
	/* i.e. go back to kgdb parameters */
	if (cs->cs_kgdb) {
		tp->t_ispeed = tp->t_ospeed = kgdb_rate;
		(void) zsparam(tp, &tp->t_termios);
	}
}

/*
 * Functions to handle a ZS line that's doing KGDB
 */

static int
zsgdb_rxint(cs)
	register struct zs_chanstate *cs;
{
	register volatile struct zschan *zc = XXX;
	register int c, i, rr0;

	/* Read the input. */
	c = zc->zc_data;
	ZS_DELAY();

#ifdef KGDB
	if (c == FRAME_START && cs->cs_kgdb && 
	    (cs->cs_ttyp->t_state & TS_ISOPEN) == 0) {
		zskgdb(cs->cs_unit);
		c = 0;
		goto clearit;
	}
#endif


clearit:
	/* clear receive error & interrupt condition */
	zc->zc_csr = ZSWR0_RESET_ERRORS;
	ZS_DELAY();
	zc->zc_csr = ZSWR0_CLR_INTR;
	ZS_DELAY();
	return (c);
}



#ifdef KGDB

/*
 * KGDB support
 */

/*
 * The kgdb zs port, if any, was altered at boot time (see zs_kgdb_init).
 * Pick up the current speed and character size and restore the original
 * speed.
 */
static void
zs_checkkgdb(int unit, struct zs_chanstate *cs, struct tty *tp)
{

	if (kgdb_dev == makedev(ZSMAJOR, unit)) {
		tp->t_ispeed = tp->t_ospeed = kgdb_rate;
		tp->t_cflag = CS8;
		cs->cs_kgdb = 1;
		cs->cs_speed = zs_kgdb_savedspeed;
		(void) zsparam(tp, &tp->t_termios);
	}
}

/*
 * KGDB framing character received: enter kernel debugger.  This probably
 * should time out after a few seconds to avoid hanging on spurious input.
 */
zskgdb(unit)
	int unit;
{

	printf("zs%d%c: kgdb interrupt\n", unit >> 1, (unit & 1) + 'a');
	kgdb_connect(1);
}

/*
 * Get a character from the given kgdb channel.  Called at splhigh().
 * XXX - Add delays, or combine with zscngetc()...
 */
static int
zs_kgdb_getc(arg)
	void *arg;
{
	register volatile struct zschan *zc = (volatile struct zschan *)arg;
	register int c, rr0;

	do {
		rr0 = *(cs->cs_reg_csr);
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);
	c = zc->zc_data;
	ZS_DELAY();
	return (c);
}

/*
 * Put a character to the given kgdb channel.  Called at splhigh().
 */
static void
zs_kgdb_putc(arg, c)
	void *arg;
	int c;
{
	register volatile struct zschan *zc = (volatile struct zschan *)arg;
	register int c, rr0;

	do {
		rr0 = *(cs->cs_reg_csr);
		ZS_DELAY();
	} while ((rr0 & ZSRR0_TX_READY) == 0);
	zc->zc_data = c;
	ZS_DELAY();
}

/*
 * Set up for kgdb; called at boot time before configuration.
 * KGDB interrupts will be enabled later when zs0 is configured.
 */
void
zs_kgdb_init()
{
	volatile struct zsdevice *addr;
	volatile struct zschan *zc;
	int unit, zs;

	if (major(kgdb_dev) != ZSMAJOR)
		return;
	unit = minor(kgdb_dev);
	/*
	 * Unit must be 0 or 1 (zs0).
	 */
	if ((unsigned)unit >= ZS_KBD) {
		printf("zs_kgdb_init: bad minor dev %d\n", unit);
		return;
	}
	zs = unit >> 1;
	unit &= 1;

	if (zsaddr[0] == NULL)
		panic("kbdb_attach: zs0 not yet mapped");
	addr = zsaddr[0];

	zc = (unit == 0) ?
		&addr->zs_chan[ZS_CHAN_A] :
		&addr->zs_chan[ZS_CHAN_B];
	zs_kgdb_savedspeed = zs_getspeed(zc);
	printf("zs_kgdb_init: attaching zs%d%c at %d baud\n",
	    zs, unit + 'a', kgdb_rate);
	zs_reset(zc, 1, kgdb_rate);
	kgdb_attach(zs_kgdb_getc, zs_kgdb_putc, (void *)zc);
}

#endif	/* KGDB */
