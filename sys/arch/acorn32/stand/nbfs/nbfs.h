/* $NetBSD: nbfs.h,v 1.2 2006/07/13 15:51:54 bjh21 Exp $ */

#define NBFS_FSNUM 0xf0 /* borrow UnixFS's number for now */

#ifndef __ASSEMBLER__
/* Structure passed to and from FSEntry_* entry points */
struct nbfs_reg {
	uint32_t	r0, r1, r2, r3, r4, r5, r6, r7;
};

extern os_error const *nbfs_open    (struct nbfs_reg *);
extern os_error const *nbfs_getbytes(struct nbfs_reg *);
extern os_error const *nbfs_putbytes(struct nbfs_reg *);
extern os_error const *nbfs_args    (struct nbfs_reg *);
extern os_error const *nbfs_close   (struct nbfs_reg *);
extern os_error const *nbfs_file    (struct nbfs_reg *);
extern os_error const *nbfs_func    (struct nbfs_reg *);
#endif /* __ASSEMBLER__ */
