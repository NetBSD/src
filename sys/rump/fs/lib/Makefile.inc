#	$NetBSD: Makefile.inc,v 1.4.18.1 2008/06/02 13:24:31 mjf Exp $
#

LIBISPRIVATE=	yes
NOMAN=		wizd

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
