/*	$NetBSD: gemini_lpc.c,v 1.3 2008/11/15 05:48:34 cliff Exp $	*/

#include "opt_gemini.h"
#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gemini_lpc.c,v 1.3 2008/11/15 05:48:34 cliff Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <arch/arm/gemini/gemini_lpcvar.h>
#include <arch/arm/gemini/gemini_lpchcvar.h>
#include <arch/arm/gemini/gemini_reg.h>


/* XXX these should be in lpcreg.h or it8721reg.h */
#define IT8712_CFGCTL		0x02
# define CFGCTL_WAITKEY		__BIT(1)
#define IT8712_LDN		0x07
#define IT8712_CHIPID1		0x20
#define IT8712_CHIPID2		0x21
#define IT8712_CSCV		0x22
# define CSCV_VERSION		__BITS(3,0);
#define IT8712_CLKSEL		0x23
#define IT8712_SWSUSPEND	0x24
#define IT8712_ADDR		0x2e
#define IT8712_DATA		0x2f

static int  gemini_lpc_match(struct device *, struct cfdata *, void *);
static void gemini_lpc_attach(struct device *, struct device *, void *);
static int  gemini_lpc_search(device_t, cfdata_t, const int *, void *);
static int  gemini_lpc_busprint(void *, const char *);

static uint8_t it8712_pnp_read(lpctag_t, int, uint);
static void    it8712_pnp_write(lpctag_t, int, uint, uint8_t);
static void    it8712_pnp_enter(lpctag_t);
static void    it8712_pnp_exit(lpctag_t);
static void   *it8712_intr_establish(lpctag_t, uint, int, int, int (*)(void *), void *);
static void    it8712_intr_disestablish(lpctag_t, void *);

CFATTACH_DECL_NEW(lpc, sizeof(struct gemini_lpc_softc),
    gemini_lpc_match, gemini_lpc_attach, NULL, NULL);

gemini_lpc_bus_ops_t gemini_lpc_bus_ops = {
	it8712_pnp_read,
	it8712_pnp_write,
	it8712_pnp_enter,
	it8712_pnp_exit,
	it8712_intr_establish,
	it8712_intr_disestablish,
};


static int
gemini_lpc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct gemini_lpc_attach_args *lpc = aux;

	if (lpc->lpc_addr == LPCCF_ADDR_DEFAULT)
		panic("lpc must have addr specified in config.");

	return 1;
}

static void
gemini_lpc_attach(struct device *parent, struct device *self, void *aux)
{
	gemini_lpc_softc_t *sc = device_private(self);
	struct gemini_lpchc_attach_args *lpchc = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint8_t id1, id2, vers, clk, susp;

	sc->sc_addr = lpchc->lpchc_addr;
#if 0
	sc->sc_size = lpchc->lpchc_size;
#else
	/*
	 * we just map the configuration registers
	 * child devices can map their own I/O
	 */
	sc->sc_size = 4096;
#endif

	iot = lpchc->lpchc_iot;
	if (bus_space_map(iot, sc->sc_addr, sc->sc_size, 0, &ioh))
		panic("%s: Cannot map registers", device_xname(self));
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	it8712_pnp_enter(sc);
	id1  = it8712_pnp_read(sc, GEMINI_LPC_LDN_ALL, IT8712_CHIPID1);
	id2  = it8712_pnp_read(sc, GEMINI_LPC_LDN_ALL, IT8712_CHIPID2);
	vers = it8712_pnp_read(sc, GEMINI_LPC_LDN_ALL, IT8712_CSCV);
	vers &= CSCV_VERSION;
	clk  = it8712_pnp_read(sc, GEMINI_LPC_LDN_ALL, IT8712_CLKSEL);
	susp = it8712_pnp_read(sc, GEMINI_LPC_LDN_ALL, IT8712_SWSUSPEND);
	it8712_pnp_exit(sc);

	aprint_normal("\n%s: chip ID %x%x, version %d",
		device_xname(self), id1, id2, vers);
	aprint_normal("\n%s: clksel %#x, susp %#x ",
		device_xname(self), clk, susp);

	sc->sc_lpchctag = lpchc->lpchc_tag;
	sc->sc_bus_ops  = &gemini_lpc_bus_ops;

	aprint_normal("\n");
	aprint_naive("\n"); 

	/*
	 * attach the rest of our devices
	 */
	config_search_ia(gemini_lpc_search, self, "lpc", NULL);
}

static int
gemini_lpc_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	gemini_lpc_softc_t *sc = device_private(parent);
	gemini_lpc_attach_args_t lpc;

	lpc.lpc_ldn      = cf->cf_loc[LPCCF_LDN];
	lpc.lpc_addr     = cf->cf_loc[LPCCF_ADDR];
	lpc.lpc_size     = cf->cf_loc[LPCCF_SIZE];
	lpc.lpc_intr     = cf->cf_loc[LPCCF_INTR];
	lpc.lpc_iot      = sc->sc_iot;
	lpc.lpc_tag      = sc;
	lpc.lpc_base     = sc->sc_addr;

	if (config_match(parent, cf, &lpc)) {
		config_attach(parent, cf, &lpc, gemini_lpc_busprint);
		return 0;			/* love it */
	}

	return UNCONF;				/* hate it */
}

static int
gemini_lpc_busprint(void *aux, const char *name)
{
        struct gemini_lpc_attach_args *lpc = aux;

	if (lpc->lpc_ldn != LPCCF_LDN_DEFAULT)
		aprint_normal(" ldn %d", lpc->lpc_ldn);
	if (lpc->lpc_addr != LPCCF_ADDR_DEFAULT)
		aprint_normal(" addr %#lx", lpc->lpc_addr);
	if (lpc->lpc_size != LPCCF_SIZE_DEFAULT)
		aprint_normal(" size %#lx", lpc->lpc_size);
	if (lpc->lpc_intr != LPCCF_INTR_DEFAULT)
		aprint_normal(" intr %d", lpc->lpc_intr);

	return UNCONF;
}

/*
 * LPC bus ops
 */

static uint8_t
it8712_pnp_read(lpctag_t tag, int ldn, uint index)
{
	gemini_lpc_softc_t *sc = tag;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	if (ldn != GEMINI_LPC_LDN_ALL) {
		bus_space_write_1(iot, ioh, IT8712_ADDR, IT8712_LDN);
		bus_space_write_1(iot, ioh, IT8712_DATA, ldn);
	}
	bus_space_write_1(iot, ioh, IT8712_ADDR, index);
	return bus_space_read_1(iot, ioh, IT8712_DATA);
}

static void
it8712_pnp_write(lpctag_t tag, int ldn, uint index, uint8_t val)
{
	gemini_lpc_softc_t *sc = tag;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	if (ldn != GEMINI_LPC_LDN_ALL) {
		bus_space_write_1(iot, ioh, IT8712_ADDR, IT8712_LDN);
		bus_space_write_1(iot, ioh, IT8712_DATA, ldn);
	}
	bus_space_write_1(iot, ioh, IT8712_ADDR, index);
	bus_space_write_1(iot, ioh, IT8712_DATA, val);
}

static void
it8712_pnp_enter(lpctag_t tag)
{
	gemini_lpc_softc_t *sc = tag;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	bus_space_write_1(iot, ioh, IT8712_ADDR, 0x87);
	bus_space_write_1(iot, ioh, IT8712_ADDR, 0x01);
	bus_space_write_1(iot, ioh, IT8712_ADDR, 0x55);
	bus_space_write_1(iot, ioh, IT8712_ADDR, 0x55);
}

static void
it8712_pnp_exit(lpctag_t tag)
{
	gemini_lpc_softc_t *sc = tag;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	bus_space_write_1(iot, ioh, IT8712_ADDR, IT8712_CFGCTL);
	bus_space_write_1(iot, ioh, IT8712_DATA, CFGCTL_WAITKEY);
}

static void *
it8712_intr_establish(lpctag_t tag, uint intr, int ipl, int ist,
	int (*func)(void *), void *arg)
{
	gemini_lpc_softc_t *sc = tag;
	void *ih;

	ih = gemini_lpchc_intr_establish(sc->sc_lpchctag, intr, ipl, ist, func, arg);
	
	return ih;
}

static void
it8712_intr_disestablish(lpctag_t tag, void *ih)
{
	gemini_lpc_softc_t *sc = tag;

	gemini_lpchc_intr_disestablish(sc->sc_lpchctag, ih);
}
