#	$NetBSD: Makefile.inc,v 1.6.2.1 2008/07/21 14:14:13 simonb Exp $
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

# If an options are changed, trigger a rebuild
${SRCS:S/.c/.o/g}: ${.CURDIR}/../Makefile.inc

.endif
