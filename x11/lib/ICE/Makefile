#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:52:18 lukem Exp $

.include <bsd.own.mk>

LIB=	ICE
.PATH:	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${X11SRCDIR.xc}/lib/xtrans

SRCS=	accept.c authutil.c connect.c error.c getauth.c iceauth.c listen.c \
	listenwk.c locking.c misc.c ping.c process.c protosetup.c \
	register.c replywait.c setauth.c shutdown.c transport.c watch.c

INCS=	ICE.h ICEconn.h ICElib.h ICEmsg.h ICEproto.h ICEutil.h
INCSDIR=${X11INCDIR}/X11/ICE


CONN_DEFINES=	${X11FLAGS.CONNECTION} -DICE_t -DTRANS_CLIENT -DTRANS_SERVER
SOCK_DEFINES=	-DBSD44SOCKETS
TRANS_INCLUDES=	-I${X11SRCDIR.xc}/lib/xtrans

CPPFLAGS.accept.c=	$(TRANS_INCLUDES) $(CONN_DEFINES)
CPPFLAGS.connect.c=	$(TRANS_INCLUDES) $(CONN_DEFINES)
CPPFLAGS.listen.c=	$(TRANS_INCLUDES) $(CONN_DEFINES)
CPPFLAGS.listenwk.c=	$(TRANS_INCLUDES) $(CONN_DEFINES)
CPPFLAGS.misc.c=	$(TRANS_INCLUDES) $(CONN_DEFINES)
CPPFLAGS.shutdown.c=	$(TRANS_INCLUDES) $(CONN_DEFINES)
CPPFLAGS.transport.c=	$(TRANS_INCLUDES) $(CONN_DEFINES) $(SOCK_DEFINES)

.include <bsd.lib.mk>
.include <bsd.x11.mk>
