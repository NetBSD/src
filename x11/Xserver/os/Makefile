#	$NetBSD: Makefile,v 1.11 2004/04/05 12:03:25 lukem Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	os
.PATH:	${X11SRCDIR.xc}/programs/Xserver/os
.PATH:	${X11SRCDIR.xc}/lib/xtrans
SRCS=	WaitFor.c access.c connection.c io.c oscolor.c osinit.c \
	utils.c log.c auth.c mitauth.c secauth.c xdmauth.c xdmcp.c \
	xdmcp.c transport.c lbxio.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/lbx

CPPFLAGS+=	-DXSERV_t -DTRANS_SERVER \
		${X11FLAGS.CONNECTION} \
		-DHASXDMAUTH \
		${X11FLAGS.OS_DEFINES} \
		-DUSE_RGB_TXT \
		-UDDXTIME

.include "../Makefile.common"

# XXX The XFree86 Xserver has its own GetTimeInMillis(), however if we are
#     building XFree86 and Xmacppc or Xsun* we want both copies.  The copy
#     here is linked into the non XFree86 server.
.if ${XSERVER_XFREE86} == "no" || \
    ${XSERVER_XMACPPC} != "no" || \
    ${XSERVER_XSUN} != "no"
SRCS+=	GetTimeInMillis.c
.endif

CPPFLAGS.WaitFor.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.access.c=	-DXDMCP -DBSD44SOCKETS -DHAS_GETIFADDRS
CPPFLAGS.auth.c=	-DXDMCP -DHAVE_ARC4RANDOM
CPPFLAGS.connection.c=	-DXDMCP -DBSD44SOCKETS
CPPFLAGS.io.c=		${X11FLAGS.EXTENSION}
CPPFLAGS.lbxio.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.oscolor.c=	-DNDBM
CPPFLAGS.osinit.c=	-DADMPATH=\"/var/log/X\%smsgs\"
CPPFLAGS.transport.c=	-DBSD44SOCKETS -I${X11SRCDIR.xc}/lib/xtrans
CPPFLAGS.utils.c=	-DXDMCP ${X11FLAGS.EXTENSION}
CPPFLAGS.xdmauth.c=	-DXDMCP
COPTS.xdmauth.c=	-Wno-error	# XXX
CPPFLAGS.xdmcp.c=	-DXDMCP -DBSD44SOCKETS

BUILDOBJS=		libc_wrapper.o
.PATH:			${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/shared
CPPFLAGS.libc_wrapper.c=-DSELF_CONTAINED_WRAPPER
CLEANFILES+=		${BUILDOBJS}
realall: ${BUILDOBJS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
