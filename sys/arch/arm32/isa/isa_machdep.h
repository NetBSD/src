/*	$NetBSD: isa_machdep.h,v 1.2 1998/06/12 21:07:44 tv Exp $	*/

#ifndef _ARM32_ISA_MACHDEP_H_
#define _ARM32_ISA_MACHDEP_H_

/*
 * Types provided to machine-independent ISA code.
 */
typedef void *isa_chipset_tag_t;
#define isa_dmainit(a,b,c,d) /* XXX - needs to change for MI ISADMA */

struct device;			/* XXX */
struct isabus_attach_args;	/* XXX */

/*
 * ISA DMA bounce buffers.
 * XXX should be made partially machine- and bus-mapping-independent.
 *
 * DMA_BOUNCE is the number of pages of low-addressed physical memory
 * to acquire for ISA bounce buffers.
 *
 * isaphysmem is the location of those bounce buffers.  (They are currently
 * assumed to be contiguous.
 */

#ifndef DMA_BOUNCE
#define	DMA_BOUNCE      8		/* one buffer per channel */
#endif

#define DMA_LARGE_BUFFER_SIZE 65536 /* 64K */

extern vm_offset_t isaphysmem;

/*
 * Functions provided to machine-independent ISA code.
 */
void	isa_attach_hook __P((struct device *, struct device *,
	    struct isabus_attach_args *));
void	*isa_intr_establish __P((isa_chipset_tag_t ic, int irq, int type,
	    int level, int (*ih_fun)(void *), void *ih_arg));
void	isa_intr_disestablish __P((isa_chipset_tag_t ic, void *handler));

/*
 * ALL OF THE FOLLOWING ARE MACHINE-DEPENDENT, AND SHOULD NOT BE USED
 * BY PORTABLE CODE.
 */

/* bus space tags */
extern struct bus_space isa_io_bs_tag;
extern struct bus_space isa_mem_bs_tag;

/* for pccons.c */
#define MONO_BASE           0x3B4
#define MONO_BUF            0x000B0000
#define CGA_BASE            0x3D4
#define CGA_BUF             0x000B8000
#define VGA_BUF             0xA0000
#define VGA_BUF_LEN         (0xBFFFF - 0xA0000)


void		isa_init __P((vm_offset_t, vm_offset_t));
void		isa_io_init __P((vm_offset_t, vm_offset_t));
vm_offset_t	isa_io_data_vaddr __P((void));
vm_offset_t	isa_mem_data_vaddr __P((void));

#endif	/* _ARM32_ISA_MACHDEP_H_ XXX */
