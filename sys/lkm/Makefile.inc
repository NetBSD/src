#	$NetBSD: Makefile.inc,v 1.14 2007/05/28 12:06:24 tls Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include
USE_FORT = no

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
