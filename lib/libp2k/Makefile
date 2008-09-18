#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 04:39:23 wrstuden Exp $
#

LIB=		p2k
LIBDPLIBS+=     rump    ${.CURDIR}/../librump	\
		ukfs	${.CURDIR}/../libukfs

INCS=		p2k.h
INCSDIR=	/usr/include/rump

SRCS=		p2k.c
MAN=		p2k.3

.include <bsd.lib.mk>
