/* $NetBSD: autoconf.h,v 1.1.8.2 2001/05/14 18:23:10 drochner Exp $ */

#ifdef _KERNEL

int mainbus_map __P((u_long, int, int, void**));

int cesfic_getetheraddr __P((unsigned char *));

#endif /* _KERNEL */
