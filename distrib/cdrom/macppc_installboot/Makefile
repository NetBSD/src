#	$NetBSD: Makefile,v 1.2 2002/09/29 06:38:19 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=		macppc_installboot
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/arch

CLEANFILES+=	mkmacppcboot

.include <bsd.prog.mk>

mkmacppcboot: mkmacppcboot.c
	${LINK.c} -o $@ $>

macppcboot.raw: mkmacppcboot bootxx.raw
	./mkmacppcboot <${.CURDIR}/bootxx.raw >$@
