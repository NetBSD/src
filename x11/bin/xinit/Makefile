#	$NetBSD: Makefile,v 1.8 2010/10/10 07:03:28 tsutsui Exp $

.include <bsd.own.mk>

.include "../../Xserver/Makefile.common"

PROG=	xinit
MAN=	xinit.1 startx.1

XINITDIR=	${X11ETCDIR}/xinit

CPPFLAGS+=	-DHAS_VFORK ${X11FLAGS.CONNECTION} -DBINDIR=\"${X11BINDIR}\" \

CPPFLAGS+=	-DXFREE86
.if (${USE_PAM} != "no")
# XXX: Legacy non-XFree86 servers don't handle PAM
.if \
    ${MACHINE} != "alpha"	&& \
    ${MACHINE} != "amiga"	&& \
    ${MACHINE} != "dreamcast"	&& \
    ${MACHINE} != "ews4800mips"	&& \
    ${MACHINE} != "hpcarm"	&& \
    ${MACHINE} != "hpcmips"	&& \
    ${MACHINE} != "hpcsh"	&& \
    ${MACHINE} != "newsmips"	&& \
    ${MACHINE} != "pmax"	&& \
    ${MACHINE} != "sun3"	&& \
    ${MACHINE} != "x68k"	&& \
    1
CPPFLAGS+=	-DUSE_PAM
.endif
.endif

CPPSCRIPTS=	startx xinitrc
CPPSCRIPTFLAGS_xinitrc=	-DXINITDIR=${XINITDIR} -DBINDIR=${X11BINDIR}
CPPSCRIPTFLAGS_startx=	-DXINITDIR=${XINITDIR} -DBINDIR=${X11BINDIR} \
		-DHAS_COOKIE_MAKER \
		-DMK_COOKIE='dd if=/dev/urandom bs=16 count=1 2>/dev/null | hexdump -e \\"%08x\\"'

SCRIPTS=	startx
CONFIGFILES=	xinitrc
FILESDIR=	${XINITDIR}

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

X11EXTRAMANDEFS=-D__xinitdir__=${XINITDIR}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
