#	$NetBSD: Makefile,v 1.6 2001/12/12 01:48:56 tv Exp $

.if ${MACHINE} == "amiga"
PROG=videomode
CPPFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.else
NOOBJ=	# defined
.endif
NOMAN=	# defined

.include <bsd.prog.mk>
