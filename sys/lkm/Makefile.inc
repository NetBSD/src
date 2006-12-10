#	$NetBSD: Makefile.inc,v 1.12.8.1 2006/12/10 07:18:59 yamt Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include
USE_SSP = no

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
