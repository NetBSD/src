/*	$NetBSD: atk0110.c,v 1.3 2010/02/09 03:32:57 cnst Exp $	*/
/*	$OpenBSD: atk0110.c,v 1.1 2009/07/23 01:38:16 cnst Exp $	*/

/*
 * Copyright (c) 2009 Constantine A. Murenin <cnst+netbsd@bugmail.mojo.ru>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: atk0110.c,v 1.3 2010/02/09 03:32:57 cnst Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <dev/sysmon/sysmonvar.h>

#include "acpi.h"
#include "acpivar.h"

/*
 * ASUSTeK AI Booster (ACPI ASOC ATK0110).
 *
 * This code was originally written for OpenBSD after the techniques
 * described in the Linux's asus_atk0110.c and FreeBSD's acpi_aiboost.c
 * were verified to be accurate on the actual hardware kindly provided by
 * Sam Fourman Jr.  It was subsequently ported from OpenBSD to DragonFly BSD,
 * and then to the NetBSD's sysmon_envsys(9) framework.
 *
 *				  -- Constantine A. Murenin <http://cnst.su/>
 */

#define AIBS_MORE_SENSORS
#define AIBS_MONLIMITS

struct aibs_sensor {
	envsys_data_t	s;
	ACPI_INTEGER	i;
	ACPI_INTEGER	l;
	ACPI_INTEGER	h;
};

struct aibs_softc {
	struct acpi_devnode	*sc_node;

	struct aibs_sensor	*sc_asens_volt;
	struct aibs_sensor	*sc_asens_temp;
	struct aibs_sensor	*sc_asens_fan;

	struct sysmon_envsys	*sc_sme;
};

static int aibs_match(device_t, cfdata_t, void *);
static void aibs_attach(device_t, device_t, void *);
static int aibs_detach(device_t, int);
static void aibs_refresh(struct sysmon_envsys *, envsys_data_t *);
#ifdef AIBS_MONLIMITS
static void aibs_get_limits(struct sysmon_envsys *, envsys_data_t *,
    sysmon_envsys_lim_t *);
#endif

static void aibs_attach_sif(device_t, enum envsys_units);

CFATTACH_DECL_NEW(aibs, sizeof(struct aibs_softc),
    aibs_match, aibs_attach, aibs_detach, NULL);

static const char* const aibs_hid[] = {
	"ATK0110",
	NULL
};

static int
aibs_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if(aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	/* successful match supersedes aiboost(4) */
	return acpi_match_hid(aa->aa_node->ad_devinfo, aibs_hid) * 2;
}

static void
aibs_attach(device_t parent, device_t self, void *aux)
{
	struct aibs_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	int err;

	sc->sc_node = aa->aa_node;

	aprint_naive(": ASUSTeK AI Booster\n");
	aprint_normal(": ASUSTeK AI Booster\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = aibs_refresh;
#ifdef AIBS_MONLIMITS
	sc->sc_sme->sme_get_limits = aibs_get_limits;
#endif

	aibs_attach_sif(self, ENVSYS_SVOLTS_DC);
	aibs_attach_sif(self, ENVSYS_STEMP);
	aibs_attach_sif(self, ENVSYS_SFANRPM);

	if (sc->sc_sme->sme_nsensors == 0) {
		aprint_error_dev(self, "no sensors found\n");
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	if ((err = sysmon_envsys_register(sc->sc_sme))) {
		aprint_error_dev(self, "unable to register with sysmon: %d\n",
		    err);
		if (sc->sc_asens_volt != NULL)
			free(sc->sc_asens_volt, M_DEVBUF);
		if (sc->sc_asens_temp != NULL)
			free(sc->sc_asens_temp, M_DEVBUF);
		if (sc->sc_asens_fan != NULL)
			free(sc->sc_asens_fan, M_DEVBUF);
		return;
	}
}

static void
aibs_attach_sif(device_t self, enum envsys_units st)
{
	struct aibs_softc	*sc = device_private(self);
	ACPI_STATUS		s;
	ACPI_BUFFER		b;
	ACPI_OBJECT		*bp, *o;
	int			i, n;
	char			name[] = "?SIF";
	struct aibs_sensor	*as;

	switch (st) {
	case ENVSYS_STEMP:
		name[0] = 'T';
		break;
	case ENVSYS_SFANRPM:
		name[0] = 'F';
		break;
	case ENVSYS_SVOLTS_DC:
		name[0] = 'V';
		break;
	default:
		return;
	}

	b.Length = ACPI_ALLOCATE_BUFFER;
	s = AcpiEvaluateObjectTyped(sc->sc_node->ad_handle, name, NULL, &b,
	    ACPI_TYPE_PACKAGE);
	if (ACPI_FAILURE(s)) {
		aprint_error_dev(self, "%s not found\n", name);
		return;
	}

	bp = b.Pointer;
	o = bp->Package.Elements;
	if (o[0].Type != ACPI_TYPE_INTEGER) {
		aprint_error_dev(self, "%s[0]: invalid type\n", name);
		AcpiOsFree(b.Pointer);
		return;
	}

	n = o[0].Integer.Value;
	if (bp->Package.Count - 1 < n) {
		aprint_error_dev(self, "%s: invalid package\n", name);
		AcpiOsFree(b.Pointer);
		return;
	} else if (bp->Package.Count - 1 > n) {
		int on = n;

#ifdef AIBS_MORE_SENSORS
		n = bp->Package.Count - 1;
#endif
		aprint_error_dev(self, "%s: malformed package: %i/%i"
		    ", assume %i\n", name, on, bp->Package.Count - 1, n);
	}
	if (n < 1) {
		aprint_error_dev(self, "%s: no members in the package\n",
		    name);
		AcpiOsFree(b.Pointer);
		return;
	}

	as = malloc(sizeof(*as) * n, M_DEVBUF, M_NOWAIT | M_ZERO);
	if (as == NULL) {
		aprint_error_dev(self, "%s: malloc fail\n", name);
		AcpiOsFree(b.Pointer);
		return;
	}
	switch (st) {
	case ENVSYS_STEMP:
		sc->sc_asens_temp = as;
		break;
	case ENVSYS_SFANRPM:
		sc->sc_asens_fan = as;
		break;
	case ENVSYS_SVOLTS_DC:
		sc->sc_asens_volt = as;
		break;
	default:
		/* NOTREACHED */
		return;
	}

	for (i = 0, o++; i < n; i++, o++) {
		ACPI_OBJECT	*oi;

		/* acpica5 automatically evaluates the referenced package */
		if(o[0].Type != ACPI_TYPE_PACKAGE) {
			aprint_error_dev(self,
			    "%s: %i: not a package: %i type\n",
			    name, i, o[0].Type);
			continue;
		}
		oi = o[0].Package.Elements;
		if (o[0].Package.Count != 5 ||
		    oi[0].Type != ACPI_TYPE_INTEGER ||
		    oi[1].Type != ACPI_TYPE_STRING ||
		    oi[2].Type != ACPI_TYPE_INTEGER ||
		    oi[3].Type != ACPI_TYPE_INTEGER ||
		    oi[4].Type != ACPI_TYPE_INTEGER) {
			aprint_error_dev(self,
			    "%s: %i: invalid package\n",
			    name, i);
			continue;
		}
		as[i].i = oi[0].Integer.Value;
		strlcpy(as[i].s.desc, oi[1].String.Pointer,
		    sizeof(as[i].s.desc));
		as[i].l = oi[2].Integer.Value;
		as[i].h = oi[3].Integer.Value;
		as[i].s.units = st;
#ifdef AIBS_MONLIMITS
		as[i].s.flags |= ENVSYS_FMONLIMITS;
		as[i].s.monitor = true;
#endif
		aprint_verbose_dev(self, "%c%i: "
		    "0x%08"PRIx64" %20s %5"PRIi64" / %5"PRIi64"  "
		    "0x%"PRIx64"\n",
		    name[0], i,
		    as[i].i, as[i].s.desc, (int64_t)as[i].l, (int64_t)as[i].h,
		    oi[4].Integer.Value);
		if (sysmon_envsys_sensor_attach(sc->sc_sme, &as[i].s))
			aprint_error_dev(self, "%c%i: unable to attach\n",
			    name[0], i);
	}

	AcpiOsFree(b.Pointer);
	return;
}

static int
aibs_detach(device_t self, int flags)
{
	struct aibs_softc	*sc = device_private(self);

	sysmon_envsys_unregister(sc->sc_sme);
	if (sc->sc_asens_volt != NULL)
		free(sc->sc_asens_volt, M_DEVBUF);
	if (sc->sc_asens_temp != NULL)
		free(sc->sc_asens_temp, M_DEVBUF);
	if (sc->sc_asens_fan != NULL)
		free(sc->sc_asens_fan, M_DEVBUF);
	return 0;
}

static void
aibs_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct aibs_softc	*sc = sme->sme_cookie;
	device_t		self = sc->sc_node->ad_device;
	envsys_data_t		*s = edata;
	enum envsys_units	st = s->units;
	ACPI_STATUS		rs;
	ACPI_OBJECT		p, *bp;
	ACPI_OBJECT_LIST	mp;
	ACPI_BUFFER		b;
	int			i;
	const char		*name;
	struct aibs_sensor	*as;
	ACPI_INTEGER		v;

	switch (st) {
	case ENVSYS_STEMP:
		name = "RTMP";
		as = sc->sc_asens_temp;
		break;
	case ENVSYS_SFANRPM:
		name = "RFAN";
		as = sc->sc_asens_fan;
		break;
	case ENVSYS_SVOLTS_DC:
		name = "RVLT";
		as = sc->sc_asens_volt;
		break;
	default:
		return;
	}
	if (as == NULL)
		return;
	for (i = 0; as[i].s.sensor != s->sensor; i++)
		;
	p.Type = ACPI_TYPE_INTEGER;
	p.Integer.Value = as[i].i;
	mp.Count = 1;
	mp.Pointer = &p;
	b.Length = ACPI_ALLOCATE_BUFFER;
	rs = AcpiEvaluateObjectTyped(sc->sc_node->ad_handle, name, &mp, &b,
	    ACPI_TYPE_INTEGER);
	if (ACPI_FAILURE(rs)) {
		aprint_debug_dev(self,
		    "%s: %i: evaluation failed\n",
		    name, i);
		s->state = ENVSYS_SINVALID;
		s->flags |= ENVSYS_FMONNOTSUPP;
		return;
	}
	bp = b.Pointer;
	v = bp->Integer.Value;
	AcpiOsFree(b.Pointer);

	switch (st) {
	case ENVSYS_STEMP:
		s->value_cur = v * 100 * 1000 + 273150000;
		if (v == 0) {
			s->state = ENVSYS_SINVALID;
			s->flags |= ENVSYS_FMONNOTSUPP;
			return;
		}
		break;
	case ENVSYS_SFANRPM:
		s->value_cur = v;
		break;
	case ENVSYS_SVOLTS_DC:
		s->value_cur = v * 1000;
		break;
	default:
		/* NOTREACHED */
		break;
	}
	if (s->state == 0 || s->state == ENVSYS_SINVALID)
		s->state = ENVSYS_SVALID;
	s->flags &= ~ENVSYS_FMONNOTSUPP;
}

#ifdef AIBS_MONLIMITS
static void
aibs_get_limits(struct sysmon_envsys *sme, envsys_data_t *edata,
    sysmon_envsys_lim_t *limits)
{
	struct aibs_softc	*sc = sme->sme_cookie;
	envsys_data_t		*s = edata;
	sysmon_envsys_lim_t	*li = limits;
	enum envsys_units	st = s->units;
	int			i;
	struct aibs_sensor	*as;
	ACPI_INTEGER		l, h;

	switch (st) {
	case ENVSYS_STEMP:
		as = sc->sc_asens_temp;
		break;
	case ENVSYS_SFANRPM:
		as = sc->sc_asens_fan;
		break;
	case ENVSYS_SVOLTS_DC:
		as = sc->sc_asens_volt;
		break;
	default:
		return;
	}
	if (as == NULL)
		return;
	for (i = 0; as[i].s.sensor != s->sensor; i++)
		;
	l = as[i].l;
	h = as[i].h;

	switch (st) {
	case ENVSYS_STEMP:
		li->sel_critmax = h * 100 * 1000 + 273150000;
		li->sel_warnmax = l * 100 * 1000 + 273150000;
		li->sel_flags = PROP_CRITMAX | PROP_WARNMAX;
		break;
	case ENVSYS_SFANRPM:
		/* some boards have strange limits for fans */
		if (l == 0) {
			li->sel_warnmin = h;
			li->sel_flags = PROP_WARNMIN;
		} else {
			li->sel_warnmin = l;
			li->sel_warnmax = h;
			li->sel_flags = PROP_WARNMIN | PROP_WARNMAX;
		}
		break;
	case ENVSYS_SVOLTS_DC:
		li->sel_critmin = l * 1000;
		li->sel_critmax = h * 1000;
		li->sel_flags = PROP_CRITMIN | PROP_CRITMAX;
		break;
	default:
		/* NOTREACHED */
		break;
	}
}
#endif /* AIBS_MONLIMITS */
