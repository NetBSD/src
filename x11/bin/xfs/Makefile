#	$NetBSD: Makefile,v 1.1.1.1 2003/09/12 02:52:12 lukem Exp $

.include <bsd.own.mk>

PROG=	xfs

.PATH:	${X11SRCDIR.xc}/programs/${PROG}/difs
SRCS=	atom.c cache.c charinfo.c difsutils.c dispatch.c events.c \
	extensions.c fontinfo.c fonts.c globals.c initfonts.c main.c \
	resource.c swaprep.c swapreq.c tables.c

.PATH:	${X11SRCDIR.xc}/programs/${PROG}/os
SRCS+=	access.c config.c connection.c daemon.c error.c io.c osglue.c \
	osinit.c utils.c waitfor.c

CPPFLAGS+=		-I${X11SRCDIR.xc}/programs/xfs/include \
			-I${X11SRCDIR.xc}/include/fonts \
			-I${X11SRCDIR.xc}/lib/font/include \
			-I${X11SRCDIR.xc}/lib/font/bitmap

CONN_DEFINES=		${X11FLAGS.CONNECTION} -DFONT_t -DTRANS_SERVER \
			-DTRANS_REOPEN

CPPFLAGS.main.c=	-DDEFAULT_CONFIG_FILE=\"${X11LIBDIR}/fs/config\"
CPPFLAGS.initfonts.c=	-DFONT_PCF -DFONT_FS -DFONT_SPEEDO -DFONTCACHE
CPPFLAGS.connection.c=	${CONN_DEFINES} -DBSD44SOCKETS -DDDXOSINIT \
			-DSERVER_LOCK -DDDXOSFATALERROR -DDDXOSVERRORF
CPPFLAGS.utils.c=	-DXFSPIDDIR=\"/var/run\"
CPPFLAGS.io.c=		${CONN_DEFINES}
CPPFLAGS.waitfor.c=	${CONN_DEFINES}
CPPFLAGS.error.c=	-DUSE_SYSLOG
CPPFLAGS.osglue.c=	${CONN_DEFINES} -DDDXOSINIT \
			-DSERVER_LOCK -DDDXOSFATALERROR -DDDXOSVERRORF
CPPFLAGS.config.c=	${CONN_DEFINES} -DFONTCACHE


LNFILES=	config.cpp fsconfig.cpp
CPPSCRIPTS=	fsconfig
CPPSCRIPTFLAGS=	-DDEFAULTFONTPATH=${X11FONTDIR}/misc/,${X11FONTDIR}/Speedo/,${X11FONTDIR}/Type1/,${X11FONTDIR}/CID/,${X11FONTDIR}/75dpi/,${X11FONTDIR}/100dpi/ \
		-DFSERRORS=/var/log/xfs.log
FILES=		fsconfig
FILESNAME=	config
FILESDIR=	${X11LIBDIR}/fs


LDADD+=	-lXfont -lfntstubs -lXfontcache -lm -lz
DPADD+=	${LIBXFONT} ${LIBFNTSTUBS} ${LIBXFONTCACHE} ${LIBM} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
