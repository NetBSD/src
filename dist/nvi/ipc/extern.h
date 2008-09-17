/*	$NetBSD: extern.h,v 1.1.1.1.4.2 2008/09/17 04:45:25 wrstuden Exp $ */

/* Do not edit: automatically built by build/distrib. */
int vi_run __P((IPVI *, int, char *[]));
int vi_send __P((int, char *, IP_BUF *));
int vi_input __P((IPVIWIN *, int));
int vi_wsend __P((IPVIWIN*, char *, IP_BUF *));
int vi_translate __P((IPVIWIN *, char *, size_t *, IP_BUF *));
int vi_create __P((IPVI **, u_int32_t));
