
void	cs_process_rx_dma __P((struct cs_softc *));
void	cs_isa_dma_chipinit __P((struct cs_softc *));
void	cs_isa_dma_attach __P((struct cs_softc *));

struct cs_softc_isa {
	struct cs_softc sc_cs;

	isa_chipset_tag_t sc_ic;	/* ISA chipset */
	int	sc_drq;			/* DRQ line */

	bus_size_t sc_dmasize;		/* DMA size (16k or 64k) */
	bus_addr_t sc_dmaaddr;		/* DMA address */
	caddr_t	sc_dmabase;		/* base DMA address (KVA) */
	caddr_t	sc_dmacur;		/* current DMA address (KVA) */
};
