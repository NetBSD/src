/* $NetBSD: aiboost.c,v 1.4 2007/03/26 07:27:36 xtraeme Exp $ */

/*-
 * Copyright (c) 2007 Juan Romero Pardines
 * Copyright (c) 2006 Takanori Watanabe
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aiboost.c,v 1.4 2007/03/26 07:27:36 xtraeme Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mutex.h>

#include <machine/bus.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/sysmon/sysmonvar.h>

#ifdef AIBOOST_DEBUG
#define DPRINTF(x)		do { printf x; } while (/* CONSTCOND */ 0)
#else
#define DPRINTF(x)
#endif

#define AIBOOST_MAX_SENSORS	15

struct aiboost_elem {
	ACPI_HANDLE h;
	UINT32 id;
	char desc[256];
};

struct aiboost_comp {
	unsigned int num;
	struct aiboost_elem elem[1];
};

struct aiboost_softc {
	struct device sc_dev;
	struct acpi_devnode *sc_node;	/* ACPI devnode */
	struct aiboost_comp *sc_aitemp, *sc_aivolt, *sc_aifan;
	struct sysmon_envsys sc_sme;
	struct envsys_tre_data sc_data[AIBOOST_MAX_SENSORS];
	struct envsys_basic_info sc_info[AIBOOST_MAX_SENSORS];
	kmutex_t sc_mtx;
};

static ACPI_STATUS aiboost_getcomp(ACPI_HANDLE *,
				   const char *,
				   struct aiboost_comp **);
static int	aiboost_get_value(ACPI_HANDLE, const char *, UINT32);

/* envsys(9) glue */
static void	aiboost_setup_sensors(struct aiboost_softc *);
static int	aiboost_gtredata(struct sysmon_envsys *, envsys_tre_data_t *);
static int	aiboost_streinfo(struct sysmon_envsys *, envsys_basic_info_t *);
static void	aiboost_refresh_sensors(struct aiboost_softc *);

/* autoconf(9) glue */
static int	aiboost_acpi_match(struct device *, struct cfdata *, void *);
static void	aiboost_acpi_attach(struct device *, struct device *, void *);

CFATTACH_DECL(aiboost, sizeof(struct aiboost_softc), aiboost_acpi_match,
    aiboost_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const aiboost_acpi_ids[] = {
	"ATK0110",
	NULL
};

static int
aiboost_acpi_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, aiboost_acpi_ids);
}

static void
aiboost_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct aiboost_softc *sc = (struct aiboost_softc *)self;
	struct acpi_attach_args *aa = aux;
	ACPI_HANDLE *handl;
	int i, maxsens;

	sc->sc_node = aa->aa_node;
	handl = sc->sc_node->ad_handle;

	aprint_naive("\n");
	aprint_normal("\n");

	aprint_normal("%s: ASUS AI Boost Hardware monitor\n",
	    sc->sc_dev.dv_xname);

	/* Get Handle components for the temp package */
	if (ACPI_FAILURE(aiboost_getcomp(handl, "TSIF", &sc->sc_aitemp)))
		return;

	/* Get Handle components for the voltage package  */
	if (ACPI_FAILURE(aiboost_getcomp(handl, "VSIF", &sc->sc_aivolt)))
		return;

	/* Get Handle components for the fan package */
	if (ACPI_FAILURE(aiboost_getcomp(handl, "FSIF", &sc->sc_aifan)))
		return;

	/* Initialize sensors */
	maxsens = sc->sc_aivolt->num + sc->sc_aitemp->num + sc->sc_aifan->num;
	DPRINTF(("%s: maxsens=%d\n", __func__, maxsens));

	for (i = 0; i < maxsens; i++) {
		sc->sc_data[i].sensor = sc->sc_info[i].sensor = i;
		sc->sc_data[i].validflags = (ENVSYS_FVALID|ENVSYS_FCURVALID);
		sc->sc_info[i].validflags = ENVSYS_FVALID;
		sc->sc_data[i].warnflags = ENVSYS_WARN_OK;
	}

	aiboost_setup_sensors(sc);

	mutex_init(&sc->sc_mtx, MUTEX_DRIVER, IPL_NONE);

	/*
	 * Hook into the system monitor.
	 */
	sc->sc_sme.sme_ranges = NULL;
	sc->sc_sme.sme_sensor_info = sc->sc_info;
	sc->sc_sme.sme_sensor_data = sc->sc_data;
	sc->sc_sme.sme_cookie = sc;
	sc->sc_sme.sme_gtredata = aiboost_gtredata;
	sc->sc_sme.sme_streinfo = aiboost_streinfo;
	sc->sc_sme.sme_nsensors = maxsens;
	sc->sc_sme.sme_envsys_version = 1000;
	sc->sc_sme.sme_flags = 0;

	if (sysmon_envsys_register(&sc->sc_sme))
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
}

#define COPYDESCR(x, y)				\
	do {					\
		strlcpy((x), (y), sizeof(x));	\
	} while (/* CONSTCOND */ 0)

static void
aiboost_setup_sensors(struct aiboost_softc *sc)
{
	int i, j;

	/* Temperatures */
	for (i = 0; i < sc->sc_aitemp->num; i++) {
		sc->sc_data[i].units = ENVSYS_STEMP;
		sc->sc_info[i].units = ENVSYS_STEMP;
		COPYDESCR(sc->sc_info[i].desc, sc->sc_aitemp->elem[i].desc);
		DPRINTF(("%s: info[%d].desc=%s elem[%d].desc=%s\n", __func__,
		    i, sc->sc_info[i].desc, i, sc->sc_aitemp->elem[i].desc));
	}

	/* skip temperatures */
	j = sc->sc_aitemp->num;

	/* Voltages */
	for (i = 0; i < sc->sc_aivolt->num; i++, j++) {
		sc->sc_data[j].units = ENVSYS_SVOLTS_DC;
		sc->sc_info[j].units = ENVSYS_SVOLTS_DC;
		COPYDESCR(sc->sc_info[j].desc, sc->sc_aivolt->elem[i].desc);
		DPRINTF(("%s: info[%d].desc=%s elem[%d].desc=%s\n", __func__,
		    j, sc->sc_info[j].desc, i, sc->sc_aivolt->elem[i].desc));
	}

	/* skip voltages */
	j = sc->sc_aitemp->num + sc->sc_aivolt->num;

	/* Fans */
	for (i = 0; i < sc->sc_aifan->num; i++, j++) {
		sc->sc_data[j].units = ENVSYS_SFANRPM;
		sc->sc_info[j].units = ENVSYS_SFANRPM;
		COPYDESCR(sc->sc_info[j].desc, sc->sc_aifan->elem[i].desc);
		DPRINTF(("%s: info[%d].desc=%s elem[%d].desc=%s\n", __func__,
		    j, sc->sc_info[j].desc, i, sc->sc_aifan->elem[i].desc));
		    
	}
}

static void
aiboost_refresh_sensors(struct aiboost_softc *sc)
{
	ACPI_HANDLE *h = sc->sc_node->ad_handle;
	int i, j, val;

	mutex_enter(&sc->sc_mtx);
	/* Temperatures */
	for (i = 0; i < sc->sc_aitemp->num; i++) {
		val = aiboost_get_value(h, "RTMP", sc->sc_aitemp->elem[i].id);
		/* envsys(9) wants mK... convert from Celsius. */
		sc->sc_data[i].cur.data_us = val * 100000 + 273150000;
		DPRINTF(("%s: temp[%d] data_us=%d val=%d\n", __func__,
		    i, sc->sc_data[i].cur.data_us, val));
	}

	j = sc->sc_aitemp->num;

	/* Voltages */
	for (i = 0; i < sc->sc_aivolt->num; i++, j++) {
		val = aiboost_get_value(h, "RVLT", sc->sc_aivolt->elem[i].id);
		sc->sc_data[j].cur.data_us = val * 10000;
		sc->sc_data[j].cur.data_us /= 10;
		sc->sc_info[j].rfact = 10000;
		DPRINTF(("%s: volt[%d] data_us=%d val=%d j=%d\n", __func__,
		    i, sc->sc_data[j].cur.data_us, val, j));
	}

	j = sc->sc_aitemp->num + sc->sc_aivolt->num;

	/* Fan */
	for (i = 0; i < sc->sc_aifan->num; i++, j++) {
		val = aiboost_get_value(h, "RFAN", sc->sc_aifan->elem[i].id);
		sc->sc_data[j].cur.data_us = val ;
		DPRINTF(("%s: fan[%d] val=%d j=%d\n", __func__, i, val, j));
	}
	mutex_exit(&sc->sc_mtx);
}

static int
aiboost_gtredata(struct sysmon_envsys *sme, envsys_tre_data_t *tred)
{
	struct aiboost_softc *sc = sme->sme_cookie;

	/* Refresh our stored data for every sensor */
	aiboost_refresh_sensors(sc);
	*tred = sc->sc_data[tred->sensor];

	return 0;
}

static int
aiboost_streinfo(struct sysmon_envsys *sme, envsys_basic_info_t *binfo)
{
	/* Not implemented */
	binfo->validflags = 0;

	return 0;
}

static int
aiboost_get_value(ACPI_HANDLE handle, const char *path, UINT32 number)
{
	ACPI_OBJECT arg1, *ret;
	ACPI_OBJECT_LIST args;
	ACPI_BUFFER buf;
	int val;	
	buf.Length = ACPI_ALLOCATE_BUFFER;
	buf.Pointer = 0;

	arg1.Type = ACPI_TYPE_INTEGER;
	arg1.Integer.Value = number;
	args.Count = 1;
	args.Pointer = &arg1;

	if (ACPI_FAILURE(AcpiEvaluateObject(handle, path, &args, &buf)))
		return -1;

	ret = buf.Pointer;
	val = (ret->Type == ACPI_TYPE_INTEGER) ? ret->Integer.Value : -1;

	AcpiOsFree(buf.Pointer);
	return val;

}

static ACPI_STATUS
aiboost_getcomp(ACPI_HANDLE *h, const char *name, struct aiboost_comp **comp)
{
	ACPI_BUFFER buf, buf2;
	ACPI_OBJECT *o, *elem, *subobj, *myobj;
	ACPI_STATUS status;
	struct aiboost_comp *c = NULL;
	int i;
	const char *str = NULL;
	size_t length;

	status = acpi_eval_struct(h, name, &buf);
	if (ACPI_FAILURE(status)) {
		DPRINTF(("%s: acpi_eval_struct\n", __func__));
		return status;
	}

	o = buf.Pointer;
	if (o->Type != ACPI_TYPE_PACKAGE) {
		DPRINTF(("%s: o->Type != ACPI_TYPE_PACKAGE\n", __func__));
		goto error;
	}

	elem = o->Package.Elements;
	if (elem->Type != ACPI_TYPE_INTEGER) {
		DPRINTF(("%s: elem->Type != ACPI_TYPE_INTEGER\n", __func__));
		goto error;
	}

	c = malloc(sizeof(struct aiboost_comp) + sizeof(struct aiboost_elem) *
		   (elem->Integer.Value - 1), M_DEVBUF, M_ZERO|M_WAITOK);
	*comp = c;
	c->num = elem->Integer.Value;

	for (i = 1; i < o->Package.Count; i++) {
		elem = &o->Package.Elements[i];
		if (elem->Type != ACPI_TYPE_ANY) {
			DPRINTF(("%s: elem->Type != ACPI_TYPE_ANY\n",
			    __func__));
			goto error;
		}

		c->elem[i - 1].h = elem->Reference.Handle;
		status = acpi_eval_struct(c->elem[i - 1].h, NULL, &buf2);
		if (ACPI_FAILURE(status)) {
			DPRINTF(("%s; fetching object in buf2\n",
			    __func__));
			goto error;
		}

		subobj = buf2.Pointer;
		myobj = &subobj->Package.Elements[0];

		/* Get id */
		if (myobj == NULL || myobj->Type != ACPI_TYPE_INTEGER)
			goto error;

		c->elem[i - 1].id = myobj->Integer.Value;

		/* Get string */
		myobj = &subobj->Package.Elements[1];
		if (myobj == NULL) {
			DPRINTF(("%s: myobj is NULL\n", __func__));
			goto error;
		}

		switch (myobj->Type) {
		case ACPI_TYPE_STRING:
			str = myobj->String.Pointer;
			length = myobj->String.Length;
			break;
		case ACPI_TYPE_BUFFER:
			str = myobj->Buffer.Pointer;
			length = myobj->Buffer.Length;
			break;
		default:
			goto error;
		}

		DPRINTF(("%s: id=%d str=%s\n", __func__, c->elem[i - 1].id,
		    str));

		(void)memcpy(c->elem[i - 1].desc, str, length);

		if (buf2.Pointer)
			AcpiOsFree(buf2.Pointer);
	}

	if (buf.Pointer)
		AcpiOsFree(buf.Pointer);

	return 0;

error:
	if (buf.Pointer)
		AcpiOsFree(buf.Pointer);
	if (buf2.Pointer)
		AcpiOsFree(buf2.Pointer);
	if (c)
		free(c, M_DEVBUF);

	return AE_BAD_DATA;
}
