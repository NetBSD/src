#	$NetBSD: Makefile.inc,v 1.7.4.1 2006/09/09 02:57:53 rpaulo Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
