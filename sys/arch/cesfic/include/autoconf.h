/* $NetBSD: autoconf.h,v 1.2 2009/03/14 14:45:58 dsl Exp $ */

#ifdef _KERNEL

int mainbus_map(u_long, int, int, void**);

int cesfic_getetheraddr(unsigned char *);

#endif /* _KERNEL */
