#define	FDC_BSIZE	512
#define	FDC_MAXIOSIZE	NBPG	/* XXX should be MAXBSIZE */

#define FDC_NSTATUS	10

#if !defined(LOCORE)
struct fdcio {
	/*
	 * 82072 (sun4c) and 82077 (sun4m) controllers have different
	 * register layout; so we cache some here.
	 */
	volatile u_int8_t	*fdcio_reg_msr;
	volatile u_int8_t	*fdcio_reg_fifo;
	volatile u_int8_t	*fdcio_reg_dor;	/* 82077 only */

	/*
	 * Interrupt state.
	 */
	int	fdcio_istate;

	/*
	 * IO state.
	 */
	char	*fdcio_data;		/* pseudo-dma data */
	int	fdcio_tc;		/* pseudo-dma Terminal Count */
	u_char	fdcio_status[FDC_NSTATUS];	/* copy of registers */
	int	fdcio_nstat;		/* # of valid status bytes */
};
#endif /* LOCORE */

/* istate values */
#define ISTATE_IDLE		0	/* No HW interrupt expected */
#define ISTATE_SPURIOUS		1	/* Spurious HW interrupt detected */
#define ISTATE_SENSEI		2	/* Do SENSEI on next HW interrupt */
#define ISTATE_DMA		3	/* Pseudo-DMA in progress */

#define FDIOCEJECT	_IO('f', 24)

