#	$NetBSD: Makefile.inc,v 1.2.2.1 2000/11/20 18:09:39 bouyer Exp $

.if !defined(__SYS_LKM_MAKEFILE_INC)
__SYS_LKM_MAKEFILE_INC=	1

.include <bsd.own.mk>

WARNS?=	1

.endif	# __SYS_LKM_MAKEFILE_INC
