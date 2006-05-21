#	$NetBSD: Makefile.inc,v 1.12 2006/05/21 05:43:49 christos Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
