/*	$NetBSD: pic.h,v 1.10 2020/04/25 15:26:18 bouyer Exp $	*/

#ifndef _X86_PIC_H
#define _X86_PIC_H

struct cpu_info;

/* 
 * Structure common to all PIC softcs
 */
struct pic {
	const char *pic_name;
	int pic_type;
	int pic_vecbase;
	int pic_apicid;
	__cpu_simple_lock_t pic_lock;
	void (*pic_hwmask)(struct pic *, int);
	void (*pic_hwunmask)(struct pic *, int);
	void (*pic_addroute)(struct pic *, struct cpu_info *, int, int, int);
	void (*pic_delroute)(struct pic *, struct cpu_info *, int, int, int);
	bool (*pic_trymask)(struct pic *, int);
	struct intrstub *pic_level_stubs;
	struct intrstub *pic_edge_stubs;
	struct ioapic_softc *pic_ioapic; /* if pic_type == PIC_IOAPIC */
	struct msipic *pic_msipic; /* if (pic_type == PIC_MSI) || (pic_type == PIC_MSIX) */
	/* interface for subr_interrupt.c */
	void (*pic_intr_get_devname)(const char *, char *, size_t);
	void (*pic_intr_get_assigned)(const char *, kcpuset_t *);
	uint64_t (*pic_intr_get_count)(const char *, u_int);
};

/*
 * PIC types.
 */
#define PIC_I8259	0
#define PIC_IOAPIC	1
#define PIC_LAPIC	2
#define PIC_MSI		3
#define PIC_MSIX	4
#define PIC_SOFT	5
#define PIC_XEN		6

extern struct pic i8259_pic;
extern struct pic local_pic;
extern struct pic softintr_pic;
extern struct pic xen_pic;
#endif
