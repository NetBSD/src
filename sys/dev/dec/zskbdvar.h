/* $NetBSD: zskbdvar.h,v 1.1 1998/10/22 00:53:24 briggs Exp $ */

extern int zs_ioasic_cnattach __P((tc_addr_t, tc_offset_t, int, int, int));
extern int zs_ioasic_lk201_cnattach __P((tc_addr_t, tc_offset_t, int));
extern int zskbd_cnattach __P((struct zs_chanstate *));
extern int zs_getc __P((struct zs_chanstate *));
