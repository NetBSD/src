#	$NetBSD: Makefile,v 1.3 2019/02/17 04:05:41 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/netinet

KMOD=	accf_httpready
IOCONF=	accf_http.ioconf
SRCS=	accf_http.c

WARNS=	3

.include <bsd.kmodule.mk>
