/* $NetBSD: pxe_netif.h,v 1.1 2003/03/12 17:33:10 drochner Exp $ */

int pxe_netif_open(void);
void pxe_netif_close(int);
void pxe_netif_shutdown(void);
