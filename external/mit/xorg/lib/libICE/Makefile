#	$NetBSD: Makefile,v 1.2.6.2 2008/10/19 22:41:08 haad Exp $

.include <bsd.own.mk>

LIB=	ICE
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/ICE

SRCS=	accept.c authutil.c connect.c error.c getauth.c iceauth.c icetrans.c \
	listen.c listenwk.c locking.c misc.c ping.c process.c protosetup.c \
	register.c replywait.c setauth.c shutdown.c watch.c

INCS=	ICE.h ICEconn.h ICElib.h ICEmsg.h ICEproto.h ICEutil.h
INCSDIR=${X11INCDIR}/X11/ICE

CONN_DEFINES=	${X11FLAGS.CONNECTION} -DICE_t=1 -DTRANS_CLIENT -DTRANS_SERVER
SOCK_DEFINES=	-DBSD44SOCKETS

CPPFLAGS+=	$(TRANS_INCLUDES) $(CONN_DEFINES) $(SOCK_DEFINES)

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
