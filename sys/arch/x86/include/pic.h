/*	$NetBSD: pic.h,v 1.1.18.3 2008/01/21 09:40:08 yamt Exp $	*/

#ifndef _X86_PIC_H
#define _X86_PIC_H

#include <sys/device.h>

struct cpu_info;

/* 
 * Structure common to all PIC softcs
 */
struct pic {
	struct device pic_dev;
        int pic_type;
	int pic_vecbase;
	int pic_apicid;
	__cpu_simple_lock_t pic_lock;
        void (*pic_hwmask)(struct pic *, int);
        void (*pic_hwunmask)(struct pic *, int);
	void (*pic_addroute)(struct pic *, struct cpu_info *, int, int, int);
	void (*pic_delroute)(struct pic *, struct cpu_info *, int, int, int);
	struct intrstub *pic_level_stubs;
	struct intrstub *pic_edge_stubs;
};

#define pic_name pic_dev.dv_xname

/*
 * PIC types.
 */
#define PIC_I8259	0
#define PIC_IOAPIC	1
#define PIC_LAPIC	2
#define PIC_SOFT	3

extern struct pic i8259_pic;
extern struct pic local_pic;
extern struct pic softintr_pic;
#endif
