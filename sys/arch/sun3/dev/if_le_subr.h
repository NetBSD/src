/*	$NetBSD: if_le_subr.h,v 1.6 1994/12/12 18:59:16 gwr Exp $	*/

/* One might also set: LE_C3_ACON | LE_C3_BCON */
#define	LE_C3_CONFIG LE_C3_BSWP

extern int  le_md_match(struct device *, void *, void *args);
extern void le_md_attach(struct device *, struct device *, void *);
extern int  le_intr(void *);
