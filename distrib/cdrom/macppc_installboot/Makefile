#	$NetBSD: Makefile,v 1.5 2004/03/19 08:07:28 jmc Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROG=		macppc_installboot
HOST_CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/arch
UUDECODE_FILES=	bootxx.raw macppcboot.raw

CLEANFILES+=	mkmacppcboot

all: macppcboot.raw

.include <bsd.files.mk>
.include <bsd.hostprog.mk>

mkmacppcboot: mkmacppcboot.c
	${LINK.c} -o ${.TARGET} ${.ALLSRC}

regen: mkmacppcboot bootxx.raw.uue
	./mkmacppcboot <${.CURDIR}/bootxx.raw | \
	    uuencode macppcboot.raw > macppcboot.raw.uue
