#	$NetBSD: Makefile,v 1.7 2007/04/21 00:41:19 matt Exp $

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

LBXPROXYDIR=	${X11ETCDIR}/lbxproxy

CPPFLAGS.dispatch.c=	${X11FLAGS.EXTENSION}
CPPFLAGS.utils.c=	-DDEF_ATOMS_FILE=\"${LBXPROXYDIR}/AtomControl\" \
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

.if ${MACHINE} == "vax"
COPTS.connection.c+=	-O2
.endif

.PATH:		${X11SRCDIR.xc}/programs/${PROG}/config
CONFIGFILES=	AtomControl
FILESDIR=	${LBXPROXYDIR}

LDADD+=	-llbxutil -lICE -lXext -lX11 -lz
DPADD+=	${LIBLBXUTIL} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
