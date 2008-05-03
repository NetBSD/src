#	$NetBSD: Makefile.inc,v 1.5 2008/05/03 14:48:31 lukem Exp $
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
