/* $NetBSD: nbfs.h,v 1.1.6.2 2006/04/22 11:37:10 simonb Exp $ */

/* Structure passed to and from FSEntry_* entry points */
struct nbfs_reg {
	uint32_t	r0, r1, r2, r3, r4, r5, r6, r7;
};

extern os_error *nbfs_open    (struct nbfs_reg *);
extern os_error *nbfs_getbytes(struct nbfs_reg *);
extern os_error *nbfs_putbytes(struct nbfs_reg *);
extern os_error *nbfs_args    (struct nbfs_reg *);
extern os_error *nbfs_close   (struct nbfs_reg *);
extern os_error *nbfs_file    (struct nbfs_reg *);
extern os_error *nbfs_func    (struct nbfs_reg *);

