/* $NetBSD: version.h,v 1.1.1.1.6.1 2006/04/22 11:38:11 simonb Exp $ */
/******************************************************************************
 * version.h
 * 
 * Xen version, type, and compile information.
 * 
 * Copyright (c) 2005, Nguyen Anh Quynh <aquynh@gmail.com>
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */

#ifndef __XEN_PUBLIC_VERSION_H__
#define __XEN_PUBLIC_VERSION_H__

/* NB. All ops return zero on success, except XENVER_version. */

/* arg == NULL; returns major:minor (16:16). */
#define XENVER_version      0

/* arg == xen_extraversion_t. */
#define XENVER_extraversion 1
typedef char xen_extraversion_t[16];

/* arg == xen_compile_info_t. */
#define XENVER_compile_info 2
typedef struct xen_compile_info {
    char compiler[64];
    char compile_by[16];
    char compile_domain[32];
    char compile_date[32];
} xen_compile_info_t;

#define XENVER_capabilities 3
typedef char xen_capabilities_info_t[1024];

#define XENVER_changeset 4
typedef char xen_changeset_info_t[64];

#define XENVER_platform_parameters 5
typedef struct xen_platform_parameters {
    unsigned long virt_start;
} xen_platform_parameters_t;

#endif /* __XEN_PUBLIC_VERSION_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
