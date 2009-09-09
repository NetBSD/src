#	$NetBSD: Makefile.inc,v 1.14.48.1 2009/09/09 21:14:59 matt Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined

.if ${MACHINE_ARCH:Mmips64*} != ""
CPUFLAGS+=	-mabi=64
LDFLAGS+=	-mabi=64
.endif


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include
USE_FORT = no

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
