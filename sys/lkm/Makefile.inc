#	$NetBSD: Makefile.inc,v 1.3 2000/03/21 11:45:56 simonb Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

.include <bsd.own.mk>

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
