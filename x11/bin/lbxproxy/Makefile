#	$NetBSD: Makefile,v 1.5 2004/04/05 12:03:25 lukem Exp $

.include <bsd.own.mk>

PROG=		lbxproxy


.PATH:		${X11SRCDIR.xc}/programs/${PROG}/di
SRCS+=		atomcache.c cache.c cmap.c cmaputil.c dispatch.c extensions.c \
		gfx.c globals.c init.c lbxfuncs.c lbxutil.c main.c options.c \
		pm.c props.c reqtype.c resource.c swaprep.c swapreq.c \
		tables.c tags.c unsquish.c utils.c wire.c zeropad.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/${PROG}/include \
		${X11FLAGS.SERVER}

CPPFLAGS.dispatch.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.utils.c=	-DDEF_ATOMS_FILE=\"${X11LIBDIR}/lbxproxy/AtomControl\" \
			-DRGB_DB=\"${X11LIBDIR}/rgb\"


.PATH:		${X11SRCDIR.xc}/programs/${PROG}/os
.PATH:		${X11SRCDIR.xc}/lib/xtrans
SRCS.os=	WaitFor.c connection.c io.c osinit.c transport.c 
SRCS+=		${SRCS.os}
.for _S in ${SRCS.os}
CPPFLAGS.${_S}=	${X11FLAGS.CONNECTION} \
		${X11FLAGS.OS_DEFINES}
.endfor

CPPFLAGS.connection.c+=	-DLBXPROXY_t -DTRANS_SERVER -DBSD44SOCKETS
CPPFLAGS.io.c+=		-DLBXPROXY_t -DTRANS_SERVER ${X11FLAGS.EXTENSION}
CPPFLAGS.transport.c+=	-DLBXPROXY_t -DTRANS_SERVER -DBSD44SOCKETS \
			-I${X11SRCDIR.xc}/lib/xtrans  

.PATH:		${X11SRCDIR.xc}/programs/${PROG}/config
FILES=		AtomControl
FILESDIR=	${X11LIBDIR}/lbxproxy

LDADD+=	-llbxutil -lICE -lXext -lX11 -lz
DPADD+=	${LIBLBXUTIL} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
