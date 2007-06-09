#	$NetBSD: Makefile.inc,v 1.13.8.1 2007/06/09 23:58:08 ad Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include
USE_FORT = no

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
