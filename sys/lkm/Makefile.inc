#	$NetBSD: Makefile.inc,v 1.3.8.1 2002/01/10 20:00:43 thorpej Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined

.include <bsd.own.mk>

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
