#	$NetBSD: Makefile.inc,v 1.4.32.2 2006/12/30 20:50:16 yamt Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include
USE_SSP = no

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
