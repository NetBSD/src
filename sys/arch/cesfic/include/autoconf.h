/* $NetBSD: autoconf.h,v 1.1 2001/05/14 18:23:09 drochner Exp $ */

#ifdef _KERNEL

int mainbus_map __P((u_long, int, int, void**));

int cesfic_getetheraddr __P((unsigned char *));

#endif /* _KERNEL */
