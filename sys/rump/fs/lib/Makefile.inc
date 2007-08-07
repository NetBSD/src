#	$NetBSD: Makefile.inc,v 1.2 2007/08/07 10:30:39 pooka Exp $
#

MKPRIVATELIB=	yes
NOMAN=		wizd
MKLINT?=	no
MKPIC?=		no

WARNS?=		4
DBG=		-O0 -g

CPPFLAGS+=	-I${.CURDIR} -I.
.if "${RUMPKERNEL}" != "no"
CFLAGS+=	-ffreestanding
CPPFLAGS:=	-I${NETBSDSRCDIR}/sys/rump/include ${CPPFLAGS}
CPPFLAGS+=	-D_KERNEL -I${NETBSDSRCDIR}/common/include
CPPFLAGS+=	-nostdinc -I${NETBSDSRCDIR}/sys
.endif
