/* $NetBSD: autoconf.h,v 1.1.144.1 2009/05/13 17:16:36 jym Exp $ */

#ifdef _KERNEL

int mainbus_map(u_long, int, int, void**);

int cesfic_getetheraddr(unsigned char *);

#endif /* _KERNEL */
