/*	$NetBSD: acpi_util.c,v 1.32 2022/01/22 11:49:17 thorpej Exp $ */

/*-
 * Copyright (c) 2003, 2007, 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum of By Noon Software, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright 2001, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_util.c,v 1.32 2022/01/22 11:49:17 thorpej Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/cpu.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

#include <sys/device_calls.h>

#include <machine/acpi_machdep.h>

#define _COMPONENT	ACPI_BUS_COMPONENT
ACPI_MODULE_NAME	("acpi_util")

static void		acpi_clean_node(ACPI_HANDLE, void *);
static ACPI_STATUS	acpi_dsd_property(ACPI_HANDLE, const char *,
			    ACPI_BUFFER *, ACPI_OBJECT_TYPE, ACPI_OBJECT **);

static const char * const acpicpu_ids[] = {
	"ACPI0007",
	NULL
};

static const struct device_compatible_entry dtlink_compat_data[] = {
	{ .compat = "PRP0001" },
	DEVICE_COMPAT_EOL
};

/*
 * ACPI device handle support.
 */

static device_call_t
acpi_devhandle_lookup_device_call(devhandle_t handle, const char *name,
    devhandle_t *call_handlep)
{
	__link_set_decl(acpi_device_calls, struct device_call_descriptor);
	struct device_call_descriptor * const *desc;

	__link_set_foreach(desc, acpi_device_calls) {
		if (strcmp((*desc)->name, name) == 0) {
			return (*desc)->call;
		}
	}
	return NULL;
}

static const struct devhandle_impl acpi_devhandle_impl = {
	.type = DEVHANDLE_TYPE_ACPI,
	.lookup_device_call = acpi_devhandle_lookup_device_call,
};

devhandle_t
devhandle_from_acpi(devhandle_t super_handle, ACPI_HANDLE const hdl)
{
	devhandle_type_t super_type = devhandle_type(super_handle);
	devhandle_t handle = { 0 };

	if (super_type == DEVHANDLE_TYPE_ACPI) {
		handle.impl = super_handle.impl;
	} else {
		KASSERT(super_type == DEVHANDLE_TYPE_INVALID);
		handle.impl = &acpi_devhandle_impl;
	}
	handle.pointer = hdl;

	return handle;
}

ACPI_HANDLE
devhandle_to_acpi(devhandle_t const handle)
{
	KASSERT(devhandle_type(handle) == DEVHANDLE_TYPE_ACPI);

	return handle.pointer;
}

static int
acpi_device_enumerate_children(device_t dev, devhandle_t call_handle, void *v)
{
	struct device_enumerate_children_args *args = v;
	ACPI_HANDLE hdl = devhandle_to_acpi(call_handle);
	struct acpi_devnode *devnode, *ad;

	devnode = acpi_match_node(hdl);
	KASSERT(devnode != NULL);

	SIMPLEQ_FOREACH(ad, &devnode->ad_child_head, ad_child_list) {
		if (ad->ad_devinfo->Type != ACPI_TYPE_DEVICE ||
		    !acpi_device_present(ad->ad_handle)) {
			continue;
		}
		if (!args->callback(dev, devhandle_from_acpi(call_handle,
							     ad->ad_handle),
				    args->callback_arg)) {
			break;
		}
	}

	return 0;
}
ACPI_DEVICE_CALL_REGISTER(DEVICE_ENUMERATE_CHILDREN_STR,
			  acpi_device_enumerate_children)

/*
 * Evaluate an integer object.
 */
ACPI_STATUS
acpi_eval_integer(ACPI_HANDLE handle, const char *path, ACPI_INTEGER *valp)
{
	ACPI_OBJECT obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	(void)memset(&obj, 0, sizeof(obj));
	buf.Pointer = &obj;
	buf.Length = sizeof(obj);

	rv = AcpiEvaluateObject(handle, path, NULL, &buf);

	if (ACPI_FAILURE(rv))
		return rv;

	/* Check that evaluation produced a return value. */
	if (buf.Length == 0)
		return AE_NULL_OBJECT;

	if (obj.Type != ACPI_TYPE_INTEGER)
		return AE_TYPE;

	if (valp != NULL)
		*valp = obj.Integer.Value;

	return AE_OK;
}

/*
 * Evaluate an integer object with a single integer input parameter.
 */
ACPI_STATUS
acpi_eval_set_integer(ACPI_HANDLE handle, const char *path, ACPI_INTEGER val)
{
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT obj;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	obj.Type = ACPI_TYPE_INTEGER;
	obj.Integer.Value = val;

	arg.Count = 1;
	arg.Pointer = &obj;

	return AcpiEvaluateObject(handle, path, &arg, NULL);
}

/*
 * Evaluate a (Unicode) string object.
 */
ACPI_STATUS
acpi_eval_string(ACPI_HANDLE handle, const char *path, char **stringp)
{
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	rv = acpi_eval_struct(handle, path, &buf);

	if (ACPI_FAILURE(rv))
		return rv;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_STRING) {
		rv = AE_TYPE;
		goto out;
	}

	if (obj->String.Length == 0) {
		rv = AE_BAD_DATA;
		goto out;
	}

	*stringp = ACPI_ALLOCATE(obj->String.Length + 1);

	if (*stringp == NULL) {
		rv = AE_NO_MEMORY;
		goto out;
	}

	(void)memcpy(*stringp, obj->String.Pointer, obj->String.Length);

	(*stringp)[obj->String.Length] = '\0';

out:
	ACPI_FREE(buf.Pointer);

	return rv;
}

/*
 * Evaluate a structure. Caller must free buf.Pointer by ACPI_FREE().
 */
ACPI_STATUS
acpi_eval_struct(ACPI_HANDLE handle, const char *path, ACPI_BUFFER *buf)
{

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	buf->Pointer = NULL;
	buf->Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	return AcpiEvaluateObject(handle, path, NULL, buf);
}

/*
 * Evaluate a reference handle from an element in a package.
 */
ACPI_STATUS
acpi_eval_reference_handle(ACPI_OBJECT *elm, ACPI_HANDLE *handle)
{

	if (elm == NULL || handle == NULL)
		return AE_BAD_PARAMETER;

	switch (elm->Type) {

	case ACPI_TYPE_ANY:
	case ACPI_TYPE_LOCAL_REFERENCE:

		if (elm->Reference.Handle == NULL)
			return AE_NULL_ENTRY;

		*handle = elm->Reference.Handle;

		return AE_OK;

	case ACPI_TYPE_STRING:
		return AcpiGetHandle(NULL, elm->String.Pointer, handle);

	default:
		return AE_TYPE;
	}
}

/*
 * Iterate over all objects in a package, and pass them all
 * to a function. If the called function returns non-AE_OK,
 * the iteration is stopped and that value is returned.
 */
ACPI_STATUS
acpi_foreach_package_object(ACPI_OBJECT *pkg,
    ACPI_STATUS (*func)(ACPI_OBJECT *, void *), void *arg)
{
	ACPI_STATUS rv = AE_OK;
	uint32_t i;

	if (pkg == NULL)
		return AE_BAD_PARAMETER;

	if (pkg->Type != ACPI_TYPE_PACKAGE)
		return AE_TYPE;

	for (i = 0; i < pkg->Package.Count; i++) {

		rv = (*func)(&pkg->Package.Elements[i], arg);

		if (ACPI_FAILURE(rv))
			break;
	}

	return rv;
}

/*
 * Fetch data info the specified (empty) ACPI buffer.
 * Caller must free buf.Pointer by ACPI_FREE().
 */
ACPI_STATUS
acpi_get(ACPI_HANDLE handle, ACPI_BUFFER *buf,
    ACPI_STATUS (*getit)(ACPI_HANDLE, ACPI_BUFFER *))
{

	buf->Pointer = NULL;
	buf->Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	return (*getit)(handle, buf);
}

/*
 * Return a complete pathname from a handle.
 *
 * Note that the function uses static data storage;
 * if the data is needed for future use, it should be
 * copied before any subsequent calls overwrite it.
 */
const char *
acpi_name(ACPI_HANDLE handle)
{
	static char name[80];
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	buf.Pointer = name;
	buf.Length = sizeof(name);

	rv = AcpiGetName(handle, ACPI_FULL_PATHNAME, &buf);

	if (ACPI_FAILURE(rv))
		return "UNKNOWN";

	return name;
}

/*
 * Pack _HID and _CID ID strings into an OpenFirmware-style
 * string list.
 */
char *
acpi_pack_compat_list(ACPI_DEVICE_INFO *ad, size_t *sizep)
{
	KASSERT(sizep != NULL);

	char *sl = NULL;
	size_t slsize = 0;
	uint32_t i;

	if ((ad->Valid & ACPI_VALID_HID) != 0) {
		strlist_append(&sl, &slsize, ad->HardwareId.String);
	}

	if ((ad->Valid & ACPI_VALID_CID) != 0) {
		for (i = 0; i < ad->CompatibleIdList.Count; i++) {
			strlist_append(&sl, &slsize,
			    ad->CompatibleIdList.Ids[i].String);
		}
	}

	*sizep = slsize;
	return sl;
}

/*
 * The ACPI_PNP_DEVICE_ID type is somewhat inconvenient for us to
 * use.  We'll need some temporary space to pack it into an array
 * of C strings.  Room for 8 should be plenty, but we can allocate
 * more if necessary.
 */
#define	ACPI_COMPATSTR_MAX	8

static const char **
acpi_compatible_alloc_strarray(ACPI_PNP_DEVICE_ID *ids,
    unsigned int count, const char **buf)
{
	unsigned int i;

	buf = kmem_tmpbuf_alloc(count * sizeof(const char *),
	    buf, ACPI_COMPATSTR_MAX * sizeof(const char *), KM_SLEEP);
	for (i = 0; i < count; i++) {
		buf[i] = ids[i].String;
	}
	return buf;
}

static void
acpi_compatible_free_strarray(const char **cpp, unsigned int count,
    const char **buf)
{
	kmem_tmpbuf_free(cpp, count * sizeof(const char *), buf);
}

static int
acpi_compatible_match_dtlink(const struct acpi_attach_args * const aa,
    const struct device_compatible_entry * const dce)
{
	const char *strings[ACPI_COMPATSTR_MAX * sizeof(const char *)];
	ACPI_HANDLE handle = aa->aa_node->ad_handle;
	ACPI_BUFFER buf;
	char *compatible;
	ACPI_STATUS ret;
	ACPI_OBJECT *obj;
	int rv = 0, n;

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_BUFFER;

	/* Match a single string _DSD value */
	ret = acpi_dsd_string(handle, "compatible", &compatible);
	if (ACPI_SUCCESS(ret)) {
		strings[0] = compatible;
		rv = device_compatible_pmatch(strings, 1, dce);
		kmem_strfree(compatible);
		goto done;
	}

	/* Match from a list of strings in a _DSD value */
	ret = acpi_dsd_property(handle, "compatible", &buf,
	    ACPI_TYPE_PACKAGE, &obj);
	if (ACPI_FAILURE(ret)) {
		goto done;
	}
	if (obj->Package.Count == 0) {
		goto done;
	}
	for (n = 0; n < imin(obj->Package.Count, ACPI_COMPATSTR_MAX); n++) {
		if (obj->Package.Elements[n].Type != ACPI_TYPE_STRING) {
			goto done;
		}
		strings[n] = obj->Package.Elements[n].String.Pointer;
	}
	rv = device_compatible_pmatch(strings, n, dce);

done:
	if (buf.Pointer != NULL) {
		ACPI_FREE(buf.Pointer);
	}
	if (rv) {
		rv = (rv - 1) + ACPI_MATCHSCORE_CID;
		return imin(rv, ACPI_MATCHSCORE_CID_MAX);
	}
	return 0;
}

/*
 * acpi_compatible_match --
 *
 *	Returns a weighted match value, comparing the _HID and _CID
 *	IDs against a driver's compatibility data.
 */
int
acpi_compatible_match(const struct acpi_attach_args * const aa,
    const struct device_compatible_entry * const dce)
{
	const char *strings[ACPI_COMPATSTR_MAX * sizeof(const char *)];
	const char **cpp;
	bool dtlink = false;
	int rv;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE) {
		return 0;
	}

	ACPI_DEVICE_INFO *ad = aa->aa_node->ad_devinfo;

	if ((ad->Valid & ACPI_VALID_HID) != 0) {
		strings[0] = ad->HardwareId.String;

		/* Matching _HID wins big. */
		if (device_compatible_pmatch(strings, 1, dce) != 0) {
			return ACPI_MATCHSCORE_HID;
		}

		if (device_compatible_pmatch(strings, 1,
					     dtlink_compat_data) != 0) {
			dtlink = true;
		}
	}

	if ((ad->Valid & ACPI_VALID_CID) != 0) {
		cpp = acpi_compatible_alloc_strarray(ad->CompatibleIdList.Ids,
		    ad->CompatibleIdList.Count, strings);

		rv = device_compatible_pmatch(cpp,
		    ad->CompatibleIdList.Count, dce);
		if (!dtlink &&
		    device_compatible_pmatch(cpp, ad->CompatibleIdList.Count,
					     dtlink_compat_data) != 0) {
			dtlink = true;
		}
		acpi_compatible_free_strarray(cpp, ad->CompatibleIdList.Count,
		    strings);
		if (rv) {
			rv = (rv - 1) + ACPI_MATCHSCORE_CID;
			return imin(rv, ACPI_MATCHSCORE_CID_MAX);
		}
	}

	if (dtlink) {
		return acpi_compatible_match_dtlink(aa, dce);
	}

	return 0;
}

/*
 * acpi_compatible_lookup --
 *
 *	Returns the device_compatible_entry that matches the _HID
 *	or _CID ID.
 */
const struct device_compatible_entry *
acpi_compatible_lookup(const struct acpi_attach_args * const aa,
    const struct device_compatible_entry * const dce)
{
	const struct device_compatible_entry *rv = NULL;
	const char *strings[ACPI_COMPATSTR_MAX];
	const char **cpp;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE) {
		return NULL;
	}

	ACPI_DEVICE_INFO *ad = aa->aa_node->ad_devinfo;

	if ((ad->Valid & ACPI_VALID_HID) != 0) {
		strings[0] = ad->HardwareId.String;

		rv = device_compatible_plookup(strings, 1, dce);
		if (rv != NULL)
			return rv;
	}

	if ((ad->Valid & ACPI_VALID_CID) != 0) {
		cpp = acpi_compatible_alloc_strarray(ad->CompatibleIdList.Ids,
		    ad->CompatibleIdList.Count, strings);

		rv = device_compatible_plookup(cpp,
		    ad->CompatibleIdList.Count, dce);
		acpi_compatible_free_strarray(cpp, ad->CompatibleIdList.Count,
		    strings);
	}

	return rv;
}

/*
 * Match given IDs against _HID and _CIDs.
 */
int
acpi_match_hid(ACPI_DEVICE_INFO *ad, const char * const *ids)
{
	uint32_t i, n;
	char *id;

	while (*ids) {

		if ((ad->Valid & ACPI_VALID_HID) != 0) {

			if (pmatch(ad->HardwareId.String, *ids, NULL) == 2)
				return 1;
		}

		if ((ad->Valid & ACPI_VALID_CID) != 0) {

			n = ad->CompatibleIdList.Count;

			for (i = 0; i < n; i++) {

				id = ad->CompatibleIdList.Ids[i].String;

				if (pmatch(id, *ids, NULL) == 2)
					return 1;
			}
		}

		ids++;
	}

	return 0;
}

/*
 * Match a PCI-defined bass-class, sub-class, and programming interface
 * against a handle's _CLS object.
 */
int
acpi_match_class(ACPI_HANDLE handle, uint8_t pci_class, uint8_t pci_subclass,
    uint8_t pci_interface)
{
	ACPI_BUFFER buf;
	ACPI_OBJECT *obj;
	ACPI_STATUS rv;
	int match = 0;

	rv = acpi_eval_struct(handle, "_CLS", &buf);
	if (ACPI_FAILURE(rv))
		goto done;

	obj = buf.Pointer;
	if (obj->Type != ACPI_TYPE_PACKAGE)
		goto done;
	if (obj->Package.Count != 3)
		goto done;
	if (obj->Package.Elements[0].Type != ACPI_TYPE_INTEGER ||
	    obj->Package.Elements[1].Type != ACPI_TYPE_INTEGER ||
	    obj->Package.Elements[2].Type != ACPI_TYPE_INTEGER)
		goto done;

	match = obj->Package.Elements[0].Integer.Value == pci_class &&
		obj->Package.Elements[1].Integer.Value == pci_subclass &&
		obj->Package.Elements[2].Integer.Value == pci_interface;

done:
	if (buf.Pointer)
		ACPI_FREE(buf.Pointer);
	return match ? ACPI_MATCHSCORE_CLS : 0;
}

/*
 * Match a device node from a handle.
 */
struct acpi_devnode *
acpi_match_node(ACPI_HANDLE handle)
{
	struct acpi_devnode *ad;
	ACPI_STATUS rv;

	if (handle == NULL)
		return NULL;

	rv = AcpiGetData(handle, acpi_clean_node, (void **)&ad);

	if (ACPI_FAILURE(rv))
		return NULL;

	return ad;
}

/*
 * Permanently associate a device node with a handle.
 */
void
acpi_match_node_init(struct acpi_devnode *ad)
{
	(void)AcpiAttachData(ad->ad_handle, acpi_clean_node, ad);
}

static void
acpi_clean_node(ACPI_HANDLE handle, void *aux)
{
	/* Nothing. */
}

/*
 * Match a handle from a cpu_info. Returns NULL on failure.
 *
 * Note that acpi_match_node() can be used if the device node
 * is also required.
 */
ACPI_HANDLE
acpi_match_cpu_info(struct cpu_info *ci)
{
	struct acpi_softc *sc = acpi_softc;
	struct acpi_devnode *ad;
	ACPI_INTEGER val;
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_HANDLE hdl;
	ACPI_STATUS rv;

	if (sc == NULL || acpi_active == 0)
		return NULL;

	/*
	 * CPUs are declared in the ACPI namespace
	 * either as a Processor() or as a Device().
	 * In both cases the MADT entries are used
	 * for the match (see ACPI 4.0, section 8.4).
	 */
	SIMPLEQ_FOREACH(ad, &sc->sc_head, ad_list) {

		hdl = ad->ad_handle;

		switch (ad->ad_type) {

		case ACPI_TYPE_DEVICE:

			if (acpi_match_hid(ad->ad_devinfo, acpicpu_ids) == 0)
				break;

			rv = acpi_eval_integer(hdl, "_UID", &val);

			if (ACPI_SUCCESS(rv) && val == ci->ci_acpiid)
				return hdl;

			break;

		case ACPI_TYPE_PROCESSOR:

			rv = acpi_eval_struct(hdl, NULL, &buf);

			if (ACPI_FAILURE(rv))
				break;

			obj = buf.Pointer;

			if (obj->Processor.ProcId == ci->ci_acpiid) {
				ACPI_FREE(buf.Pointer);
				return hdl;
			}

			ACPI_FREE(buf.Pointer);
			break;
		}
	}

	return NULL;
}

/*
 * Match a CPU from a handle. Returns NULL on failure.
 */
struct cpu_info *
acpi_match_cpu_handle(ACPI_HANDLE hdl)
{
	struct cpu_info *ci;
	ACPI_DEVICE_INFO *di;
	CPU_INFO_ITERATOR cii;
	ACPI_INTEGER val;
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	ci = NULL;
	di = NULL;
	buf.Pointer = NULL;

	rv = AcpiGetObjectInfo(hdl, &di);

	if (ACPI_FAILURE(rv))
		return NULL;

	switch (di->Type) {

	case ACPI_TYPE_DEVICE:

		if (acpi_match_hid(di, acpicpu_ids) == 0)
			goto out;

		rv = acpi_eval_integer(hdl, "_UID", &val);

		if (ACPI_FAILURE(rv))
			goto out;

		break;

	case ACPI_TYPE_PROCESSOR:

		rv = acpi_eval_struct(hdl, NULL, &buf);

		if (ACPI_FAILURE(rv))
			goto out;

		obj = buf.Pointer;
		val = obj->Processor.ProcId;
		break;

	default:
		goto out;
	}

	for (CPU_INFO_FOREACH(cii, ci)) {

		if (ci->ci_acpiid == val)
			goto out;
	}

	ci = NULL;

out:
	if (di != NULL)
		ACPI_FREE(di);

	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	return ci;
}

struct acpi_irq_handler {
	uint32_t aih_irq;
	void *aih_ih;
};

void *
acpi_intr_establish(device_t dev, uint64_t c, int ipl, bool mpsafe,
    int (*intr)(void *), void *iarg, const char *xname)
{
	ACPI_STATUS rv;
	ACPI_HANDLE hdl = (void *)(uintptr_t)c;
	struct acpi_resources res;
	struct acpi_irq *irq;
	void *aih = NULL;

	rv = acpi_resource_parse(dev, hdl, "_CRS", &res,
	    &acpi_resource_parse_ops_quiet);
	if (ACPI_FAILURE(rv))
		return NULL;

	irq = acpi_res_irq(&res, 0);
	if (irq == NULL)
		goto end;

	aih = acpi_intr_establish_irq(dev, irq, ipl, mpsafe,
	    intr, iarg, xname);

end:
	acpi_resource_cleanup(&res);

	return aih;
}

void *
acpi_intr_establish_irq(device_t dev, struct acpi_irq *irq, int ipl,
    bool mpsafe, int (*intr)(void *), void *iarg, const char *xname)
{
	struct acpi_irq_handler *aih;
	void *ih;

	const int type = (irq->ar_type == ACPI_EDGE_SENSITIVE) ? IST_EDGE : IST_LEVEL;
	ih = acpi_md_intr_establish(irq->ar_irq, ipl, type, intr, iarg, mpsafe, xname);
	if (ih == NULL)
		return NULL;

	aih = kmem_alloc(sizeof(struct acpi_irq_handler), KM_SLEEP);
	aih->aih_irq = irq->ar_irq;
	aih->aih_ih = ih;

	return aih;
}

void
acpi_intr_mask(void *c)
{
	struct acpi_irq_handler * const aih = c;

	acpi_md_intr_mask(aih->aih_ih);
}

void
acpi_intr_unmask(void *c)
{
	struct acpi_irq_handler * const aih = c;

	acpi_md_intr_unmask(aih->aih_ih);
}

void
acpi_intr_disestablish(void *c)
{
	struct acpi_irq_handler *aih = c;

	acpi_md_intr_disestablish(aih->aih_ih);
	kmem_free(aih, sizeof(struct acpi_irq_handler));
}

const char *
acpi_intr_string(void *c, char *buf, size_t size)
{
	struct acpi_irq_handler *aih = c;
	intr_handle_t ih = aih->aih_irq;

	return intr_string(ih, buf, size);
}

/*
 * Device-Specific Data (_DSD) support
 */

static UINT8 acpi_dsd_uuid[ACPI_UUID_LENGTH] = {
	0x14, 0xd8, 0xff, 0xda, 0xba, 0x6e, 0x8c, 0x4d,
	0x8a, 0x91, 0xbc, 0x9b, 0xbf, 0x4a, 0xa3, 0x01
};

static ACPI_STATUS
acpi_dsd_property(ACPI_HANDLE handle, const char *prop, ACPI_BUFFER *pbuf, ACPI_OBJECT_TYPE type, ACPI_OBJECT **ret)
{
	ACPI_OBJECT *obj, *uuid, *props, *pobj, *propkey, *propval;
	ACPI_STATUS rv;
	int n;

	rv = AcpiEvaluateObjectTyped(handle, "_DSD", NULL, pbuf, ACPI_TYPE_PACKAGE);
	if (ACPI_FAILURE(rv))
		return rv;

	props = NULL;
	obj = (ACPI_OBJECT *)pbuf->Pointer;
	for (n = 0; (n + 1) < obj->Package.Count; n += 2) {
		uuid = &obj->Package.Elements[n];
		if (uuid->Buffer.Length == ACPI_UUID_LENGTH &&
		    memcmp(uuid->Buffer.Pointer, acpi_dsd_uuid, ACPI_UUID_LENGTH) == 0) {
			props = &obj->Package.Elements[n + 1];
			break;
		}
	}
	if (props == NULL)
		return AE_NOT_FOUND;

	for (n = 0; n < props->Package.Count; n++) {
		pobj = &props->Package.Elements[n];
		if (pobj->Type != ACPI_TYPE_PACKAGE || pobj->Package.Count != 2)
			continue;
		propkey = (ACPI_OBJECT *)&pobj->Package.Elements[0];
		propval = (ACPI_OBJECT *)&pobj->Package.Elements[1];
		if (propkey->Type != ACPI_TYPE_STRING)
			continue;
		if (strcmp(propkey->String.Pointer, prop) != 0)
			continue;

		if (propval->Type != type) {
			return AE_TYPE;
		} else {
			*ret = propval;
			return AE_OK;
		}
		break;
	}

	return AE_NOT_FOUND;
}

ACPI_STATUS
acpi_dsd_integer(ACPI_HANDLE handle, const char *prop, ACPI_INTEGER *val)
{
	ACPI_OBJECT *propval;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_BUFFER;

	rv = acpi_dsd_property(handle, prop, &buf, ACPI_TYPE_INTEGER, &propval);
	if (ACPI_SUCCESS(rv))
		*val = propval->Integer.Value;

	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);
	return rv;
}

ACPI_STATUS
acpi_dsd_string(ACPI_HANDLE handle, const char *prop, char **val)
{
	ACPI_OBJECT *propval;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_BUFFER;

	rv = acpi_dsd_property(handle, prop, &buf, ACPI_TYPE_STRING, &propval);
	if (ACPI_SUCCESS(rv))
		*val = kmem_strdup(propval->String.Pointer, KM_SLEEP);

	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);
	return rv;
}

ACPI_STATUS
acpi_dsd_bool(ACPI_HANDLE handle, const char *prop, bool *val)
{
	ACPI_STATUS rv;
	ACPI_INTEGER ival;

	rv = acpi_dsd_integer(handle, prop, &ival);
	if (ACPI_SUCCESS(rv)) {
		*val = ival != 0;
	}

	return rv;
}


/*
 * Device Specific Method (_DSM) support
 */

ACPI_STATUS
acpi_dsm_typed(ACPI_HANDLE handle, uint8_t *uuid, ACPI_INTEGER rev,
    ACPI_INTEGER func, const ACPI_OBJECT *arg3, ACPI_OBJECT_TYPE return_type,
    ACPI_OBJECT **return_obj)
{
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT obj[4];
	ACPI_BUFFER buf;
	ACPI_STATUS status;

	arg.Count = 4;
	arg.Pointer = obj;

	obj[0].Type = ACPI_TYPE_BUFFER;
	obj[0].Buffer.Length = ACPI_UUID_LENGTH;
	obj[0].Buffer.Pointer = uuid;

	obj[1].Type = ACPI_TYPE_INTEGER;
	obj[1].Integer.Value = rev;

	obj[2].Type = ACPI_TYPE_INTEGER;
	obj[2].Integer.Value = func;

	if (arg3 != NULL) {
		obj[3] = *arg3;
	} else {
		obj[3].Type = ACPI_TYPE_PACKAGE;
		obj[3].Package.Count = 0;
		obj[3].Package.Elements = NULL;
	}

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_BUFFER;

	if (return_obj == NULL && return_type == ACPI_TYPE_ANY) {
		status = AcpiEvaluateObject(handle, "_DSM", &arg, NULL);
	} else {
		*return_obj = NULL;
		status = AcpiEvaluateObjectTyped(handle, "_DSM", &arg, &buf,
		    return_type);
	}
	if (ACPI_FAILURE(status)) {
		return status;
	}
	if (return_obj != NULL) {
		*return_obj = buf.Pointer;
	} else if (buf.Pointer != NULL) {
		ACPI_FREE(buf.Pointer);
	}
	return AE_OK;
}

ACPI_STATUS
acpi_dsm_integer(ACPI_HANDLE handle, uint8_t *uuid, ACPI_INTEGER rev,
    ACPI_INTEGER func, const ACPI_OBJECT *arg3, ACPI_INTEGER *ret)
{
	ACPI_OBJECT *obj;
	ACPI_STATUS status;

	status = acpi_dsm_typed(handle, uuid, rev, func, arg3,
	    ACPI_TYPE_INTEGER, &obj);
	if (ACPI_FAILURE(status)) {
		return status;
	}

	*ret = obj->Integer.Value;
	ACPI_FREE(obj);

	return AE_OK;
}

ACPI_STATUS
acpi_dsm(ACPI_HANDLE handle, uint8_t *uuid, ACPI_INTEGER rev,
    ACPI_INTEGER func, const ACPI_OBJECT *arg3, ACPI_OBJECT **return_obj)
{
	return acpi_dsm_typed(handle, uuid, rev, func, arg3, ACPI_TYPE_ANY,
	    return_obj);
}

ACPI_STATUS
acpi_dsm_query(ACPI_HANDLE handle, uint8_t *uuid, ACPI_INTEGER rev,
    ACPI_INTEGER *ret)
{
	ACPI_OBJECT *obj;
	ACPI_STATUS status;
	uint8_t *data;
	u_int n;

	status = acpi_dsm(handle, uuid, rev, 0, NULL, &obj);
	if (ACPI_FAILURE(status)) {
		return status;
	}

	if (obj->Type == ACPI_TYPE_INTEGER) {
		*ret = obj->Integer.Value;
	} else if (obj->Type == ACPI_TYPE_BUFFER &&
		   obj->Buffer.Length <= 8) {
		*ret = 0;
		data = (uint8_t *)obj->Buffer.Pointer;
		for (n = 0; n < obj->Buffer.Length; n++) {
			*ret |= (uint64_t)data[n] << (n * 8);
		}
	} else {
		status = AE_TYPE;
	}

	ACPI_FREE(obj);

	return status;
}

ACPI_STATUS
acpi_claim_childdevs(device_t dev, struct acpi_devnode *devnode)
{
	struct acpi_devnode *ad;

	SIMPLEQ_FOREACH(ad, &devnode->ad_child_head, ad_child_list) {
		if (ad->ad_device != NULL)
			continue;
		aprint_debug_dev(dev, "claiming %s\n",
		    acpi_name(ad->ad_handle));
		ad->ad_device = dev;
		acpi_claim_childdevs(dev, ad);
	}

	return AE_OK;
}
