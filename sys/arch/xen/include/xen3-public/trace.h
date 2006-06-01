/* $NetBSD: trace.h,v 1.1.1.1.6.2 2006/06/01 22:35:37 kardel Exp $ */
/******************************************************************************
 * include/public/trace.h
 * 
 * Mark Williamson, (C) 2004 Intel Research Cambridge
 * Copyright (C) 2005 Bin Ren
 */

#ifndef __XEN_PUBLIC_TRACE_H__
#define __XEN_PUBLIC_TRACE_H__

/* Trace classes */
#define TRC_CLS_SHIFT 16
#define TRC_GEN     0x0001f000    /* General trace            */
#define TRC_SCHED   0x0002f000    /* Xen Scheduler trace      */
#define TRC_DOM0OP  0x0004f000    /* Xen DOM0 operation trace */
#define TRC_VMX     0x0008f000    /* Xen VMX trace            */
#define TRC_MEM     0x000af000    /* Xen memory trace         */
#define TRC_ALL     0xfffff000

/* Trace subclasses */
#define TRC_SUBCLS_SHIFT 12
/* trace subclasses for VMX */
#define TRC_VMXEXIT  0x00081000   /* VMX exit trace            */
#define TRC_VMXTIMER 0x00082000   /* VMX timer trace           */
#define TRC_VMXINT   0x00084000   /* VMX interrupt trace       */
#define TRC_VMXIO    0x00088000   /* VMX io emulation trace  */

/* Trace events per class */

#define TRC_SCHED_DOM_ADD       (TRC_SCHED +  1)
#define TRC_SCHED_DOM_REM       (TRC_SCHED +  2)
#define TRC_SCHED_SLEEP         (TRC_SCHED +  3)
#define TRC_SCHED_WAKE          (TRC_SCHED +  4)
#define TRC_SCHED_YIELD         (TRC_SCHED +  5)
#define TRC_SCHED_BLOCK         (TRC_SCHED +  6)
#define TRC_SCHED_SHUTDOWN      (TRC_SCHED +  7)
#define TRC_SCHED_CTL           (TRC_SCHED +  8)
#define TRC_SCHED_ADJDOM        (TRC_SCHED +  9)
#define TRC_SCHED_SWITCH        (TRC_SCHED + 10)
#define TRC_SCHED_S_TIMER_FN    (TRC_SCHED + 11)
#define TRC_SCHED_T_TIMER_FN    (TRC_SCHED + 12)
#define TRC_SCHED_DOM_TIMER_FN  (TRC_SCHED + 13)
#define TRC_SCHED_SWITCH_INFPREV (TRC_SCHED + 14)
#define TRC_SCHED_SWITCH_INFNEXT (TRC_SCHED + 15)

#define TRC_MEM_PAGE_GRANT_MAP      (TRC_MEM + 1)
#define TRC_MEM_PAGE_GRANT_UNMAP    (TRC_MEM + 2)
#define TRC_MEM_PAGE_GRANT_TRANSFER (TRC_MEM + 3)

/* trace events per subclass */
#define TRC_VMX_VMEXIT          (TRC_VMXEXIT + 1)
#define TRC_VMX_VMENTRY         (TRC_VMXEXIT + 2)

#define TRC_VMX_TIMER_INTR      (TRC_VMXTIMER + 1)

#define TRC_VMX_INT             (TRC_VMXINT + 1)


/* This structure represents a single trace buffer record. */
struct t_rec {
    uint64_t cycles;          /* cycle counter timestamp */
    uint32_t event;           /* event ID                */
    unsigned long data[5];    /* event data items        */
};

/*
 * This structure contains the metadata for a single trace buffer.  The head
 * field, indexes into an array of struct t_rec's.
 */
struct t_buf {
    uint32_t cons;      /* Next item to be consumed by control tools. */
    uint32_t prod;      /* Next item to be produced by Xen.           */
    /* 'nr_recs' records follow immediately after the meta-data header.    */
};

#endif /* __XEN_PUBLIC_TRACE_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
