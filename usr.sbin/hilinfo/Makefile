#	$NetBSD: Makefile,v 1.7 2002/09/18 03:54:29 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

.if ${MACHINE} == "hp300"
PROG=	hilinfo
CPPFLAGS+= -I${NETBSDSRCDIR}/sys/arch/hp300
.endif

.include <bsd.prog.mk>
