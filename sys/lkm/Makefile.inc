#	$NetBSD: Makefile.inc,v 1.13.10.1 2007/07/11 20:10:34 mjf Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include
USE_FORT = no

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
