#	$NetBSD: Makefile,v 1.1 2020/08/20 21:28:01 riastradh Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netinet	\
	${.CURDIR}/../../../../netinet6

LIB=	rumpnet_wireguard
COMMENT= WireGuard

IOCONF=	WG.ioconf
SRCS=	if_wg.c

CPPFLAGS.if_wg.c+=	-DWG_RUMPKERNEL

# For sysctl knobs
CPPFLAGS.if_wg.c+=	-DWG_DEBUG_PARAMS
.ifdef RUMP_DEBUG
CPPFLAGS.if_wg.c+=	-DWG_DEBUG_TRACE -DWG_DEBUG_LOG
.endif

SRCS+=	wg_component.c

#SRCS+=	wg_user.c
RUMPCOMP_USER_SRCS=	wg_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
