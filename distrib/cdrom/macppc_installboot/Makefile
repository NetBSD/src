#	$NetBSD: Makefile,v 1.3 2002/10/21 05:08:24 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>

PROG=		macppc_installboot
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/arch

CLEANFILES+=	mkmacppcboot bootxx.raw macppcboot.raw

all: macppcboot.raw

bootxx.raw: bootxx.raw.uue
	rm -f ${.TARGET}
	uudecode ${.ALLSRC}

macppcboot.raw: macppcboot.raw.uue
	rm -f ${.TARGET}
	uudecode ${.ALLSRC}

.include <bsd.prog.mk>

mkmacppcboot: mkmacppcboot.c
	${LINK.c} -o ${.TARGET} ${.ALLSRC}

regen: mkmacppcboot bootxx.raw.uue
	./mkmacppcboot <${.CURDIR}/bootxx.raw | \
	    uuencode macppcboot.raw > macppcboot.raw.uue
