#include "opt_gemini.h"
#if !defined(GEMINI_MASTER)  && !defined(GEMINI_SLAVE)
# error IPI needs GEMINI_MASTER or GEMINI_SLAVE
#endif
#include "locators.h"
#include "gpn.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: gemini_ipm.c,v 1.7 2023/05/26 20:50:21 andvar Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <arm/cpufunc.h>
#include <arm/arm32/pte.h>
#include <arch/arm/gemini/gemini_obiovar.h>
#include <arch/arm/gemini/gemini_ipivar.h>
#include <arch/arm/gemini/gemini_ipm.h>
#include <arch/arm/gemini/gemini_ipmvar.h>
#include <evbarm/gemini/gemini.h>

// #define IPMDEBUG
#if defined IPMDEBUG
static int ipmdebug;
# define DPRINTFN(n, x)	do { if ((n) >= ipmdebug) printf x ; } while (1)
#else
# define DPRINTFN(n, x)
#endif

typedef struct dispatch_entry {
	unsigned int ipl;
	size_t quota;
	void *arg; 
	void (*consume)(void *, const void *);
	void (*counter)(void *, size_t);
#ifdef NOTYET
	void *sih;		/* softint handle */
#endif
} ipm_dispatch_entry_t;

typedef struct gemini_ipm_softc {
	device_t              sc_dev;
	void		     *sc_ih;
	ipm_queue_t	     *sc_rxqueue;
	ipm_queue_t	     *sc_txqueue;
	size_t		      sc_txqavail;	/* quota available */
	unsigned long long    sc_rxcount;
	unsigned long long    sc_txcount;
	ipm_dispatch_entry_t  sc_dispatch_tab[256];
} gemini_ipm_softc_t;


static int  gemini_ipm_match(device_t, cfdata_t, void *);
static void gemini_ipm_attach(device_t, device_t, void *);
static int  gemini_ipm_intr(void *);
static void gemini_ipm_count_txdone(gemini_ipm_softc_t *);


CFATTACH_DECL_NEW(geminiipm, sizeof(struct gemini_ipm_softc),
	gemini_ipm_match, gemini_ipm_attach, NULL, NULL);

gemini_ipm_softc_t *gemini_ipm_sc = NULL;


/*
 * copy from shared queue to private copy
 * SW coherency would go here if desc_src were in cached mem
 */
static inline void
gemini_ipm_desc_read(ipm_desc_t *desc_dst, const ipm_desc_t *desc_src)
{
	extern void gpn_print_gd(const void *);	/* XXX DEBUG */
	DPRINTFN(2, ("%s: %p %p\n", __FUNCTION__, desc_dst, desc_src));
#ifdef IPMDEBUG
	if (ipmdebug >= 3)
		gpn_print_gd(desc_src);
#endif
	*desc_dst = *desc_src;
	KASSERT(desc_dst->tag != IPM_TAG_NONE);
}

/*
 * copy from private copy to shared queue
 * SW coherency would go here if desc_dst were in cached mem
 */
static inline void
gemini_ipm_desc_write(ipm_desc_t *desc_dst, const ipm_desc_t *desc_src)
{
	extern void gpn_print_gd(const void *);	/* XXX DEBUG */
	DPRINTFN(2, ("%s: %p %p\n", __FUNCTION__, desc_dst, desc_src));
#ifdef IPMDEBUG
	if (ipmdebug >= 3)
		gpn_print_gd(desc_src);
#endif
	KASSERT(desc_src->tag != IPM_TAG_NONE);
	*desc_dst = *desc_src;
}


static int
gemini_ipm_match(device_t parent, cfdata_t cf, void *aux)
{
        char *name = aux;

        if (strcmp(name, "geminiipm") != 0)
		return 0;

        return 1;
}

static void
gemini_ipm_attach(device_t parent, device_t self, void *aux)
{
        gemini_ipm_softc_t *sc = device_private(self);
	void *ih;

	sc->sc_dev = self;
	ih = ipi_intr_establish(gemini_ipm_intr, sc);
	if (ih == NULL)
		panic("%s: Cannot establish IPI interrupt\n",
			device_xname(self));
	sc->sc_ih = ih;
	memset(&sc->sc_dispatch_tab, 0, sizeof(sc->sc_dispatch_tab));


	/*
	 * queues are flipped tx/rx for master/slave
	 */
	KASSERT(GEMINI_IPMQ_SIZE == (2 * sizeof(ipm_queue_t)));
#if defined(GEMINI_MASTER)
	sc->sc_rxqueue = (ipm_queue_t *)GEMINI_IPMQ_VBASE;
	sc->sc_txqueue = sc->sc_rxqueue + 1;
	memset(sc->sc_rxqueue, 0, sizeof(ipm_queue_t));
	memset(sc->sc_txqueue, 0, sizeof(ipm_queue_t));
#elif defined(GEMINI_SLAVE)
	sc->sc_txqueue = (ipm_queue_t *)GEMINI_IPMQ_VBASE;
	sc->sc_rxqueue = sc->sc_txqueue + 1;
#else
# error one of GEMINI_MASTER or GEMINI_SLAVE must be defined
#endif
	sc->sc_txqavail = NIPMDESC;

	sc->sc_rxcount = 0LL;
	sc->sc_txcount = 0LL;

	gemini_ipm_sc = sc;

	aprint_normal("\n");
	aprint_naive("\n");

#if NGPN > 0
	config_found(self, __UNCONST("gpn"), NULL, CFARGS_NONE);
#endif
}

void *
gemini_ipm_register(uint8_t tag, unsigned int ipl, size_t quota,
        void (*consume)(void *, const void *),
        void (*counter)(void *, size_t),
	void *arg)
{
	gemini_ipm_softc_t *sc = gemini_ipm_sc;
	ipm_dispatch_entry_t *disp;
	void *ipmh = NULL;
	int psw;

	DPRINTFN(1, ("%s:%d: %d %d %ld %p %p %p\n", __FUNCTION__, __LINE__,
		tag, ipl, quota, consume, counter, arg));
	if (sc == NULL)
		return NULL;		/* not attached yet */

	if (tag == 0)
		return NULL;		/* tag #0 is reserved */

	psw = disable_interrupts(I32_bit);
	disp = &sc->sc_dispatch_tab[tag];
	if (disp->consume == 0) {
		if (sc->sc_txqavail >= quota) {
			sc->sc_txqavail -= quota;
			disp->ipl = ipl;
			disp->consume = consume;
			disp->counter = counter;
			disp->arg = arg;
#ifdef NOTYET
			if (ipl > SOFTINT_LVLMASK)
				panic("%s: bad level %d",
					device_xname(sc->sc_dev), ipl);
			disp->sih = softint_establish(ipl, consume, arg);
#endif
			ipmh = disp;
		}
	}
	restore_interrupts(psw);

	return ipmh;
}

void
gemini_ipm_deregister(void *ipmh)
{
	gemini_ipm_softc_t *sc = gemini_ipm_sc;
	ipm_dispatch_entry_t *disp = ipmh;
	int psw;

	if (sc == NULL)
		return;

	psw = disable_interrupts(I32_bit);
	memset(disp, 0, sizeof(*disp));
#ifdef NOTYET
	softint_disestablish(sc->sih);
#endif
	restore_interrupts(psw);
}

static inline int
gemini_ipm_dispatch(gemini_ipm_softc_t *sc)
{
	ipm_dispatch_entry_t *disp;
	ipm_desc_t desc;
	ipmqindex_t ix_read;
	ipmqindex_t ix_write;
	int rv = 0;

	ix_read = sc->sc_rxqueue->ix_read;
	ix_write = sc->sc_rxqueue->ix_write;

	if (! ipmqisempty(ix_read, ix_write)) {
		rv = 1;
		do {
			gemini_ipm_desc_read(&desc,
				&sc->sc_rxqueue->ipm_desc[ix_read]);
			ix_read = ipmqnext(ix_read);
			KASSERT(desc.tag != IPM_TAG_NONE);
			disp = &sc->sc_dispatch_tab[desc.tag];
#ifdef NOTYET
			softint_schedule(disp->sih);
#else
			(*disp->consume)(disp->arg, &desc);
#endif
			ix_write = sc->sc_rxqueue->ix_write;
			sc->sc_rxqueue->ix_read = ix_read;
			sc->sc_rxcount++;
		} while (! ipmqisempty(ix_read, ix_write));
	} else {
		DPRINTFN(1, ("%s: ipmqisempty %d %d\n",
			__FUNCTION__, ix_read, ix_write));
	}
	return rv;
}

static int
gemini_ipm_intr(void *arg)
{
	gemini_ipm_softc_t *sc = arg;
	int rv;

	rv = gemini_ipm_dispatch(sc);
	gemini_ipm_count_txdone(sc); 

	return rv;
}

int
gemini_ipm_produce(const void *adescp, size_t ndesc)
{
	const ipm_desc_t *descp = adescp;
	gemini_ipm_softc_t *sc = gemini_ipm_sc;
	ipmqindex_t ix_read;
	ipmqindex_t ix_write;

	KASSERT(ndesc == 1);			/* XXX TMP */

	DPRINTFN(2, ("%s:%d: %p %ld, tag %d\n",
		__FUNCTION__, __LINE__, descp, ndesc, descp->tag));
	ix_read = sc->sc_txqueue->ix_read;
	ix_write = sc->sc_txqueue->ix_write;
	if (ipmqisfull(ix_read, ix_write)) {
		/* we expect this to "never" happen; check your quotas */
		panic("%s: queue full\n", device_xname(sc->sc_dev));
	}
	gemini_ipm_desc_write(&sc->sc_txqueue->ipm_desc[ix_write], descp);
	sc->sc_txqueue->ix_write = ipmqnext(ix_write);
	sc->sc_txcount++; 

	ipi_send(); 

	gemini_ipm_count_txdone(sc); 

	return 0;
} 

static void *
gemini_ba_to_va(bus_addr_t ba)
{
	return (void *)(GEMINI_ALLMEM_VBASE + ba);
}

void
gemini_ipm_copyin(void *dst, bus_addr_t ba, size_t len)
{
	void *src;

	DPRINTFN(2, ("%s:%d: %p %#lx %ld\n",
		__FUNCTION__, __LINE__, dst, ba, len));
	src = gemini_ba_to_va(ba);
	memcpy(dst, src, len);
	cpu_dcache_inv_range((vaddr_t)src, len);
}


static void
gemini_ipm_count_txdone(gemini_ipm_softc_t *sc)
{
	ipmqindex_t count = 0;		/* XXX must count per tag */
	ipm_dispatch_entry_t *disp;
	ipmqindex_t ixr = sc->sc_txqueue->ix_read;
	uint8_t tag = IPM_TAG_GPN;
	static ipmqindex_t oixr = 0;

	while (oixr != ixr) {
		oixr = ipmqnext(oixr);
		count++;
	}
	if (count != 0) {
		disp = &sc->sc_dispatch_tab[tag];
		(*disp->counter)(disp->arg, count);
	}
}

void gemini_ipm_stats_print(void);
void
gemini_ipm_stats_print(void)
{
	gemini_ipm_softc_t *sc = gemini_ipm_sc;

	printf("rxcount %lld, txcount %lld\n", sc->sc_rxcount, sc->sc_txcount);
}
