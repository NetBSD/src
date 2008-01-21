/*	$NetBSD: rtas.c,v 1.1.6.5 2008/01/21 09:38:29 yamt Exp $ */

/*
 * CHRP RTAS support routines
 * Common Hardware Reference Platform / Run-Time Abstraction Services
 *
 * Started by Aymeric Vincent in 2007, public domain.
 * Modifications by Tim Rightnour 2007.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtas.c,v 1.1.6.5 2008/01/21 09:38:29 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <uvm/uvm.h>

#include <dev/clock_subr.h>
#include <dev/ofw/openfirm.h>
#include <machine/autoconf.h>
#include <machine/stdarg.h>
#include <powerpc/rtas.h>

int machine_has_rtas = 0;

struct rtas_softc *rtas0_softc;

struct rtas_softc {
	struct device ra_dev;
	int ra_phandle;
	int ra_version;

	void (*ra_entry_pa)(paddr_t, paddr_t);
	paddr_t ra_base_pa;

	struct todr_chip_handle ra_todr_handle;
};

static struct {
	int token;
	int exists;
} rtas_function_token[RTAS_FUNC_number];

static struct {
        const char *name;
        int index;
} rtas_function_lookup[] = {
        { "restart-rtas", RTAS_FUNC_RESTART_RTAS },
        { "nvram-fetch", RTAS_FUNC_NVRAM_FETCH },
        { "nvram-store", RTAS_FUNC_NVRAM_STORE },
        { "get-time-of-day", RTAS_FUNC_GET_TIME_OF_DAY },
        { "set-time-of-day", RTAS_FUNC_SET_TIME_OF_DAY },
        { "set-time-for-power-on", RTAS_FUNC_SET_TIME_FOR_POWER_ON },
        { "event-scan", RTAS_FUNC_EVENT_SCAN },
        { "check-exception", RTAS_FUNC_CHECK_EXCEPTION },
        /* Typo in my Efika's firmware */
        { "check-execption", RTAS_FUNC_CHECK_EXCEPTION },
        { "read-pci-config", RTAS_FUNC_READ_PCI_CONFIG },
        { "write-pci-config", RTAS_FUNC_WRITE_PCI_CONFIG },
        { "display-character", RTAS_FUNC_DISPLAY_CHARACTER },
        { "set-indicator", RTAS_FUNC_SET_INDICATOR },
        { "power-off", RTAS_FUNC_POWER_OFF },
        { "suspend", RTAS_FUNC_SUSPEND },
        { "hibernate", RTAS_FUNC_HIBERNATE },
        { "system-reboot", RTAS_FUNC_SYSTEM_REBOOT },
};

static int rtas_match(struct device *, struct cfdata *, void *);
static void rtas_attach(struct device *, struct device *, void *);
static int rtas_detach(struct device *, int);
static int rtas_activate(struct device *, enum devact);
static int rtas_todr_gettime_ymdhms(struct todr_chip_handle *,
    struct clock_ymdhms *);
static int rtas_todr_settime_ymdhms(struct todr_chip_handle *,
    struct clock_ymdhms *);

CFATTACH_DECL(rtas, sizeof (struct rtas_softc),
    rtas_match, rtas_attach, rtas_detach, rtas_activate);

static int
rtas_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "rtas"))
		return 0;

	return 1;
}

static void
rtas_attach(struct device *parent, struct device *self, void *aux)
{
	struct confargs *ca = aux;
	struct rtas_softc *sc = (struct rtas_softc *) self;
	int ph = ca->ca_node;
	int ih;
	int rtas_size;
	int rtas_entry;
	struct pglist pglist;
	char buf[4];
	int i;

	machine_has_rtas = 1;
	
	sc->ra_phandle = ph;
	if (OF_getprop(ph, "rtas-version", buf, sizeof buf) != sizeof buf)
		goto fail;
	sc->ra_version = of_decode_int(buf);
	if (OF_getprop(ph, "rtas-size", buf, sizeof buf) != sizeof buf)
		goto fail;
	rtas_size = of_decode_int(buf);

	/*
	 * Instantiate the RTAS
	 */
	if (uvm_pglistalloc(rtas_size, 0, ~0, 4096, 256 << 20, &pglist, 1, 0))
		goto fail;

	sc->ra_base_pa = TAILQ_FIRST(&pglist)->phys_addr;

	ih = OF_open("/rtas");
	if (ih == -1)
		goto fail_and_free;

	rtas_entry =
		OF_call_method_1("instantiate-rtas", ih, 1, sc->ra_base_pa);

	if (rtas_entry == -1)
		goto fail_and_free;

	sc->ra_entry_pa = (void *) rtas_entry;

	/*
	 * Get the tokens of the methods the RTAS provides
	 */

	for (i = 0;
	    i < sizeof rtas_function_lookup / sizeof rtas_function_lookup[0];
	    i++) {
		int index = rtas_function_lookup[i].index;

		if (OF_getprop(ph, rtas_function_lookup[i].name, buf,
				sizeof buf) != sizeof buf)
			continue;

		rtas_function_token[index].token = of_decode_int(buf);
		rtas_function_token[index].exists = 1;
	}

	rtas0_softc = sc;

	printf(": version %d, entry @pa 0x%x\n", sc->ra_version,
		(unsigned) rtas_entry);

	/*
	 * Initialise TODR support
	 */
	sc->ra_todr_handle.cookie = sc;
	sc->ra_todr_handle.bus_cookie = NULL;
	sc->ra_todr_handle.todr_gettime = NULL;
	sc->ra_todr_handle.todr_settime = NULL;
	sc->ra_todr_handle.todr_gettime_ymdhms = rtas_todr_gettime_ymdhms;
	sc->ra_todr_handle.todr_settime_ymdhms = rtas_todr_settime_ymdhms;
	sc->ra_todr_handle.todr_setwen = NULL;
	todr_attach(&sc->ra_todr_handle);

	return;

fail_and_free:
	uvm_pglistfree(&pglist);
fail:
	aprint_error(": attach failed!\n");
}

static int
rtas_detach(struct device *self, int flags)
{
	return EOPNOTSUPP;
}

static int
rtas_activate(struct device *self, enum devact act)
{
	return EOPNOTSUPP;
}

/*
 * Support for calling to the RTAS
 */

int
rtas_call(int token, int nargs, int nreturns, ...)
{
	va_list ap;
	static struct {
		int token;
		int nargs;
		int nreturns;
		int args_n_results[RTAS_MAXARGS];
	} args;
	paddr_t pargs = (paddr_t)&args;
	paddr_t base;
	register_t msr;
	void (*entry)(paddr_t, paddr_t);
	int n;

	if (rtas0_softc == NULL)
		return -1;

	if (nargs + nreturns > RTAS_MAXARGS)
		return -1;

	if (!rtas_function_token[token].exists)
		return -1;
	
	base = rtas0_softc->ra_base_pa;
	entry = rtas0_softc->ra_entry_pa;

	memset(args.args_n_results, 0, RTAS_MAXARGS * sizeof(int));
	args.nargs = nargs;
	args.nreturns = nreturns;
	args.token = rtas_function_token[token].token;
	
	va_start(ap, nreturns);
	for (n=0; n < nargs && n < RTAS_MAXARGS; n++)
		args.args_n_results[n] = va_arg(ap, int);

	__insn_barrier();
	msr = mfmsr();
	mtmsr(msr & ~(PSL_EE | PSL_FP | PSL_ME | PSL_FE0 | PSL_SE | PSL_BE |
		PSL_FE1 | PSL_IR | PSL_DR | PSL_RI));
	__asm("isync;\n");

	entry(pargs, base);

	mtmsr(msr);
	__asm("isync;\n");

	for (n = nargs; n < nargs + nreturns && n < RTAS_MAXARGS; n++)
		*va_arg(ap, int *) = args.args_n_results[n];

	va_end(ap);

	return args.args_n_results[nargs];
}

int
rtas_has_func(int token)
{
	return rtas_function_token[token].exists;
}

/*
 * Real-Time Clock support
 */

static int
rtas_todr_gettime_ymdhms(struct todr_chip_handle *h, struct clock_ymdhms *t)
{
	int status, year, month, day, hour, minute, second, nanosecond;

	if (!rtas_function_token[RTAS_FUNC_GET_TIME_OF_DAY].exists)
		return ENXIO;

	if (rtas_call(RTAS_FUNC_GET_TIME_OF_DAY, 0, 8, &status, &year,
		&month, &day, &hour, &minute, &second, &nanosecond) < 0)
		return ENXIO;
	
	t->dt_year = year;
	t->dt_mon = month;
	t->dt_day = day;
	t->dt_hour = hour;
	t->dt_min = minute;
	t->dt_sec = second;

	return 0;
}

static int
rtas_todr_settime_ymdhms(struct todr_chip_handle *h, struct clock_ymdhms *t)
{
	int status, year, month, day, hour, minute, second, nanosecond;

	if (!rtas_function_token[RTAS_FUNC_SET_TIME_OF_DAY].exists)
		return ENXIO;

	year = t->dt_year;
	month = t->dt_mon;
	day = t->dt_day;
	hour = t->dt_hour;
	minute = t->dt_min;
	second = t->dt_sec;
	nanosecond = 0;

	if (rtas_call(RTAS_FUNC_SET_TIME_OF_DAY, 7, 1, year, month,
		day, hour, minute, second, nanosecond, &status) < 0)
		return ENXIO;
	
	return 0;
}
