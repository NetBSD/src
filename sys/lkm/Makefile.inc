#	$NetBSD: Makefile.inc,v 1.4.32.1 2006/06/21 15:10:23 yamt Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
