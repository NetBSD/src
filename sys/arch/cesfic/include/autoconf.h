/* $NetBSD: autoconf.h,v 1.1.138.1 2009/04/28 07:33:57 skrll Exp $ */

#ifdef _KERNEL

int mainbus_map(u_long, int, int, void**);

int cesfic_getetheraddr(unsigned char *);

#endif /* _KERNEL */
