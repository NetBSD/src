#	$NetBSD: Makefile.inc,v 1.7 2008/07/18 16:20:03 pooka Exp $
#

LIBISPRIVATE=	yes
NOMAN=		wizd

WARNS?=		4
DBG=		-O0 -g

CPPFLAGS+=	-I${.CURDIR} -I.
.if "${RUMPKERNEL}" != "no"
CFLAGS+=	-ffreestanding
CPPFLAGS:=	-I${NETBSDSRCDIR}/sys/rump/include ${CPPFLAGS}
CPPFLAGS+=	-D_KERNEL -I${NETBSDSRCDIR}/common/include
CPPFLAGS+=	-D_RUMPKERNEL -DDEBUG -DDIAGNOSTIC
CPPFLAGS+=	-nostdinc -I${NETBSDSRCDIR}/sys
.endif
