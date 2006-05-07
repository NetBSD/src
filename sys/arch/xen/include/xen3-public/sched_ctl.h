/* $NetBSD: sched_ctl.h,v 1.3 2006/05/07 10:56:37 bouyer Exp $ */
/******************************************************************************
 * Generic scheduler control interface.
 *
 * Mark Williamson, (C) 2004 Intel Research Cambridge
 */

#ifndef __XEN_PUBLIC_SCHED_CTL_H__
#define __XEN_PUBLIC_SCHED_CTL_H__

/* Scheduler types. */
#define SCHED_BVT      0
#define SCHED_SEDF     4

/* Set or get info? */
#define SCHED_INFO_PUT 0
#define SCHED_INFO_GET 1

/*
 * Generic scheduler control command - used to adjust system-wide scheduler
 * parameters
 */
struct sched_ctl_cmd {
    uint32_t sched_id;
    uint32_t direction;
    union {
        struct bvt_ctl {
            uint32_t ctx_allow;
        } bvt;
    } u;
};

struct sched_adjdom_cmd {
    uint32_t sched_id;
    uint32_t direction;
    domid_t  domain;
    union {
        struct bvt_adjdom {
            uint32_t mcu_adv;      /* mcu advance: inverse of weight */
            uint32_t warpback;     /* warp? */
            int32_t  warpvalue;    /* warp value */
            int64_t  warpl;        /* warp limit */
            int64_t  warpu;        /* unwarp time requirement */
        } bvt;
        struct sedf_adjdom {
            uint64_t period;
            uint64_t slice;
            uint64_t latency;
            uint32_t extratime;
            uint32_t weight;
        } sedf;
    } u;
};

#endif /* __XEN_PUBLIC_SCHED_CTL_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
