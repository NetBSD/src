/*
 * acm_ops.h: Xen access control module hypervisor commands
 *
 * Reiner Sailer <sailer@watson.ibm.com>
 * Copyright (c) 2005, International Business Machines Corporation.
 */

#ifndef __XEN_PUBLIC_ACM_OPS_H__
#define __XEN_PUBLIC_ACM_OPS_H__

#include "xen.h"
#include "sched_ctl.h"
#include "acm.h"

/*
 * Make sure you increment the interface version whenever you modify this file!
 * This makes sure that old versions of acm tools will stop working in a
 * well-defined way (rather than crashing the machine, for instance).
 */
#define ACM_INTERFACE_VERSION   0xAAAA0005

/************************************************************************/

#define ACM_SETPOLICY         4
struct acm_setpolicy {
    /* OUT variables */
    void *pushcache;
    uint32_t pushcache_size;
};


#define ACM_GETPOLICY         5
struct acm_getpolicy {
    /* OUT variables */
    void *pullcache;
    uint32_t pullcache_size;
};


#define ACM_DUMPSTATS         6
struct acm_dumpstats {
    void *pullcache;
    uint32_t pullcache_size;
};


#define ACM_GETSSID           7
enum get_type {UNSET=0, SSIDREF, DOMAINID};
struct acm_getssid {
    enum get_type get_ssid_by;
    union {
        domaintype_t domainid;
        ssidref_t    ssidref;
    } id;
    void *ssidbuf;
    uint32_t ssidbuf_size;
};

#define ACM_GETDECISION        8
struct acm_getdecision {
    enum get_type get_decision_by1; /* in */
    enum get_type get_decision_by2;
    union {
        domaintype_t domainid;
        ssidref_t    ssidref;
    } id1;
    union {
        domaintype_t domainid;
        ssidref_t    ssidref;
    } id2;
    enum acm_hook_type hook;
    int acm_decision;           /* out */
};

typedef struct acm_op {
    uint32_t cmd;
    uint32_t interface_version;      /* ACM_INTERFACE_VERSION */
    union {
        struct acm_setpolicy setpolicy;
        struct acm_getpolicy getpolicy;
        struct acm_dumpstats dumpstats;
        struct acm_getssid getssid;
        struct acm_getdecision getdecision;
    } u;
} acm_op_t;
DEFINE_GUEST_HANDLE(acm_op_t);

#endif                          /* __XEN_PUBLIC_ACM_OPS_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
