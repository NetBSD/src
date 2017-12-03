#	$NetBSD: Makefile,v 1.1.12.1 2017/12/03 11:38:54 jdolecek Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpiosim
IOCONF=		gpiosim.conf
SRCS=		gpiosim.c

CPPFLAGS+=	-I${S}/gpio

.include <bsd.kmodule.mk>
