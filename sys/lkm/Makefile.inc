#	$NetBSD: Makefile.inc,v 1.3.6.1 2002/01/08 00:32:55 nathanw Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

NOMAN=	# defined

.include <bsd.own.mk>

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
