#	$NetBSD: Makefile.inc,v 1.10 2006/05/21 04:43:58 christos Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include

. if ${HAVE_GCC} > 3
CWARNFLAGS+=    -Wno-pointer-sign -fno-strict-aliasi
CFLAGS+=	-fno-strict-aliasing
. endif

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
