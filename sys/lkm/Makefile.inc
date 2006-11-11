#	$NetBSD: Makefile.inc,v 1.13 2006/11/11 22:38:38 christos Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include
USE_SSP = no

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
