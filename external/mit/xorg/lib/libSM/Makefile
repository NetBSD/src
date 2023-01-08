#	$NetBSD: Makefile,v 1.5 2023/01/08 09:10:58 mrg Exp $

.include <bsd.own.mk>

LIB=	SM
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/SM

SRCS=	sm_auth.c sm_client.c sm_error.c sm_genid.c sm_manager.c sm_misc.c \
	sm_process.c

INCS=	SM.h SMlib.h SMproto.h
INCSDIR=${X11INCDIR}/X11/SM

CPPFLAGS+=	${X11FLAGS.THREADLIB}
# XXX not yet; libuuid_ul is not yet ready.
#CPPFLAGS+=	-DHAVE_LIBUUID

CONN_DEFINES=	${X11FLAGS.CONNECTION} -DICE_t

CPPFLAGS.sm_manager.c=	$(CONN_DEFINES)
CPPFLAGS.sm_genid.c=	$(CONN_DEFINES)

.if (${USE_INET6} != "no")
CPPFLAGS+=      -DIPv6
.endif

LIBDPLIBS=ICE ${.CURDIR}/../libICE

# XXXMRG
COPTS.sm_client.c=	-Wno-error
COPTS.sm_manager.c=	-Wno-error

PKGDIST=	${LIB}
PKGCONFIG_SED_FLAGS=	-e "s,@UUID_PC@,," -e "s,@UUID_LIB_PRIVATE@,,"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
