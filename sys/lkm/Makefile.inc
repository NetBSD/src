#	$NetBSD: Makefile.inc,v 1.9 2006/05/20 22:22:04 christos Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include

. if ${HAVE_GCC} > 3
CWARNFLAGS+=    -Wno-pointer-sign
. endif

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
