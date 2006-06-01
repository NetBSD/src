#	$NetBSD: Makefile.inc,v 1.7.6.1 2006/06/01 22:38:28 kardel Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined


.include <bsd.own.mk>

CPPFLAGS+=-I${NETBSDSRCDIR}/common/include

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
