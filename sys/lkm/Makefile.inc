#	$NetBSD: Makefile.inc,v 1.4.32.3 2007/09/03 14:41:34 yamt Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include
USE_FORT = no

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
