/* $NetBSD: atppc_isadma.h,v 1.1 2004/01/25 11:35:46 jdolecek Exp $ */

/* atppc ISA DMA functions */
int atppc_isadma_setup(struct atppc_softc *, isa_chipset_tag_t, int);
int atppc_isadma_start(isa_chipset_tag_t, int, void *, u_int, 
	u_int8_t);
int atppc_isadma_finish(isa_chipset_tag_t, int);
int atppc_isadma_abort(isa_chipset_tag_t, int);
int atppc_isadma_malloc(isa_chipset_tag_t, int, caddr_t *, bus_addr_t *, 
	bus_size_t);
void atppc_isadma_free(isa_chipset_tag_t, int, caddr_t *, bus_addr_t *, 
	bus_size_t);
