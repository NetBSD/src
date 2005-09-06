#	$NetBSD: Makefile.inc,v 1.6 2005/09/06 23:25:54 christos Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined

.include <bsd.own.mk>

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
