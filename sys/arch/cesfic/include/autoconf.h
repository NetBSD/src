/* $NetBSD: autoconf.h,v 1.1.130.1 2009/05/04 08:10:53 yamt Exp $ */

#ifdef _KERNEL

int mainbus_map(u_long, int, int, void**);

int cesfic_getetheraddr(unsigned char *);

#endif /* _KERNEL */
