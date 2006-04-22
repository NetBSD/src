/* $NetBSD: dom0_ops.h,v 1.1.1.1.6.1 2006/04/22 11:38:11 simonb Exp $ */
/******************************************************************************
 * dom0_ops.h
 * 
 * Process command requests from domain-0 guest OS.
 * 
 * Copyright (c) 2002-2003, B Dragovic
 * Copyright (c) 2002-2004, K Fraser
 */


#ifndef __XEN_PUBLIC_DOM0_OPS_H__
#define __XEN_PUBLIC_DOM0_OPS_H__

#include "xen.h"
#include "sched_ctl.h"

/*
 * Make sure you increment the interface version whenever you modify this file!
 * This makes sure that old versions of dom0 tools will stop working in a
 * well-defined way (rather than crashing the machine, for instance).
 */
#define DOM0_INTERFACE_VERSION   0x03000000

/************************************************************************/

#define DOM0_GETMEMLIST        2
typedef struct {
    /* IN variables. */
    domid_t       domain;
    unsigned long max_pfns;
    void         *buffer;
    /* OUT variables. */
    unsigned long num_pfns;
} dom0_getmemlist_t;

#define DOM0_SCHEDCTL          6
 /* struct sched_ctl_cmd is from sched-ctl.h   */
typedef struct sched_ctl_cmd dom0_schedctl_t;

#define DOM0_ADJUSTDOM         7
/* struct sched_adjdom_cmd is from sched-ctl.h */
typedef struct sched_adjdom_cmd dom0_adjustdom_t;

#define DOM0_CREATEDOMAIN      8
typedef struct {
    /* IN parameters */
    uint32_t ssidref;
    xen_domain_handle_t handle;
    /* IN/OUT parameters. */
    /* Identifier for new domain (auto-allocate if zero is specified). */
    domid_t domain;
} dom0_createdomain_t;

#define DOM0_DESTROYDOMAIN     9
typedef struct {
    /* IN variables. */
    domid_t domain;
} dom0_destroydomain_t;

#define DOM0_PAUSEDOMAIN      10
typedef struct {
    /* IN parameters. */
    domid_t domain;
} dom0_pausedomain_t;

#define DOM0_UNPAUSEDOMAIN    11
typedef struct {
    /* IN parameters. */
    domid_t domain;
} dom0_unpausedomain_t;

#define DOM0_GETDOMAININFO    12
typedef struct {
    /* IN variables. */
    domid_t  domain;                  /* NB. IN/OUT variable. */
    /* OUT variables. */
#define DOMFLAGS_DYING     (1<<0) /* Domain is scheduled to die.             */
#define DOMFLAGS_SHUTDOWN  (1<<2) /* The guest OS has shut down.             */
#define DOMFLAGS_PAUSED    (1<<3) /* Currently paused by control software.   */
#define DOMFLAGS_BLOCKED   (1<<4) /* Currently blocked pending an event.     */
#define DOMFLAGS_RUNNING   (1<<5) /* Domain is currently running.            */
#define DOMFLAGS_CPUMASK      255 /* CPU to which this domain is bound.      */
#define DOMFLAGS_CPUSHIFT       8
#define DOMFLAGS_SHUTDOWNMASK 255 /* DOMFLAGS_SHUTDOWN guest-supplied code.  */
#define DOMFLAGS_SHUTDOWNSHIFT 16
    uint32_t flags;
    unsigned long tot_pages;
    unsigned long max_pages;
    unsigned long shared_info_frame;       /* MFN of shared_info struct */
    uint64_t cpu_time;
    uint32_t nr_online_vcpus;     /* Number of VCPUs currently online. */
    uint32_t max_vcpu_id;         /* Maximum VCPUID in use by this domain. */
    uint32_t ssidref;
    xen_domain_handle_t handle;
} dom0_getdomaininfo_t;

#define DOM0_SETDOMAININFO      13
typedef struct {
    /* IN variables. */
    domid_t               domain;
    uint32_t              vcpu;
    /* IN/OUT parameters */
    vcpu_guest_context_t *ctxt;
} dom0_setdomaininfo_t;

#define DOM0_MSR              15
typedef struct {
    /* IN variables. */
    uint32_t write;
    cpumap_t cpu_mask;
    uint32_t msr;
    uint32_t in1;
    uint32_t in2;
    /* OUT variables. */
    uint32_t out1;
    uint32_t out2;
} dom0_msr_t;

/*
 * Set clock such that it would read <secs,nsecs> after 00:00:00 UTC,
 * 1 January, 1970 if the current system time was <system_time>.
 */
#define DOM0_SETTIME          17
typedef struct {
    /* IN variables. */
    uint32_t secs;
    uint32_t nsecs;
    uint64_t system_time;
} dom0_settime_t;

#define DOM0_GETPAGEFRAMEINFO 18
#define NOTAB 0         /* normal page */
#define L1TAB (1<<28)
#define L2TAB (2<<28)
#define L3TAB (3<<28)
#define L4TAB (4<<28)
#define LPINTAB  (1<<31)
#define XTAB  (0xf<<28) /* invalid page */
#define LTAB_MASK XTAB
#define LTABTYPE_MASK (0x7<<28)

typedef struct {
    /* IN variables. */
    unsigned long pfn;     /* Machine page frame number to query.       */
    domid_t domain;        /* To which domain does the frame belong?    */
    /* OUT variables. */
    /* Is the page PINNED to a type? */
    uint32_t type;              /* see above type defs */
} dom0_getpageframeinfo_t;

/*
 * Read console content from Xen buffer ring.
 */
#define DOM0_READCONSOLE      19
typedef struct {
    /* IN variables. */
    uint32_t clear;        /* Non-zero -> clear after reading. */
    /* IN/OUT variables. */
    char    *buffer;       /* In: Buffer start; Out: Used buffer start */
    uint32_t count;        /* In: Buffer size;  Out: Used buffer size  */
} dom0_readconsole_t;

/* 
 * Set which physical cpus a vcpu can execute on.
 */
#define DOM0_PINCPUDOMAIN     20
typedef struct {
    /* IN variables. */
    domid_t   domain;
    uint32_t  vcpu;
    cpumap_t  cpumap;
} dom0_pincpudomain_t;

/* Get trace buffers machine base address */
#define DOM0_TBUFCONTROL       21
typedef struct {
    /* IN variables */
#define DOM0_TBUF_GET_INFO     0
#define DOM0_TBUF_SET_CPU_MASK 1
#define DOM0_TBUF_SET_EVT_MASK 2
#define DOM0_TBUF_SET_SIZE     3
#define DOM0_TBUF_ENABLE       4
#define DOM0_TBUF_DISABLE      5
    uint32_t      op;
    /* IN/OUT variables */
    cpumap_t      cpu_mask;
    uint32_t      evt_mask;
    /* OUT variables */
    unsigned long buffer_mfn;
    uint32_t size;
} dom0_tbufcontrol_t;

/*
 * Get physical information about the host machine
 */
#define DOM0_PHYSINFO         22
typedef struct {
    uint32_t threads_per_core;
    uint32_t cores_per_socket;
    uint32_t sockets_per_node;
    uint32_t nr_nodes;
    uint32_t cpu_khz;
    unsigned long total_pages;
    unsigned long free_pages;
    uint32_t hw_cap[8];
} dom0_physinfo_t;

/*
 * Get the ID of the current scheduler.
 */
#define DOM0_SCHED_ID        24
typedef struct {
    /* OUT variable */
    uint32_t sched_id;
} dom0_sched_id_t;

/* 
 * Control shadow pagetables operation
 */
#define DOM0_SHADOW_CONTROL  25

#define DOM0_SHADOW_CONTROL_OP_OFF         0
#define DOM0_SHADOW_CONTROL_OP_ENABLE_TEST 1
#define DOM0_SHADOW_CONTROL_OP_ENABLE_LOGDIRTY 2
#define DOM0_SHADOW_CONTROL_OP_ENABLE_TRANSLATE 3

#define DOM0_SHADOW_CONTROL_OP_FLUSH       10     /* table ops */
#define DOM0_SHADOW_CONTROL_OP_CLEAN       11
#define DOM0_SHADOW_CONTROL_OP_PEEK        12

typedef struct dom0_shadow_control
{
    uint32_t fault_count;
    uint32_t dirty_count;
    uint32_t dirty_net_count;     
    uint32_t dirty_block_count;     
} dom0_shadow_control_stats_t;

typedef struct {
    /* IN variables. */
    domid_t        domain;
    uint32_t       op;
    unsigned long *dirty_bitmap; /* pointer to locked buffer */
    /* IN/OUT variables. */
    unsigned long  pages;        /* size of buffer, updated with actual size */
    /* OUT variables. */
    dom0_shadow_control_stats_t stats;
} dom0_shadow_control_t;

#define DOM0_SETDOMAINMAXMEM   28
typedef struct {
    /* IN variables. */
    domid_t       domain;
    unsigned long max_memkb;
} dom0_setdomainmaxmem_t;

#define DOM0_GETPAGEFRAMEINFO2 29   /* batched interface */
typedef struct {
    /* IN variables. */
    domid_t        domain;
    unsigned long  num;
    /* IN/OUT variables. */
    unsigned long *array;
} dom0_getpageframeinfo2_t;

/*
 * Request memory range (@pfn, @pfn+@nr_pfns-1) to have type @type.
 * On x86, @type is an architecture-defined MTRR memory type.
 * On success, returns the MTRR that was used (@reg) and a handle that can
 * be passed to DOM0_DEL_MEMTYPE to accurately tear down the new setting.
 * (x86-specific).
 */
#define DOM0_ADD_MEMTYPE         31
typedef struct {
    /* IN variables. */
    unsigned long pfn;
    unsigned long nr_pfns;
    uint32_t      type;
    /* OUT variables. */
    uint32_t      handle;
    uint32_t      reg;
} dom0_add_memtype_t;

/*
 * Tear down an existing memory-range type. If @handle is remembered then it
 * should be passed in to accurately tear down the correct setting (in case
 * of overlapping memory regions with differing types). If it is not known
 * then @handle should be set to zero. In all cases @reg must be set.
 * (x86-specific).
 */
#define DOM0_DEL_MEMTYPE         32
typedef struct {
    /* IN variables. */
    uint32_t handle;
    uint32_t reg;
} dom0_del_memtype_t;

/* Read current type of an MTRR (x86-specific). */
#define DOM0_READ_MEMTYPE        33
typedef struct {
    /* IN variables. */
    uint32_t reg;
    /* OUT variables. */
    unsigned long pfn;
    unsigned long nr_pfns;
    uint32_t type;
} dom0_read_memtype_t;

/* Interface for controlling Xen software performance counters. */
#define DOM0_PERFCCONTROL        34
/* Sub-operations: */
#define DOM0_PERFCCONTROL_OP_RESET 1   /* Reset all counters to zero. */
#define DOM0_PERFCCONTROL_OP_QUERY 2   /* Get perfctr information. */
typedef struct {
    uint8_t      name[80];             /* name of perf counter */
    uint32_t     nr_vals;              /* number of values for this counter */
    uint32_t     vals[64];             /* array of values */
} dom0_perfc_desc_t;
typedef struct {
    /* IN variables. */
    uint32_t       op;                /*  DOM0_PERFCCONTROL_OP_??? */
    /* OUT variables. */
    uint32_t       nr_counters;       /*  number of counters */
    dom0_perfc_desc_t *desc;          /*  counter information (or NULL) */
} dom0_perfccontrol_t;

#define DOM0_MICROCODE           35
typedef struct {
    /* IN variables. */
    void    *data;                    /* Pointer to microcode data */
    uint32_t length;                  /* Length of microcode data. */
} dom0_microcode_t;

#define DOM0_IOPORT_PERMISSION   36
typedef struct {
    domid_t  domain;                  /* domain to be affected */
    uint32_t first_port;              /* first port int range */
    uint32_t nr_ports;                /* size of port range */
    uint8_t  allow_access;            /* allow or deny access to range? */
} dom0_ioport_permission_t;

#define DOM0_GETVCPUCONTEXT      37
typedef struct {
    /* IN variables. */
    domid_t  domain;                  /* domain to be affected */
    uint32_t vcpu;                    /* vcpu # */
    /* OUT variables. */
    vcpu_guest_context_t *ctxt;
} dom0_getvcpucontext_t;

#define DOM0_GETVCPUINFO         43
typedef struct {
    /* IN variables. */
    domid_t  domain;                  /* domain to be affected */
    uint32_t vcpu;                    /* vcpu # */
    /* OUT variables. */
    uint8_t  online;                  /* currently online (not hotplugged)? */
    uint8_t  blocked;                 /* blocked waiting for an event? */
    uint8_t  running;                 /* currently scheduled on its CPU? */
    uint64_t cpu_time;                /* total cpu time consumed (ns) */
    uint32_t cpu;                     /* current mapping   */
    cpumap_t cpumap;                  /* allowable mapping */
} dom0_getvcpuinfo_t;

#define DOM0_GETDOMAININFOLIST   38
typedef struct {
    /* IN variables. */
    domid_t               first_domain;
    uint32_t              max_domains;
    dom0_getdomaininfo_t *buffer;
    /* OUT variables. */
    uint32_t              num_domains;
} dom0_getdomaininfolist_t;

#define DOM0_PLATFORM_QUIRK      39  
#define QUIRK_NOIRQBALANCING  1
typedef struct {
    /* IN variables. */
    uint32_t quirk_id;
} dom0_platform_quirk_t;

#define DOM0_PHYSICAL_MEMORY_MAP 40
typedef struct {
    /* IN variables. */
    uint32_t max_map_entries;
    /* OUT variables. */
    uint32_t nr_map_entries;
    struct dom0_memory_map_entry {
        uint64_t start, end;
        uint32_t flags; /* reserved */
        uint8_t  is_ram;
    } *memory_map;
} dom0_physical_memory_map_t;

#define DOM0_MAX_VCPUS 41
typedef struct {
    domid_t  domain;        /* domain to be affected */
    uint32_t max;           /* maximum number of vcpus */
} dom0_max_vcpus_t;

#define DOM0_SETDOMAINHANDLE 44
typedef struct {
    domid_t domain;
    xen_domain_handle_t handle;
} dom0_setdomainhandle_t;

typedef struct {
    uint32_t cmd;
    uint32_t interface_version; /* DOM0_INTERFACE_VERSION */
    union {
        dom0_createdomain_t      createdomain;
        dom0_pausedomain_t       pausedomain;
        dom0_unpausedomain_t     unpausedomain;
        dom0_destroydomain_t     destroydomain;
        dom0_getmemlist_t        getmemlist;
        dom0_schedctl_t          schedctl;
        dom0_adjustdom_t         adjustdom;
        dom0_setdomaininfo_t     setdomaininfo;
        dom0_getdomaininfo_t     getdomaininfo;
        dom0_getpageframeinfo_t  getpageframeinfo;
        dom0_msr_t               msr;
        dom0_settime_t           settime;
        dom0_readconsole_t       readconsole;
        dom0_pincpudomain_t      pincpudomain;
        dom0_tbufcontrol_t       tbufcontrol;
        dom0_physinfo_t          physinfo;
        dom0_sched_id_t          sched_id;
        dom0_shadow_control_t    shadow_control;
        dom0_setdomainmaxmem_t   setdomainmaxmem;
        dom0_getpageframeinfo2_t getpageframeinfo2;
        dom0_add_memtype_t       add_memtype;
        dom0_del_memtype_t       del_memtype;
        dom0_read_memtype_t      read_memtype;
        dom0_perfccontrol_t      perfccontrol;
        dom0_microcode_t         microcode;
        dom0_ioport_permission_t ioport_permission;
        dom0_getvcpucontext_t    getvcpucontext;
        dom0_getvcpuinfo_t       getvcpuinfo;
        dom0_getdomaininfolist_t getdomaininfolist;
        dom0_platform_quirk_t    platform_quirk;
        dom0_physical_memory_map_t physical_memory_map;
        dom0_max_vcpus_t         max_vcpus;
        dom0_setdomainhandle_t   setdomainhandle;
        uint8_t                  pad[128];
    } u;
} dom0_op_t;

#endif /* __XEN_PUBLIC_DOM0_OPS_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
