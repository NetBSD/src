#	$NetBSD: Makefile,v 1.2 2003/09/23 23:21:13 lukem Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		os
.PATH:		${X11SRCDIR.xc}/programs/Xserver/os
.PATH:		${X11SRCDIR.xc}/lib/xtrans

SRCS=		WaitFor.c access.c connection.c io.c oscolor.c osinit.c \
		utils.c auth.c mitauth.c secauth.c xdmauth.c xdmcp.c \
		decompress.c transport.c lbxio.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${XSERVERINCDIR} \
		-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/lbx

CPPFLAGS+=	-DXSERV_t -DTRANS_SERVER \
		${X11FLAGS.CONNECTION} -DHASXDMAUTH -DDDXOSINIT \
		-DSERVER_LOCK -DDDXOSFATALERROR -DDDXOSVERRORF -DUSE_RGB_TXT

CPPFLAGS.WaitFor.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.access.c=	-DXDMCP -DBSD44SOCKETS -DHAS_GETIFADDRS
CPPFLAGS.auth.c=	-DXDMCP
CPPFLAGS.connection.c=	-DXDMCP -DBSD44SOCKETS
CPPFLAGS.io.c=		${X11FLAGS.EXTENSION}
CPPFLAGS.lbxio.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.oscolor.c=	-DNDBM
CPPFLAGS.osinit.c=	-DADMPATH=\"/var/log/X\%smsgs\"
CPPFLAGS.transport.c=	-DBSD44SOCKETS
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
