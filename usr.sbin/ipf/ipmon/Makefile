#	$NetBSD: Makefile,v 1.1.1.2 1997/03/29 02:49:46 darrenr Exp $

PROG=	ipmon
MAN=	ipmon.8
CFLAGS+=        -DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../../sbin/ipf

.PATH: .${CURDIR}/../../sbin/ipf
.include <bsd.prog.mk>
