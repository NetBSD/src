/* $NetBSD: autoconf.h,v 1.13 1998/02/13 01:29:09 thorpej Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Machine-dependent structures of autoconfiguration
 */

struct confargs;

typedef int (*intr_handler_t) __P((void *));

struct abus {
	struct	device *ab_dv;		/* back-pointer to device */
	int	ab_type;		/* bus type (see below) */
	void	(*ab_intr_establish)	/* bus's set-handler function */
		    __P((struct confargs *, intr_handler_t, void *));
	void	(*ab_intr_disestablish)	/* bus's unset-handler function */
		    __P((struct confargs *));
	caddr_t	(*ab_cvtaddr)		/* convert slot/offset to address */
		    __P((struct confargs *));
	int	(*ab_matchname)		/* see if name matches driver */
		    __P((struct confargs *, char *));
};

#define	BUS_MAIN	1		/* mainbus */
#define	BUS_TC		2		/* TurboChannel */
#define	BUS_ASIC	3		/* IOCTL ASIC; under TurboChannel */
#define	BUS_TCDS	4		/* TCDS ASIC; under TurboChannel */

#define	BUS_INTR_ESTABLISH(ca, handler, val)				\
	    (*(ca)->ca_bus->ab_intr_establish)((ca), (handler), (val))
#define	BUS_INTR_DISESTABLISH(ca)					\
	    (*(ca)->ca_bus->ab_intr_establish)(ca)
#define	BUS_CVTADDR(ca)							\
	    (*(ca)->ca_bus->ab_cvtaddr)(ca)
#define	BUS_MATCHNAME(ca, name)						\
	    (*(ca)->ca_bus->ab_matchname)((ca), (name))

struct confargs {
	char	*ca_name;		/* Device name. */
	int	ca_slot;		/* Device slot. */
	int	ca_offset;		/* Offset into slot. */
	struct	abus *ca_bus;		/* bus device resides on. */
};

struct bootdev_data {
	char	*protocol;
	int	bus;
	int	slot;
	int	channel;
	char	*remote_address;
	int	unit;
	int	boot_dev_type;
	char	*ctrl_dev_type;
};

/*
 * The boot program passes a pointer (in the boot environment virtual
 * address address space; "BEVA") to a bootinfo to the kernel using
 * the following convention:
 *
 *	a0 contains first free page frame number
 *	a1 contains page number of current level 1 page table
 *	if a2 contains BOOTINFO_MAGIC and a4 is nonzero:
 *		a3 contains pointer (BEVA) to bootinfo
 *		a4 contains bootinfo version number
 *	if a2 contains BOOTINFO_MAGIC and a4 contains 0 (backward compat):
 *		a3 contains pointer (BEVA) to bootinfo version
 *		    (u_long), then the bootinfo
 */

#define	BOOTINFO_MAGIC			0xdeadbeeffeedface

struct bootinfo_v1 {
	u_long	ssym;			/* 0: start of kernel sym table	*/
	u_long	esym;			/* 8: end of kernel sym table	*/
	char	boot_flags[64];		/* 16: boot flags		*/
	char	booted_kernel[64];	/* 80: name of booted kernel	*/
	void	*hwrpb;			/* 144: hwrpb pointer (BEVA)	*/
	u_long	hwrpbsize;		/* 152: size of hwrpb data	*/
	int	(*cngetc) __P((void));	/* 160: console getc pointer	*/
	void	(*cnputc) __P((int));	/* 168: console putc pointer	*/
	void	(*cnpollc) __P((int));	/* 176: console pollc pointer	*/
	u_long	pad[9];			/* 184: rsvd for future use	*/
					/* 256: total size		*/
};

/*
 * Kernel-internal structure used to hold important bits of boot
 * information.  NOT to be used by boot blocks.
 *
 * Note that not all of the fields from the bootinfo struct(s)
 * passed by the boot blocks aren't here (because they're not currently
 * used by the kernel!).  Fields here which aren't supplied by the
 * bootinfo structure passed by the boot blocks are supposed to be
 * filled in at startup with sane contents.
 */
struct bootinfo_kernel {
	u_long	ssym;			/* start of syms */
	u_long	esym;			/* end of syms */
	char	boot_flags[64];		/* boot flags */
	char	booted_kernel[64];	/* name of booted kernel */
	char	booted_dev[64];		/* name of booted device */
};

/*
 * Lookup table entry for Alpha system variations.
 */
struct alpha_variation_table {
	u_int64_t	avt_variation;	/* variation, from HWRPB */
	const char	*avt_model;	/* model string */
};

#ifdef _KERNEL
#ifdef EVCNT_COUNTERS
extern struct evcnt clock_intr_evcnt;
#endif

extern struct device *booted_device;
extern int booted_partition;
extern struct bootdev_data *bootdev_data;
extern struct bootinfo_kernel bootinfo;

const char *alpha_variation_name __P((u_int64_t,
    const struct alpha_variation_table *));
const char *alpha_unknown_sysname __P((void));
#endif /* _KERNEL */
