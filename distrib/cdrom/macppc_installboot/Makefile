#	$NetBSD: Makefile,v 1.4 2004/02/29 01:09:18 jmc Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROG=		macppc_installboot
HOST_CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/arch

CLEANFILES+=	mkmacppcboot bootxx.raw macppcboot.raw

all: macppcboot.raw

bootxx.raw: bootxx.raw.uue
	rm -f ${.TARGET}
	uudecode ${.ALLSRC}

macppcboot.raw: macppcboot.raw.uue
	rm -f ${.TARGET}
	uudecode ${.ALLSRC}

.include <bsd.hostprog.mk>

mkmacppcboot: mkmacppcboot.c
	${LINK.c} -o ${.TARGET} ${.ALLSRC}

regen: mkmacppcboot bootxx.raw.uue
	./mkmacppcboot <${.CURDIR}/bootxx.raw | \
	    uuencode macppcboot.raw > macppcboot.raw.uue
