/* $NetBSD: ascvar.h,v 1.1.2.1 1998/10/15 02:48:59 nisimura Exp $ */

struct asc_softc {
	struct ncr53c9x_softc sc_ncr53c9x;	/* glue to MI code */

	volatile u_int32_t *sc_base;		/* base of IOASIC or PMAZ */
	volatile u_int8_t *sc_reg;		/* the registers */
	void	*sc_cookie;			/* intr. handling cookie */

	/*
	 * DMA bookkeeping information.
	 */
	int	sc_active;			/* DMA active ? */
	int	sc_iswrite;			/* DMA into main memory? */
	int	sc_datain;
	size_t	sc_dmasize;
	caddr_t *sc_dmaaddr;
	size_t	*sc_dmalen;

	/* IOASIC private */
	volatile u_int32_t *sc_ssr;
	volatile u_int32_t *sc_scsi_scr;
	volatile u_int32_t *sc_scsi_dmaptr;
	volatile u_int32_t *sc_scsi_nextptr;
	volatile u_int32_t *sc_scsi_sdr0;
	volatile u_int32_t *sc_scsi_sdr1;

	/* PMAZ-A private */
	volatile u_int32_t *sc_tc_dmar;		/* TURBOchannel DMR */
	caddr_t sc_bounce, sc_target;		/* for data trampoline */

#if 0
	bus_space_tag_t sc_bst;			/* to frob ASC regs */
	bus_space_handle_t sc_bsh;
#endif
};
