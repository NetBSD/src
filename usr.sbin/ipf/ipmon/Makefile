#	$NetBSD: Makefile,v 1.2 1997/03/29 04:28:39 darrenr Exp $

PROG=	ipmon
MAN=	ipmon.8
CFLAGS+=        -DIPL_NAME=\"/dev/ipl\" -DLOGFAC=LOG_LOCAL0 -I${.CURDIR}/../../../sbin/ipf -I${.CURDIR}/../../../sys

.PATH: .${CURDIR}/../../../sbin/ipf
.include <bsd.prog.mk>
