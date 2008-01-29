/* $NetBSD: aiboost.c,v 1.21 2008/01/29 19:35:05 xtraeme Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: aiboost.c,v 1.21 2008/01/29 19:35:05 xtraeme Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/sysmon/sysmonvar.h>

#ifdef AIBOOST_DEBUG
#define DPRINTF(x)		do { printf x; } while (/* CONSTCOND */ 0)
#else
#define DPRINTF(x)
#endif

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
	struct acpi_devnode *sc_node;	/* ACPI devnode */
	struct aiboost_comp *sc_aitemp, *sc_aivolt, *sc_aifan;
	struct sysmon_envsys *sc_sme;
	envsys_data_t *sc_sensor;
	kmutex_t sc_mtx;
};

static ACPI_STATUS aiboost_getcomp(ACPI_HANDLE *,
				   const char *,
				   struct aiboost_comp **);
static int	aiboost_get_value(ACPI_HANDLE, const char *, UINT32);

/* sysmon_envsys(9) glue */
static void	aiboost_setup_sensors(struct aiboost_softc *);
static void	aiboost_refresh_sensors(struct sysmon_envsys *,
					envsys_data_t *);

/* autoconf(9) glue */
static int	aiboost_acpi_match(device_t, struct cfdata *, void *);
static void	aiboost_acpi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(aiboost, sizeof(struct aiboost_softc), aiboost_acpi_match,
    aiboost_acpi_attach, NULL, NULL);

/*
 * Supported device IDs
 */

static const char * const aiboost_acpi_ids[] = {
	"ATK0110",
	NULL
};

static int
aiboost_acpi_match(device_t parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, aiboost_acpi_ids);
}

static void
aiboost_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct aiboost_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	ACPI_HANDLE *handl;
	int i, maxsens, error = 0;
	size_t len;

	sc->sc_node = aa->aa_node;
	handl = sc->sc_node->ad_handle;

	aprint_naive("\n");
	aprint_normal("\n");

	aprint_normal_dev(self, "ASUS AI Boost Hardware monitor\n");

	if (ACPI_FAILURE(aiboost_getcomp(handl, "TSIF", &sc->sc_aitemp)))
		return;

	if (ACPI_FAILURE(aiboost_getcomp(handl, "VSIF", &sc->sc_aivolt)))
		return;

	if (ACPI_FAILURE(aiboost_getcomp(handl, "FSIF", &sc->sc_aifan)))
		return;

	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_NONE);
	/* Initialize sensors */
	maxsens = sc->sc_aivolt->num + sc->sc_aitemp->num + sc->sc_aifan->num;
	DPRINTF(("%s: maxsens=%d\n", __func__, maxsens));

	sc->sc_sme = sysmon_envsys_create();
	len = sizeof(envsys_data_t) * maxsens;
	sc->sc_sensor = kmem_zalloc(len, KM_NOSLEEP);
	if (!sc->sc_sensor)
		goto bad2;

	/*
	 * Set properties in sensors.
	 */
	aiboost_setup_sensors(sc);

	/*
	 * Add the sensors into the sysmon_envsys device.
	 */
	for (i = 0; i < maxsens; i++) {
		if (sysmon_envsys_sensor_attach(sc->sc_sme,
						&sc->sc_sensor[i]))
			goto bad;
	}

	/*
	 * Register the sysmon_envsys device.
	 */
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = aiboost_refresh_sensors;

	if ((error = sysmon_envsys_register(sc->sc_sme))) {
		aprint_error_dev(self, "unable to register with sysmon "
		    "(error=%d)\n", error);
		goto bad;
	}

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;

bad:
	kmem_free(sc->sc_sensor, len);
bad2:
	sysmon_envsys_destroy(sc->sc_sme);
	mutex_destroy(&sc->sc_mtx);
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
		sc->sc_sensor[i].units = ENVSYS_STEMP;
		COPYDESCR(sc->sc_sensor[i].desc, sc->sc_aitemp->elem[i].desc);
		DPRINTF(("%s: data[%d].desc=%s elem[%d].desc=%s\n", __func__,
		    i, sc->sc_sensor[i].desc, i, sc->sc_aitemp->elem[i].desc));
	}

	/* skip temperatures */
	j = sc->sc_aitemp->num;

	/* Voltages */
	for (i = 0; i < sc->sc_aivolt->num; i++, j++) {
		sc->sc_sensor[j].units = ENVSYS_SVOLTS_DC;
		COPYDESCR(sc->sc_sensor[j].desc, sc->sc_aivolt->elem[i].desc);
		DPRINTF(("%s: data[%d].desc=%s elem[%d].desc=%s\n", __func__,
		    j, sc->sc_sensor[j].desc, i, sc->sc_aivolt->elem[i].desc));
	}

	/* skip voltages */
	j = sc->sc_aitemp->num + sc->sc_aivolt->num;

	/* Fans */
	for (i = 0; i < sc->sc_aifan->num; i++, j++) {
		sc->sc_sensor[j].units = ENVSYS_SFANRPM;
		COPYDESCR(sc->sc_sensor[j].desc, sc->sc_aifan->elem[i].desc);
		DPRINTF(("%s: data[%d].desc=%s elem[%d].desc=%s\n", __func__,
		    j, sc->sc_sensor[j].desc, i, sc->sc_aifan->elem[i].desc));
		    
	}
}

static void
aiboost_refresh_sensors(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct aiboost_softc *sc = sme->sme_cookie;
	ACPI_HANDLE *h = sc->sc_node->ad_handle;
	int i, j, val;

	mutex_enter(&sc->sc_mtx);
	j = 0;
	i = edata->sensor; /* sensor number */

#define AIBOOST_INVALIDATE_SENSOR()					\
	do {								\
		if (val == -1 || val == 0) {				\
			edata->state = ENVSYS_SINVALID;			\
			goto out;					\
		}							\
	} while (/* CONSTCOND */ 0)

	switch (edata->units) {
	case ENVSYS_STEMP:
		/* Temperatures */
		val = aiboost_get_value(h, "RTMP", sc->sc_aitemp->elem[i].id);
		AIBOOST_INVALIDATE_SENSOR();
		/* envsys(4) wants mK... convert from Celsius. */
		edata->value_cur = val * 100000 + 273150000;
		DPRINTF(("%s: temp[%d] value_cur=%d val=%d j=%d\n", __func__,
		    i, edata->value_cur, val, j));
		break;
	case ENVSYS_SVOLTS_DC:
		/* Voltages */
		j = i - sc->sc_aitemp->num;
		val = aiboost_get_value(h, "RVLT", sc->sc_aivolt->elem[j].id);
		AIBOOST_INVALIDATE_SENSOR();
		/* envsys(4) wants mV... */
		edata->value_cur = val * 10000;
		edata->value_cur /= 10;
		DPRINTF(("%s: volt[%d] value_cur=%d val=%d j=%d\n", __func__,
		    i, edata->value_cur, val, j));
		break;
	case ENVSYS_SFANRPM:
		/* Fans */
		j = i - (sc->sc_aitemp->num + sc->sc_aivolt->num);
		val = aiboost_get_value(h, "RFAN", sc->sc_aifan->elem[j].id);
		AIBOOST_INVALIDATE_SENSOR();
		edata->value_cur = val;
		DPRINTF(("%s: fan[%d] val=%d j=%d\n", __func__, i, val, j));
		break;
	}

	edata->state = ENVSYS_SVALID;
out:
	mutex_exit(&sc->sc_mtx);
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
	size_t length, clen = 0;

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

	clen = sizeof(struct aiboost_comp) + sizeof(struct aiboost_elem) *
	    (elem->Integer.Value - 1);
	c = kmem_zalloc(clen, KM_NOSLEEP);
	if (!c)
		goto error;

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

		/* Get UID */
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

		DPRINTF(("%s: id=%d str=%s\n", __func__,
		    c->elem[i - 1].id, str));

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
		kmem_free(c, clen);

	return AE_BAD_DATA;
}
