/*	$NetBSD: wmi_acpi.c,v 1.6 2010/01/04 09:43:30 jruoho Exp $	*/

/*-
 * Copyright (c) 2009 Jukka Ruohonen <jruohonen@iki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
__KERNEL_RCSID(0, "$NetBSD: wmi_acpi.c,v 1.6 2010/01/04 09:43:30 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/kmem.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/wmi_acpivar.h>

#define _COMPONENT          ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME            ("wmi_acpi")

/*
 * This implements something called "Microsoft Windows Management
 * Instrumentation" (WMI). This subset of ACPI is desribed in:
 *
 * http://www.microsoft.com/whdc/system/pnppwr/wmi/wmi-acpi.mspx
 *
 * (Obtained on Thu Feb 12 18:21:44 EET 2009.)
 */
struct guid_t {

	/*
	 * The GUID itself. The used format is the usual 32-16-16-64-bit
	 * representation. All except the fourth field are in native byte
	 * order. A 32-16-16-16-48-bit hexadecimal notation with hyphens
	 * is used for human-readable GUIDs.
	 */
	struct {
		uint32_t data1;
		uint16_t data2;
		uint16_t data3;
		uint8_t  data4[8];
	} __packed;

	union {
		char oid[2];            /* ACPI object ID. */

		struct {
			uint8_t nid;    /* Notification value. */
			uint8_t res;    /* Reserved. */
		} __packed;
	} __packed;

	uint8_t count;	                /* Number of instances. */
	uint8_t flags;		        /* Additional flags. */

} __packed;

struct wmi_t {
	struct guid_t         guid;
	bool                  eevent;

	SIMPLEQ_ENTRY(wmi_t)  wmi_link;
};

struct acpi_wmi_softc {
	device_t	      sc_dev;
	device_t              sc_child;
        ACPI_NOTIFY_HANDLER   sc_handler;
	struct acpi_devnode  *sc_node;

	SIMPLEQ_HEAD(, wmi_t) wmi_head;
};

#define ACPI_WMI_FLAG_EXPENSIVE 0x01
#define ACPI_WMI_FLAG_METHOD    0x02
#define ACPI_WMI_FLAG_STRING    0x04
#define ACPI_WMI_FLAG_EVENT     0x08
#define ACPI_WMI_FLAG_DATA      (ACPI_WMI_FLAG_EXPENSIVE |		\
	                         ACPI_WMI_FLAG_STRING)

#define UGET16(x)   (*(uint16_t *)(x))
#define UGET64(x)   (*(uint64_t *)(x))

#define HEXCHAR(x)  (((x) >= '0' && (x) <= '9') ||			\
	             ((x) >= 'a' && (x) <= 'f') ||			\
		     ((x) >= 'A' && (x) <= 'F'))

#define GUIDCMP(a, b)							\
	((a)->data1 == (b)->data1 &&					\
	 (a)->data2 == (b)->data2 &&					\
	 (a)->data3 == (b)->data3 &&					\
	 UGET64((a)->data4) == UGET64((b)->data4))

static int         acpi_wmi_match(device_t, cfdata_t, void *);
static void        acpi_wmi_attach(device_t, device_t, void *);
static int         acpi_wmi_print(void *, const char *);
static bool        acpi_wmi_init(struct acpi_wmi_softc *);
static bool        acpi_wmi_add(struct acpi_wmi_softc *, ACPI_OBJECT *);
static void        acpi_wmi_del(struct acpi_wmi_softc *);

#ifdef ACPIVERBOSE
static void        acpi_wmi_dump(struct acpi_wmi_softc *);
#endif

static ACPI_STATUS acpi_wmi_guid_get(struct acpi_wmi_softc *,
                                     const char *, struct wmi_t **);
static void        acpi_wmi_event_add(struct acpi_wmi_softc *);
static void        acpi_wmi_event_del(struct acpi_wmi_softc *);
static void        acpi_wmi_event_handler(ACPI_HANDLE, uint32_t, void *);
static bool        acpi_wmi_suspend(device_t PMF_FN_PROTO);
static bool        acpi_wmi_resume(device_t PMF_FN_PROTO);
static ACPI_STATUS acpi_wmi_enable(ACPI_HANDLE, const char *, bool, bool);
static bool        acpi_wmi_input(struct wmi_t *, uint8_t, uint8_t);

const char * const acpi_wmi_ids[] = {
	"PNP0C14",
	NULL
};

CFATTACH_DECL_NEW(acpiwmi, sizeof(struct acpi_wmi_softc),
    acpi_wmi_match, acpi_wmi_attach, NULL, NULL);

static int
acpi_wmi_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, acpi_wmi_ids);
}

static void
acpi_wmi_attach(device_t parent, device_t self, void *aux)
{
	struct acpi_wmi_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;

	sc->sc_dev = self;
	sc->sc_node = aa->aa_node;
	sc->sc_handler = NULL;

	aprint_naive("\n");
	aprint_normal(": ACPI WMI Interface\n");

	if (acpi_wmi_init(sc) != true)
		return;

#ifdef ACPIVERBOSE
	acpi_wmi_dump(sc);
#endif

	acpi_wmi_event_add(sc);

	if (pmf_device_register(sc->sc_dev,
		acpi_wmi_suspend, acpi_wmi_resume) != true)
		aprint_error_dev(self, "failed to register power handler\n");

	/* Attach a child device to the pseudo-bus. */
	sc->sc_child = config_found_ia(self, "acpiwmibus",
	    NULL, acpi_wmi_print);
}

static int
acpi_wmi_print(void *aux, const char *pnp)
{

	if (pnp != NULL)
		aprint_normal("acpiwmibus at %s", pnp);

	return UNCONF;
}

static bool
acpi_wmi_init(struct acpi_wmi_softc *sc)
{
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	uint32_t len;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_WDG", &buf);

	if (ACPI_FAILURE(rv))
		goto fail;

	obj = buf.Pointer;
	len = obj->Buffer.Length;

	KASSERT(obj->Type == ACPI_TYPE_BUFFER);

	if (len != obj->Package.Count) {
		rv = AE_BAD_VALUE;
		goto fail;
	}

	KASSERT(sizeof(struct guid_t) == 20);

	if (len < sizeof(struct guid_t) ||
	    len % sizeof(struct guid_t) != 0) {
		rv = AE_BAD_DATA;
		goto fail;
	}

	return acpi_wmi_add(sc, obj);

fail:
	aprint_error_dev(sc->sc_dev, "failed to evaluate _WDG: %s\n",
	    AcpiFormatException(rv));

	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return false;
}

static bool
acpi_wmi_add(struct acpi_wmi_softc *sc, ACPI_OBJECT *obj)
{
	struct wmi_t *wmi;
	size_t i, n, offset, siz;

	siz = sizeof(struct guid_t);
	n = obj->Buffer.Length / siz;

	SIMPLEQ_INIT(&sc->wmi_head);

	for (i = offset = 0; i < n; ++i) {

		if ((wmi = kmem_zalloc(sizeof(*wmi), KM_NOSLEEP)) == NULL)
			goto fail;

		ACPI_MEMCPY(&wmi->guid, obj->Buffer.Pointer + offset, siz);

		wmi->eevent = false;
		offset = offset + siz;

		SIMPLEQ_INSERT_TAIL(&sc->wmi_head, wmi, wmi_link);
	}

	ACPI_FREE(obj);

	return true;

fail:
	ACPI_FREE(obj);
	acpi_wmi_del(sc);

	return false;
}

static void
acpi_wmi_del(struct acpi_wmi_softc *sc)
{
	struct wmi_t *wmi;

	if (SIMPLEQ_EMPTY(&sc->wmi_head))
		return;

	while (SIMPLEQ_FIRST(&sc->wmi_head) != NULL) {

		wmi = SIMPLEQ_FIRST(&sc->wmi_head);
		SIMPLEQ_REMOVE_HEAD(&sc->wmi_head, wmi_link);

		KASSERT(wmi != NULL);

		kmem_free(wmi, sizeof(*wmi));
	}
}

#ifdef ACPIVERBOSE
static void
acpi_wmi_dump(struct acpi_wmi_softc *sc)
{
	struct wmi_t *wmi;

	KASSERT(!(SIMPLEQ_EMPTY(&sc->wmi_head)));

	SIMPLEQ_FOREACH(wmi, &sc->wmi_head, wmi_link) {

		aprint_normal_dev(sc->sc_dev, "{%08X-%04X-%04X-",
		    wmi->guid.data1, wmi->guid.data2, wmi->guid.data3);

		aprint_normal("%02X%02X-%02X%02X%02X%02X%02X%02X} ",
		    wmi->guid.data4[0], wmi->guid.data4[1],
		    wmi->guid.data4[2], wmi->guid.data4[3],
		    wmi->guid.data4[4], wmi->guid.data4[5],
		    wmi->guid.data4[6], wmi->guid.data4[7]);

		aprint_normal("oid %04X count %02X flags %02X\n",
		    UGET16(wmi->guid.oid), wmi->guid.count, wmi->guid.flags);
	}
}
#endif

static ACPI_STATUS
acpi_wmi_guid_get(struct acpi_wmi_softc *sc,
    const char *src, struct wmi_t **out)
{
	struct wmi_t *wmi;
	struct guid_t *guid;
	char bin[16];
	char hex[2];
	const char *ptr;
	uint8_t i;

	if (sc == NULL || src == NULL || ACPI_STRLEN(src) != 36)
		return AE_BAD_PARAMETER;

	for (ptr = src, i = 0; i < 16; i++) {

		if (*ptr == '-')
			ptr++;

		ACPI_MEMCPY(hex, ptr, 2);

		if (!HEXCHAR(hex[0]) || !HEXCHAR(hex[1]))
			return AE_BAD_HEX_CONSTANT;

		bin[i] = ACPI_STRTOUL(hex, NULL, 16) & 0xFF;

		ptr++;
		ptr++;
	}

	guid = (struct guid_t *)bin;
	guid->data1 = be32toh(guid->data1);
	guid->data2 = be16toh(guid->data2);
	guid->data3 = be16toh(guid->data3);

	SIMPLEQ_FOREACH(wmi, &sc->wmi_head, wmi_link) {

		if (GUIDCMP(guid, &wmi->guid)) {

			if (out != NULL)
				*out = wmi;

			return AE_OK;
		}
	}

	return AE_NOT_FOUND;
}

/*
 * Checks if a GUID is present. Child devices
 * can use this in their autoconf(9) routines.
 */
int
acpi_wmi_guid_match(device_t self, const char *guid)
{
	struct acpi_wmi_softc *sc = device_private(self);

	if (ACPI_SUCCESS(acpi_wmi_guid_get(sc, guid, NULL)))
		return 1;

	return 0;
}

/*
 * Adds internal event handler.
 */
static void
acpi_wmi_event_add(struct acpi_wmi_softc *sc)
{
	struct wmi_t *wmi;
	ACPI_STATUS rv;

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_ALL_NOTIFY, acpi_wmi_event_handler, sc);

	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "failed to install notify "
		    "handler: %s\n", AcpiFormatException(rv));
		return;
	}

	/* Enable possible expensive events. */
	SIMPLEQ_FOREACH(wmi, &sc->wmi_head, wmi_link) {

		if ((wmi->guid.flags & ACPI_WMI_FLAG_EVENT) &&
		    (wmi->guid.flags & ACPI_WMI_FLAG_EXPENSIVE)) {

			rv = acpi_wmi_enable(sc->sc_node->ad_handle,
			    wmi->guid.oid, false, true);

			if (ACPI_SUCCESS(rv)) {
				wmi->eevent = true;
				continue;
			}

			aprint_error_dev(sc->sc_dev, "failed to enable "
			    "expensive WExx: %s\n", AcpiFormatException(rv));
		}
	}
}

/*
 * Removes the internal event handler.
 */
static void
acpi_wmi_event_del(struct acpi_wmi_softc *sc)
{
	struct wmi_t *wmi;
	ACPI_STATUS rv;

	rv = AcpiRemoveNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_ALL_NOTIFY, acpi_wmi_event_handler);

	if (ACPI_FAILURE(rv)) {
		aprint_debug_dev(sc->sc_dev, "failed to remove notify "
		    "handler: %s\n", AcpiFormatException(rv));
		return;
	}

	SIMPLEQ_FOREACH(wmi, &sc->wmi_head, wmi_link) {

		if (wmi->eevent != true)
			continue;

		KASSERT(wmi->guid.flags & ACPI_WMI_FLAG_EVENT);
		KASSERT(wmi->guid.flags & ACPI_WMI_FLAG_EXPENSIVE);

		rv = acpi_wmi_enable(sc->sc_node->ad_handle,
		    wmi->guid.oid, false, false);

		if (ACPI_SUCCESS(rv)) {
			wmi->eevent = false;
			continue;
		}

		aprint_error_dev(sc->sc_dev, "failed to disable "
		    "expensive WExx: %s\n", AcpiFormatException(rv));
	}
}

/*
 * Returns extra information possibly associated with an event.
 */
ACPI_STATUS
acpi_wmi_event_get(device_t self, uint32_t event, ACPI_BUFFER *obuf)
{
	struct acpi_wmi_softc *sc = device_private(self);
	struct wmi_t *wmi;
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT obj;

	if (sc == NULL || obuf == NULL)
		return AE_BAD_PARAMETER;

	if (sc->sc_handler == NULL)
		return AE_ABORT_METHOD;

	obj.Type = ACPI_TYPE_INTEGER;
	obj.Integer.Value = event;

	arg.Count = 0x01;
	arg.Pointer = &obj;

	obuf->Pointer = NULL;
	obuf->Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	SIMPLEQ_FOREACH(wmi, &sc->wmi_head, wmi_link) {

		if (!(wmi->guid.flags & ACPI_WMI_FLAG_EVENT))
			continue;

		if (wmi->guid.nid != event)
			continue;

		return AcpiEvaluateObject(sc->sc_node->ad_handle, "_WED",
		    &arg, obuf);
	}

	return AE_NOT_FOUND;
}

/*
 * Forwards events to the external handler through the internal one.
 */
static void
acpi_wmi_event_handler(ACPI_HANDLE hdl, uint32_t evt, void *aux)
{
	device_t self = aux;
	struct acpi_wmi_softc *sc = device_private(self);

	if (sc->sc_handler == NULL)
		return;

	(*sc->sc_handler)(NULL, evt, sc->sc_child);
}

/*
 * Adds or removes (NULL) the external event handler.
 */
ACPI_STATUS
acpi_wmi_event_register(device_t self, ACPI_NOTIFY_HANDLER handler)
{
	struct acpi_wmi_softc *sc = device_private(self);

	if (sc == NULL)
		return AE_BAD_PARAMETER;

	if (handler != NULL && sc->sc_handler != NULL)
		return AE_ALREADY_EXISTS;

	sc->sc_handler = handler;

	return AE_OK;
}

/*
 * As there is no prior knowledge about the expensive
 * events that cause "significant overhead", try to
 * disable (enable) these before suspending (resuming).
 */
static bool
acpi_wmi_suspend(device_t self PMF_FN_ARGS)
{
	struct acpi_wmi_softc *sc = device_private(self);

	acpi_wmi_event_del(sc);

	return true;
}

static bool
acpi_wmi_resume(device_t self PMF_FN_ARGS)
{
	struct acpi_wmi_softc *sc = device_private(self);

	acpi_wmi_event_add(sc);

	return true;
}

/*
 * Enables or disables data collection (WCxx) or an event (WExx).
 */
static ACPI_STATUS
acpi_wmi_enable(ACPI_HANDLE hdl, const char *oid, bool data, bool flag)
{
	char path[5];
	const char *str;

	str = (data != false) ? "WC" : "WE";

	(void)strlcpy(path, str, sizeof(path));
	(void)strlcat(path, oid, sizeof(path));

	return acpi_eval_set_integer(hdl, path, (flag != false) ? 0x01 : 0x00);
}

static bool
acpi_wmi_input(struct wmi_t *wmi, uint8_t flag, uint8_t idx)
{

	if (!(wmi->guid.flags & flag))
		return false;

	if (wmi->guid.count == 0x00 || wmi->guid.flags == 0x00)
		return false;

	if (wmi->guid.count < idx)
		return false;

	return true;
}

/*
 * Makes a WMI data block query (WQxx). The corresponding control
 * method for data collection will be invoked if it is available.
 */
ACPI_STATUS
acpi_wmi_data_query(device_t self, const char *guid,
    uint8_t idx, ACPI_BUFFER *obuf)
{
	struct acpi_wmi_softc *sc = device_private(self);
	struct wmi_t *wmi;
	char path[5] = "WQ";
	ACPI_OBJECT_LIST arg;
	ACPI_STATUS rv, rvxx;
	ACPI_OBJECT obj;

	rvxx = AE_SUPPORT;

	if (obuf == NULL)
		return AE_BAD_PARAMETER;

	rv = acpi_wmi_guid_get(sc, guid, &wmi);

	if (ACPI_FAILURE(rv))
		return rv;

	if (acpi_wmi_input(wmi, ACPI_WMI_FLAG_DATA, idx) != true)
		return AE_BAD_DATA;

	(void)strlcat(path, wmi->guid.oid, sizeof(path));

	obj.Type = ACPI_TYPE_INTEGER;
	obj.Integer.Value = idx;

	arg.Count = 0x01;
	arg.Pointer = &obj;

	obuf->Pointer = NULL;
	obuf->Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	/*
	 * If the expensive flag is set, we should enable
	 * data collection before evaluating the WQxx buffer.
	 */
	if (wmi->guid.flags & ACPI_WMI_FLAG_EXPENSIVE) {

		rvxx = acpi_wmi_enable(sc->sc_node->ad_handle,
		    wmi->guid.oid, true, true);
	}

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, path, &arg, obuf);

	/* No longer needed. */
	if (ACPI_SUCCESS(rvxx)) {

		(void)acpi_wmi_enable(sc->sc_node->ad_handle,
		    wmi->guid.oid, true, false);
	}

#ifdef DIAGNOSTIC
	/*
	 * XXX: It appears that quite a few laptops have WQxx
	 * methods that are declared as expensive, but lack the
	 * corresponding WCxx control method.
	 *
	 * -- Acer Aspire One is one example <jruohonen@iki.fi>.
	 */
	if (ACPI_FAILURE(rvxx) && rvxx != AE_SUPPORT)
		aprint_error_dev(sc->sc_dev, "failed to evaluate WCxx "
		    "for %s: %s\n", path, AcpiFormatException(rvxx));
#endif
	return rv;
}

/*
 * Writes to a data block (WSxx).
 */
ACPI_STATUS
acpi_wmi_data_write(device_t self, const char *guid,
    uint8_t idx, ACPI_BUFFER *ibuf)
{
	struct acpi_wmi_softc *sc = device_private(self);
	struct wmi_t *wmi;
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT obj[2];
	char path[5] = "WS";
	ACPI_STATUS rv;

	if (ibuf == NULL)
		return AE_BAD_PARAMETER;

	rv = acpi_wmi_guid_get(sc, guid, &wmi);

	if (ACPI_FAILURE(rv))
		return rv;

	if (acpi_wmi_input(wmi, ACPI_WMI_FLAG_DATA, idx) != true)
		return AE_BAD_DATA;

	(void)strlcat(path, wmi->guid.oid, sizeof(path));

	obj[0].Integer.Value = idx;
	obj[0].Type = ACPI_TYPE_INTEGER;

	obj[1].Buffer.Length = ibuf->Length;
	obj[1].Buffer.Pointer = ibuf->Pointer;

	obj[1].Type = (wmi->guid.flags & ACPI_WMI_FLAG_STRING) ?
	    ACPI_TYPE_STRING : ACPI_TYPE_INTEGER;

	arg.Count = 0x02;
	arg.Pointer = obj;

	return AcpiEvaluateObject(sc->sc_node->ad_handle, path, &arg, NULL);
}

/*
 * Executes a method (WMxx).
 */
ACPI_STATUS
acpi_wmi_method(device_t self, const char *guid, uint8_t idx,
    uint32_t mid, ACPI_BUFFER *ibuf, ACPI_BUFFER *obuf)
{
	struct acpi_wmi_softc *sc = device_private(self);
	struct wmi_t *wmi;
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT obj[3];
	char path[5] = "WM";
	ACPI_STATUS rv;

	if (ibuf == NULL || obuf == NULL)
		return AE_BAD_PARAMETER;

	rv = acpi_wmi_guid_get(sc, guid, &wmi);

	if (ACPI_FAILURE(rv))
		return rv;

	if (acpi_wmi_input(wmi, ACPI_WMI_FLAG_METHOD, idx) != true)
		return AE_BAD_DATA;

	(void)strlcat(path, wmi->guid.oid, sizeof(path));

	obj[0].Integer.Value = idx;
	obj[1].Integer.Value = mid;
	obj[0].Type = obj[1].Type = ACPI_TYPE_INTEGER;

	obj[2].Buffer.Length = ibuf->Length;
	obj[2].Buffer.Pointer = ibuf->Pointer;

	obj[2].Type = (wmi->guid.flags & ACPI_WMI_FLAG_STRING) ?
	    ACPI_TYPE_STRING : ACPI_TYPE_INTEGER;

	arg.Count = 0x03;
	arg.Pointer = obj;

	obuf->Pointer = NULL;
	obuf->Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	return AcpiEvaluateObject(sc->sc_node->ad_handle, path, &arg, obuf);
}
