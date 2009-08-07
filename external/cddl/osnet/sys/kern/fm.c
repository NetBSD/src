/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Fault Management Architecture (FMA) Resource and Protocol Support
 *
 * The routines contained herein provide services to support kernel subsystems
 * in publishing fault management telemetry (see PSARC 2002/412 and 2003/089).
 *
 * Name-Value Pair Lists
 *
 * The embodiment of an FMA protocol element (event, fmri or authority) is a
 * name-value pair list (nvlist_t).  FMA-specific nvlist construtor and
 * destructor functions, fm_nvlist_create() and fm_nvlist_destroy(), are used
 * to create an nvpair list using custom allocators.  Callers may choose to
 * allocate either from the kernel memory allocator, or from a preallocated
 * buffer, useful in constrained contexts like high-level interrupt routines.
 *
 * Protocol Event and FMRI Construction
 *
 * Convenience routines are provided to construct nvlist events according to
 * the FMA Event Protocol and Naming Schema specification for ereports and
 * FMRIs for the dev, cpu, hc, mem, legacy hc and de schemes.
 *
 * ENA Manipulation
 *
 * Routines to generate ENA formats 0, 1 and 2 are available as well as
 * routines to increment formats 1 and 2.  Individual fields within the
 * ENA are extractable via fm_ena_time_get(), fm_ena_id_get(),
 * fm_ena_format_get() and fm_ena_gen_get().
 */

#include <sys/types.h>
#include <sys/pset.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/sysevent.h>
#include <sys/nvpair.h>
#include <sys/cmn_err.h>
#include <sys/cpuvar.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/systeminfo.h>
#include <sys/sysevent/eventdefs.h>
#include <sys/fm/util.h>
#include <sys/fm/protocol.h>

/*
 * URL and SUNW-MSG-ID value to display for fm_panic(), defined below.  These
 * values must be kept in sync with the FMA source code in usr/src/cmd/fm.
 */
static const char *fm_url = "http://www.sun.com/msg";
static const char *fm_msgid = "SUNOS-8000-0G";
static char *volatile fm_panicstr = NULL;

errorq_t *ereport_errorq;

static uint_t ereport_chanlen = ERPT_EVCH_MAX;
static evchan_t *ereport_chan = NULL;
static ulong_t ereport_qlen = 0;
static size_t ereport_size = 0;
static int ereport_cols = 80;

/*
 * Formatting utility function for fm_nvprintr.  We attempt to wrap chunks of
 * output so they aren't split across console lines, and return the end column.
 */
/*PRINTFLIKE4*/
static int
fm_printf(int depth, int c, int cols, const char *format, ...)
{
	va_list ap;
	int width;
	char c1;
	return 0;
	va_start(ap, format);
	width = vsnprintf(&c1, sizeof (c1), format, ap);
	va_end(ap);

	if (c + width >= cols) {
		printf("\n\r");
		c = 0;
		if (format[0] != ' ' && depth > 0) {
			printf(" ");
			c++;
		}
	}

	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	return ((c + width) % cols);
}

/*
 * Recursively print a nvlist in the specified column width and return the
 * column we end up in.  This function is called recursively by fm_nvprint(),
 * below.  We generically format the entire nvpair using hexadecimal
 * integers and strings, and elide any integer arrays.  Arrays are basically
 * used for cache dumps right now, so we suppress them so as not to overwhelm
 * the amount of console output we produce at panic time.  This can be further
 * enhanced as FMA technology grows based upon the needs of consumers.  All
 * FMA telemetry is logged using the dump device transport, so the console
 * output serves only as a fallback in case this procedure is unsuccessful.
 */
static int
fm_nvprintr(nvlist_t *nvl, int d, int c, int cols)
{
	nvpair_t *nvp;

	for (nvp = nvlist_next_nvpair(nvl, NULL);
	    nvp != NULL; nvp = nvlist_next_nvpair(nvl, nvp)) {

		data_type_t type = nvpair_type(nvp);
		const char *name = nvpair_name(nvp);

		boolean_t b;
		uint8_t i8;
		uint16_t i16;
		uint32_t i32;
		uint64_t i64;
		char *str;
		nvlist_t *cnv;

		if (strcmp(name, FM_CLASS) == 0)
			continue; /* already printed by caller */

		c = fm_printf(d, c, cols, " %s=", name);

		switch (type) {
		case DATA_TYPE_BOOLEAN:
			c = fm_printf(d + 1, c, cols, " 1");
			break;

		case DATA_TYPE_BOOLEAN_VALUE:
			(void) nvpair_value_boolean_value(nvp, &b);
			c = fm_printf(d + 1, c, cols, b ? "1" : "0");
			break;

		case DATA_TYPE_BYTE:
			(void) nvpair_value_byte(nvp, &i8);
			c = fm_printf(d + 1, c, cols, "%x", i8);
			break;

		case DATA_TYPE_INT8:
			(void) nvpair_value_int8(nvp, (void *)&i8);
			c = fm_printf(d + 1, c, cols, "%x", i8);
			break;

		case DATA_TYPE_UINT8:
			(void) nvpair_value_uint8(nvp, &i8);
			c = fm_printf(d + 1, c, cols, "%x", i8);
			break;

		case DATA_TYPE_INT16:
			(void) nvpair_value_int16(nvp, (void *)&i16);
			c = fm_printf(d + 1, c, cols, "%x", i16);
			break;

		case DATA_TYPE_UINT16:
			(void) nvpair_value_uint16(nvp, &i16);
			c = fm_printf(d + 1, c, cols, "%x", i16);
			break;

		case DATA_TYPE_INT32:
			(void) nvpair_value_int32(nvp, (void *)&i32);
			c = fm_printf(d + 1, c, cols, "%x", i32);
			break;

		case DATA_TYPE_UINT32:
			(void) nvpair_value_uint32(nvp, &i32);
			c = fm_printf(d + 1, c, cols, "%x", i32);
			break;

		case DATA_TYPE_INT64:
			(void) nvpair_value_int64(nvp, (void *)&i64);
			c = fm_printf(d + 1, c, cols, "%llx",
			    (u_longlong_t)i64);
			break;

		case DATA_TYPE_UINT64:
			(void) nvpair_value_uint64(nvp, &i64);
			c = fm_printf(d + 1, c, cols, "%llx",
			    (u_longlong_t)i64);
			break;

		case DATA_TYPE_HRTIME:
			(void) nvpair_value_hrtime(nvp, (void *)&i64);
			c = fm_printf(d + 1, c, cols, "%llx",
			    (u_longlong_t)i64);
			break;

		case DATA_TYPE_STRING:
			(void) nvpair_value_string(nvp, &str);
			c = fm_printf(d + 1, c, cols, "\"%s\"",
			    str ? str : "<NULL>");
			break;

		case DATA_TYPE_NVLIST:
			c = fm_printf(d + 1, c, cols, "[");
			(void) nvpair_value_nvlist(nvp, &cnv);
			c = fm_nvprintr(cnv, d + 1, c, cols);
			c = fm_printf(d + 1, c, cols, " ]");
			break;

		case DATA_TYPE_NVLIST_ARRAY: {
			nvlist_t **val;
			uint_t i, nelem;

			c = fm_printf(d + 1, c, cols, "[");
			(void) nvpair_value_nvlist_array(nvp, &val, &nelem);
			for (i = 0; i < nelem; i++) {
				c = fm_nvprintr(val[i], d + 1, c, cols);
			}
			c = fm_printf(d + 1, c, cols, " ]");
			}
			break;

		case DATA_TYPE_BOOLEAN_ARRAY:
		case DATA_TYPE_BYTE_ARRAY:
		case DATA_TYPE_INT8_ARRAY:
		case DATA_TYPE_UINT8_ARRAY:
		case DATA_TYPE_INT16_ARRAY:
		case DATA_TYPE_UINT16_ARRAY:
		case DATA_TYPE_INT32_ARRAY:
		case DATA_TYPE_UINT32_ARRAY:
		case DATA_TYPE_INT64_ARRAY:
		case DATA_TYPE_UINT64_ARRAY:
		case DATA_TYPE_STRING_ARRAY:
			c = fm_printf(d + 1, c, cols, "[...]");
			break;
		case DATA_TYPE_UNKNOWN:
			c = fm_printf(d + 1, c, cols, "<unknown>");
			break;
		}
	}

	return (c);
}

void
fm_nvprint(nvlist_t *nvl)
{
	char *class;
	int c = 0;

	printf("\r");

	if (nvlist_lookup_string(nvl, FM_CLASS, &class) == 0)
		c = fm_printf(0, c, ereport_cols, "%s", class);

	if (fm_nvprintr(nvl, 0, c, ereport_cols) != 0)
		printf("\n");

	printf("\n");
}

/*
 * Wrapper for panic() that first produces an FMA-style message for admins.
 * Normally such messages are generated by fmd(1M)'s syslog-msgs agent: this
 * is the one exception to that rule and the only error that gets messaged.
 * This function is intended for use by subsystems that have detected a fatal
 * error and enqueued appropriate ereports and wish to then force a panic.
 */
/*PRINTFLIKE1*/
void
fm_panic(const char *format, ...)
{
	va_list ap;

	(void) atomic_cas_ptr((void *)&fm_panicstr, NULL, (void *)format);
	va_start(ap, format);
	vcmn_err(CE_PANIC, format, ap);
	va_end(ap);
}

/*
 * Print any appropriate FMA banner message before the panic message.  This
 * function is called by panicsys() and prints the message for fm_panic().
 * We print the message here so that it comes after the system is quiesced.
 * A one-line summary is recorded in the log only (cmn_err(9F) with "!" prefix).
 * The rest of the message is for the console only and not needed in the log,
 * so it is printed using printf().  We break it up into multiple
 * chunks so as to avoid overflowing any small legacy prom_printf() buffers.
 */
void
fm_banner(void)
{
	struct timespec tod;
	hrtime_t now;

	if (!fm_panicstr)
		return; /* panic was not initiated by fm_panic(); do nothing */

	getnanotime(&tod);
	now = hardclock_ticks;

	cmn_err(CE_NOTE, "!SUNW-MSG-ID: %s, "
	    "TYPE: Error, VER: 1, SEVERITY: Major\n", fm_msgid);

	printf(
"\n\rSUNW-MSG-ID: %s, TYPE: Error, VER: 1, SEVERITY: Major\n"
"EVENT-TIME: 0x%lx.0x%lx (0x%llx)\n",
	    fm_msgid, tod.tv_sec, tod.tv_nsec, (u_longlong_t)now);

	printf(
"PLATFORM: %s, CSN: -, HOSTNAME: %s\n"
"SOURCE: %s, REV: %s\n",
	    machine, hostname, "NetBSD",
	    osrelease);

	printf(
"DESC: Errors have been detected that require a reboot to ensure system\n"
"integrity.  See %s/%s for more information.\n",
	    fm_url, fm_msgid);

	printf(
"AUTO-RESPONSE: NetBSD will not attempt to save and diagnose the error telemetry\n"
"IMPACT: The system will sync files, save a crash dump if needed, and reboot\n"
"REC-ACTION: Save the error summary below\n");

	printf("\n");
}

/*
 * Post an error report (ereport) to the sysevent error channel.  The error
 * channel must be established with a prior call to sysevent_evc_create()
 * before publication may occur.
 */
void
fm_ereport_post(nvlist_t *ereport, int evc_flag)
{
	size_t nvl_size = 0;
	evchan_t *error_chan;

	(void) nvlist_size(ereport, &nvl_size, NV_ENCODE_NATIVE);
	if (nvl_size > ERPT_DATA_SZ || nvl_size == 0) {
		printf("fm_ereport_post: dropped report\n");
		return;
	}

	fm_banner();
	fm_nvprint(ereport);

}

/*
 * Wrapppers for FM nvlist allocators
 */
/* ARGSUSED */
static void *
i_fm_alloc(nv_alloc_t *nva, size_t size)
{
	return (kmem_zalloc(size, KM_SLEEP));
}

/* ARGSUSED */
static void
i_fm_free(nv_alloc_t *nva, void *buf, size_t size)
{
	kmem_free(buf, size);
}

const nv_alloc_ops_t fm_mem_alloc_ops = {
	NULL,
	NULL,
	i_fm_alloc,
	i_fm_free,
	NULL
};

/*
 * Create and initialize a new nv_alloc_t for a fixed buffer, buf.  A pointer
 * to the newly allocated nv_alloc_t structure is returned upon success or NULL
 * is returned to indicate that the nv_alloc structure could not be created.
 */
nv_alloc_t *
fm_nva_xcreate(char *buf, size_t bufsz)
{
	nv_alloc_t *nvhdl = kmem_zalloc(sizeof (nv_alloc_t), KM_SLEEP);

	if (bufsz == 0 || nv_alloc_init(nvhdl, nv_fixed_ops, buf, bufsz) != 0) {
		kmem_free(nvhdl, sizeof (nv_alloc_t));
		return (NULL);
	}

	return (nvhdl);
}

/*
 * Destroy a previously allocated nv_alloc structure.  The fixed buffer
 * associated with nva must be freed by the caller.
 */
void
fm_nva_xdestroy(nv_alloc_t *nva)
{
	nv_alloc_fini(nva);
	kmem_free(nva, sizeof (nv_alloc_t));
}

/*
 * Create a new nv list.  A pointer to a new nv list structure is returned
 * upon success or NULL is returned to indicate that the structure could
 * not be created.  The newly created nv list is created and managed by the
 * operations installed in nva.   If nva is NULL, the default FMA nva
 * operations are installed and used.
 *
 * When called from the kernel and nva == NULL, this function must be called
 * from passive kernel context with no locks held that can prevent a
 * sleeping memory allocation from occurring.  Otherwise, this function may
 * be called from other kernel contexts as long a valid nva created via
 * fm_nva_create() is supplied.
 */
nvlist_t *
fm_nvlist_create(nv_alloc_t *nva)
{
	int hdl_alloced = 0;
	nvlist_t *nvl;
	nv_alloc_t *nvhdl;

	if (nva == NULL) {
		nvhdl = kmem_zalloc(sizeof (nv_alloc_t), KM_SLEEP);

		if (nv_alloc_init(nvhdl, &fm_mem_alloc_ops, NULL, 0) != 0) {
			kmem_free(nvhdl, sizeof (nv_alloc_t));
			return (NULL);
		}
		hdl_alloced = 1;
	} else {
		nvhdl = nva;
	}

	if (nvlist_xalloc(&nvl, NV_UNIQUE_NAME, nvhdl) != 0) {
		if (hdl_alloced) {
			kmem_free(nvhdl, sizeof (nv_alloc_t));
			nv_alloc_fini(nvhdl);
		}
		return (NULL);
	}

	return (nvl);
}

/*
 * Destroy a previously allocated nvlist structure.  flag indicates whether
 * or not the associated nva structure should be freed (FM_NVA_FREE) or
 * retained (FM_NVA_RETAIN).  Retaining the nv alloc structure allows
 * it to be re-used for future nvlist creation operations.
 */
void
fm_nvlist_destroy(nvlist_t *nvl, int flag)
{
	nv_alloc_t *nva = nvlist_lookup_nv_alloc(nvl);

	nvlist_free(nvl);

	if (nva != NULL) {
		if (flag == FM_NVA_FREE)
			fm_nva_xdestroy(nva);
	}
}

int
i_fm_payload_set(nvlist_t *payload, const char *name, va_list ap)
{
	int nelem, ret = 0;
	data_type_t type;

	while (ret == 0 && name != NULL) {
		type = va_arg(ap, data_type_t);
		switch (type) {
		case DATA_TYPE_BYTE:
			ret = nvlist_add_byte(payload, name,
			    va_arg(ap, uint_t));
			break;
		case DATA_TYPE_BYTE_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_byte_array(payload, name,
			    va_arg(ap, uchar_t *), nelem);
			break;
		case DATA_TYPE_BOOLEAN_VALUE:
			ret = nvlist_add_boolean_value(payload, name,
			    va_arg(ap, boolean_t));
			break;
		case DATA_TYPE_BOOLEAN_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_boolean_array(payload, name,
			    va_arg(ap, boolean_t *), nelem);
			break;
		case DATA_TYPE_INT8:
			ret = nvlist_add_int8(payload, name,
			    va_arg(ap, int));
			break;
		case DATA_TYPE_INT8_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_int8_array(payload, name,
			    va_arg(ap, int8_t *), nelem);
			break;
		case DATA_TYPE_UINT8:
			ret = nvlist_add_uint8(payload, name,
			    va_arg(ap, uint_t));
			break;
		case DATA_TYPE_UINT8_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_uint8_array(payload, name,
			    va_arg(ap, uint8_t *), nelem);
			break;
		case DATA_TYPE_INT16:
			ret = nvlist_add_int16(payload, name,
			    va_arg(ap, int));
			break;
		case DATA_TYPE_INT16_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_int16_array(payload, name,
			    va_arg(ap, int16_t *), nelem);
			break;
		case DATA_TYPE_UINT16:
			ret = nvlist_add_uint16(payload, name,
			    va_arg(ap, uint_t));
			break;
		case DATA_TYPE_UINT16_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_uint16_array(payload, name,
			    va_arg(ap, uint16_t *), nelem);
			break;
		case DATA_TYPE_INT32:
			ret = nvlist_add_int32(payload, name,
			    va_arg(ap, int32_t));
			break;
		case DATA_TYPE_INT32_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_int32_array(payload, name,
			    va_arg(ap, int32_t *), nelem);
			break;
		case DATA_TYPE_UINT32:
			ret = nvlist_add_uint32(payload, name,
			    va_arg(ap, uint32_t));
			break;
		case DATA_TYPE_UINT32_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_uint32_array(payload, name,
			    va_arg(ap, uint32_t *), nelem);
			break;
		case DATA_TYPE_INT64:
			ret = nvlist_add_int64(payload, name,
			    va_arg(ap, int64_t));
			break;
		case DATA_TYPE_INT64_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_int64_array(payload, name,
			    va_arg(ap, int64_t *), nelem);
			break;
		case DATA_TYPE_UINT64:
			ret = nvlist_add_uint64(payload, name,
			    va_arg(ap, uint64_t));
			break;
		case DATA_TYPE_UINT64_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_uint64_array(payload, name,
			    va_arg(ap, uint64_t *), nelem);
			break;
		case DATA_TYPE_STRING:
			ret = nvlist_add_string(payload, name,
			    va_arg(ap, char *));
			break;
		case DATA_TYPE_STRING_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_string_array(payload, name,
			    va_arg(ap, char **), nelem);
			break;
		case DATA_TYPE_NVLIST:
			ret = nvlist_add_nvlist(payload, name,
			    va_arg(ap, nvlist_t *));
			break;
		case DATA_TYPE_NVLIST_ARRAY:
			nelem = va_arg(ap, int);
			ret = nvlist_add_nvlist_array(payload, name,
			    va_arg(ap, nvlist_t **), nelem);
			break;
		default:
			ret = EINVAL;
		}

		name = va_arg(ap, char *);
	}
	return (ret);
}

void
fm_payload_set(nvlist_t *payload, ...)
{
	int ret;
	const char *name;
	va_list ap;

	va_start(ap, payload);
	name = va_arg(ap, char *);
	ret = i_fm_payload_set(payload, name, ap);
	va_end(ap);

	if (ret)
		printf("fm_payload_set: failed\n");
}

/*
 * Set-up and validate the members of an ereport event according to:
 *
 *	Member name		Type		Value
 *	====================================================
 *	class			string		ereport
 *	version			uint8_t		0
 *	ena			uint64_t	<ena>
 *	detector		nvlist_t	<detector>
 *	ereport-payload		nvlist_t	<var args>
 *
 */
void
fm_ereport_set(nvlist_t *ereport, int version, const char *erpt_class,
    uint64_t ena, const nvlist_t *detector, ...)
{
	char ereport_class[FM_MAX_CLASS];
	const char *name;
	va_list ap;
	int ret;

	if (version != FM_EREPORT_VERS0) {
		printf("fm_payload_set: bad version\n");
		return;
	}

	(void) snprintf(ereport_class, FM_MAX_CLASS, "%s.%s",
	    FM_EREPORT_CLASS, erpt_class);
	if (nvlist_add_string(ereport, FM_CLASS, ereport_class) != 0) {
		printf("fm_payload_set: can't add\n");
		return;
	}

	if (nvlist_add_uint64(ereport, FM_EREPORT_ENA, ena)) {
		printf("fm_payload_set: can't add\n");
	}

	if (nvlist_add_nvlist(ereport, FM_EREPORT_DETECTOR,
	    (nvlist_t *)detector) != 0) {
		printf("fm_payload_set: can't add\n");
	}

	va_start(ap, detector);
	name = va_arg(ap, const char *);
	ret = i_fm_payload_set(ereport, name, ap);
	va_end(ap);

	if (ret)
		printf("fm_payload_set: can't add\n");
}

void
fm_fmri_zfs_set(nvlist_t *fmri, int version, uint64_t pool_guid,
    uint64_t vdev_guid)
{
	if (version != ZFS_SCHEME_VERSION0) {
		printf("fm_fmri_zfs_set: bad version\n");
		return;
	}

	if (nvlist_add_uint8(fmri, FM_VERSION, version) != 0) {
		printf("fm_fmri_zfs_set: can't set\n");
		return;
	}

	if (nvlist_add_string(fmri, FM_FMRI_SCHEME, FM_FMRI_SCHEME_ZFS) != 0) {
		printf("fm_fmri_zfs_set: can't set\n");
		return;
	}

	if (nvlist_add_uint64(fmri, FM_FMRI_ZFS_POOL, pool_guid) != 0) {
		printf("fm_fmri_zfs_set: can't set\n");
	}

	if (vdev_guid != 0) {
		if (nvlist_add_uint64(fmri, FM_FMRI_ZFS_VDEV, vdev_guid) != 0) {
			printf("fm_fmri_zfs_set: can't set\n");
		}
	}
}

uint64_t
fm_ena_increment(uint64_t ena)
{
	uint64_t new_ena;

	switch (ENA_FORMAT(ena)) {
	case FM_ENA_FMT1:
		new_ena = ena + (1 << ENA_FMT1_GEN_SHFT);
		break;
	case FM_ENA_FMT2:
		new_ena = ena + (1 << ENA_FMT2_GEN_SHFT);
		break;
	default:
		new_ena = 0;
	}

	return (new_ena);
}

uint64_t
fm_ena_generate_cpu(uint64_t timestamp, processorid_t cpuid, uchar_t format)
{
	uint64_t ena = 0;

	switch (format) {
	case FM_ENA_FMT1:
		if (timestamp) {
			ena = (uint64_t)((format & ENA_FORMAT_MASK) |
			    ((cpuid << ENA_FMT1_CPUID_SHFT) &
			    ENA_FMT1_CPUID_MASK) |
			    ((timestamp << ENA_FMT1_TIME_SHFT) &
			    ENA_FMT1_TIME_MASK));
		} else {
			ena = (uint64_t)((format & ENA_FORMAT_MASK) |
			    ((cpuid << ENA_FMT1_CPUID_SHFT) &
			    ENA_FMT1_CPUID_MASK) |
			    ((hardclock_ticks << ENA_FMT1_TIME_SHFT) &
			    ENA_FMT1_TIME_MASK));
		}
		break;
	case FM_ENA_FMT2:
		ena = (uint64_t)((format & ENA_FORMAT_MASK) |
		    ((timestamp << ENA_FMT2_TIME_SHFT) & ENA_FMT2_TIME_MASK));
		break;
	default:
		break;
	}

	return (ena);
}

uint64_t
fm_ena_generate(uint64_t timestamp, uchar_t format)
{
	return (fm_ena_generate_cpu(timestamp, cpu_index(curcpu()), format));
}

uint64_t
fm_ena_generation_get(uint64_t ena)
{
	uint64_t gen;

	switch (ENA_FORMAT(ena)) {
	case FM_ENA_FMT1:
		gen = (ena & ENA_FMT1_GEN_MASK) >> ENA_FMT1_GEN_SHFT;
		break;
	case FM_ENA_FMT2:
		gen = (ena & ENA_FMT2_GEN_MASK) >> ENA_FMT2_GEN_SHFT;
		break;
	default:
		gen = 0;
		break;
	}

	return (gen);
}

uchar_t
fm_ena_format_get(uint64_t ena)
{

	return (ENA_FORMAT(ena));
}

uint64_t
fm_ena_id_get(uint64_t ena)
{
	uint64_t id;

	switch (ENA_FORMAT(ena)) {
	case FM_ENA_FMT1:
		id = (ena & ENA_FMT1_ID_MASK) >> ENA_FMT1_ID_SHFT;
		break;
	case FM_ENA_FMT2:
		id = (ena & ENA_FMT2_ID_MASK) >> ENA_FMT2_ID_SHFT;
		break;
	default:
		id = 0;
	}

	return (id);
}

uint64_t
fm_ena_time_get(uint64_t ena)
{
	uint64_t time;

	switch (ENA_FORMAT(ena)) {
	case FM_ENA_FMT1:
		time = (ena & ENA_FMT1_TIME_MASK) >> ENA_FMT1_TIME_SHFT;
		break;
	case FM_ENA_FMT2:
		time = (ena & ENA_FMT2_TIME_MASK) >> ENA_FMT2_TIME_SHFT;
		break;
	default:
		time = 0;
	}

	return (time);
}
