/*	$NetBSD: rtas.c,v 1.4 2007/12/27 17:23:54 garbled Exp $ */

/*
 * CHRP RTAS support routines
 * Common Hardware Reference Platform / Run-Time Abstraction Services
 *
 * Started by Aymeric Vincent in 2007, public domain.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtas.c,v 1.4 2007/12/27 17:23:54 garbled Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <uvm/uvm.h>

#include <dev/clock_subr.h>
#include <dev/ofw/openfirm.h>
#include <machine/autoconf.h>

struct rtas_softc *rtas0_softc;

struct rtas_softc {
	struct device ra_dev;
	int ra_phandle;
	int ra_version;

	void (*ra_entry_pa)(paddr_t, paddr_t);
	paddr_t ra_base_pa;

	struct todr_chip_handle ra_todr_handle;
};

enum rtas_func {
	RTAS_FUNC_RESTART_RTAS, RTAS_FUNC_NVRAM_FETCH, RTAS_FUNC_NVRAM_STORE,
	RTAS_FUNC_GET_TIME_OF_DAY, RTAS_FUNC_SET_TIME_OF_DAY,
	RTAS_FUNC_SET_TIME_FOR_POWER_ON,
	RTAS_FUNC_EVENT_SCAN, RTAS_FUNC_CHECK_EXCEPTION,
	RTAS_FUNC_READ_PCI_CONFIG, RTAS_FUNC_WRITE_PCI_CONFIG,
	RTAS_FUNC_DISPLAY_CHARACTER, RTAS_FUNC_SET_INDICATOR,
	RTAS_FUNC_POWER_OFF, RTAS_FUNC_SUSPEND, RTAS_FUNC_HIBERNATE,
	RTAS_FUNC_SYSTEM_REBOOT,
	RTAS_FUNC_number
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

struct rtas_call {
	int token;
	int nargs;
	int nreturns;
	int args[0];
};
void rtas_reboot(void);
static int	rtas_match(struct device *, struct cfdata *, void *);
static void	rtas_attach(struct device *, struct device *, void *);
static int	rtas_detach(struct device *, int);
static int	rtas_activate(struct device *, enum devact);
static int	rtas_call(struct rtas_call *);
static int	rtas_todr_gettime_ymdhms(struct todr_chip_handle *,
							struct clock_ymdhms *);
static int	rtas_todr_settime_ymdhms(struct todr_chip_handle *,
							struct clock_ymdhms *);

CFATTACH_DECL(rtas, sizeof (struct rtas_softc),
	rtas_match, rtas_attach, rtas_detach, rtas_activate);

static int
rtas_match(struct device *parent, struct cfdata *match, void *aux) {
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "rtas"))
		return 0;

	return 1;
}

static void
rtas_attach(struct device *parent, struct device *self, void *aux) {
	struct confargs *ca = aux;
	struct rtas_softc *sc = (struct rtas_softc *) self;
	int ph = ca->ca_node;
	int ih;
	int rtas_size;
	int rtas_entry;
	struct pglist pglist;
	char buf[4];
	int i;

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
rtas_detach(struct device *self, int flags) {
	return EOPNOTSUPP;
}

static int
rtas_activate(struct device *self, enum devact act) {
	return EOPNOTSUPP;
}

/*
 * Support for calling to the RTAS
 */

static int
rtas_call(struct rtas_call *args) {
	paddr_t pargs = (paddr_t) args;
	paddr_t base;
	void (*entry)(paddr_t, paddr_t);
	register_t msr;

	if (rtas0_softc == NULL)
		return -1;

	base = rtas0_softc->ra_base_pa;
	entry = rtas0_softc->ra_entry_pa;

	__insn_barrier();
	msr = mfmsr();
	mtmsr(msr & ~(PSL_EE | PSL_FP | PSL_ME | PSL_FE0 | PSL_SE | PSL_BE |
		PSL_FE1 | PSL_IR | PSL_DR | PSL_RI));
	__asm("isync;\n");

	entry(pargs, base);

	mtmsr(msr);
	__asm("isync;\n");

	return args->args[args->nargs];
}

void
rtas_reboot(void)
{
	static struct {
		struct rtas_call rc;
		int status;
	} args;

	if (!rtas_function_token[RTAS_FUNC_SYSTEM_REBOOT].exists)
		return;

	args.rc.token = rtas_function_token[RTAS_FUNC_SYSTEM_REBOOT].token;
	args.rc.nargs = 0;
	args.rc.nreturns = 1;
	rtas_call(&args.rc);
}

/*
 * Real-Time Clock support
 */

static int
rtas_todr_gettime_ymdhms(struct todr_chip_handle *h, struct clock_ymdhms *t) {
	static struct {
		struct rtas_call rc;
		int status;
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
		int nanosecond;
	} args;

	if (!rtas_function_token[RTAS_FUNC_GET_TIME_OF_DAY].exists)
		return ENXIO;

	args.rc.token = rtas_function_token[RTAS_FUNC_GET_TIME_OF_DAY].token;
	args.rc.nargs = 0;
	args.rc.nreturns = 8;
	if (rtas_call(&args.rc) < 0)
		return ENXIO;

	t->dt_year = args.year;
	t->dt_mon = args.month;
	t->dt_day = args.day;
	t->dt_hour = args.hour;
	t->dt_min = args.minute;
	t->dt_sec = args.second;

	return 0;
}

static int
rtas_todr_settime_ymdhms(struct todr_chip_handle *h, struct clock_ymdhms *t) {
	static struct {
		struct rtas_call rc;
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
		int nanosecond;
		int status;
	} args;

	if (!rtas_function_token[RTAS_FUNC_SET_TIME_OF_DAY].exists)
		return ENXIO;

	args.rc.token = rtas_function_token[RTAS_FUNC_SET_TIME_OF_DAY].token;
	args.rc.nargs = 7;
	args.rc.nreturns = 1;
	args.year = t->dt_year;
	args.month = t->dt_mon;
	args.day = t->dt_day;
	args.hour = t->dt_hour;
	args.minute = t->dt_min;
	args.second = t->dt_sec;
	args.nanosecond = 0;

	if (rtas_call(&args.rc) < 0)
		return ENXIO;

	return 0;
}
