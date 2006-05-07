#	$NetBSD: Makefile.inc,v 1.8 2006/05/07 20:15:14 christos Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
