#	$NetBSD: Makefile,v 1.7 2002/09/18 03:54:38 lukem Exp $

NOMAN=	# defined
.include <bsd.own.mk>

.if ${MACHINE} == "amiga"
PROG=videomode
CPPFLAGS+=-I${NETBSDSRCDIR}/sys/arch -D${MACHINE}
.endif

.include <bsd.prog.mk>
