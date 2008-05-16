#	$NetBSD: Makefile.inc,v 1.4.22.1 2008/05/16 02:25:50 yamt Exp $
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
