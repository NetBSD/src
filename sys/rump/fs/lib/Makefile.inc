#	$NetBSD: Makefile.inc,v 1.3.12.1 2007/11/13 16:03:11 bouyer Exp $
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
CPPFLAGS+=	-D_KERNEL -DDIAGNOSTIC -I${NETBSDSRCDIR}/common/include
CPPFLAGS+=	-D_RUMPKERNEL
CPPFLAGS+=	-nostdinc -I${NETBSDSRCDIR}/sys
.endif
