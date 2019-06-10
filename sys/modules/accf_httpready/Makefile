#	$NetBSD: Makefile,v 1.2.18.1 2019/06/10 22:09:06 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/netinet

KMOD=	accf_httpready
IOCONF=	accf_http.ioconf
SRCS=	accf_http.c

WARNS=	3

.include <bsd.kmodule.mk>
