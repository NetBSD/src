#	$NetBSD: Makefile,v 1.5 2006/09/11 22:13:51 dbj Exp $

NOMAN=		1

.include <bsd.own.mk>

HOSTPROG=	xkbcomp
.PATH:		${X11SRCDIR.xc}/programs/${HOSTPROG}

SRCS=		xkbcomp.c xkbscan.c expr.c vmod.c indicators.c misc.c \
		keymap.c keycodes.c keytypes.c compat.c action.c alias.c \
		symbols.c geometry.c xkbpath.c listing.c \
		xkbparse.y parseutils.c utils.c

SRCS+=		xkbcomp-stubs.c xkbcomp-KeyBind.c

.PATH:		${X11SRCDIR.xc}/lib/xkbfile
SRCS+=		xkbatom.c xkberrs.c xkbout.c xkmread.c xkbtext.c \
		cout.c xkmout.c

SRCS+=		xkbfile-xkbmisc.c

.PATH:		${X11SRCDIR.xc}/lib/X11
SRCS+=		KeysymStr.c StrKeysym.c Quarks.c Xrm.c 
SRCS+=		XKB.c XKBAlloc.c XKBGAlloc.c XKBMAlloc.c XKBMisc.c \
		XKBGeom.c XKBRdBuf.c 

HOST_CPPFLAGS+=	-I${DESTDIR}${X11INCDIR} \
		-I. \
		-I${X11SRCDIR.xc}/lib/X11 \
		-I${X11SRCDIR.xc}/programs/${HOSTPROG} \
		-DDFLT_XKB_CONFIG_ROOT=\"${X11LIBDIR}/xkb\" -DXKB


.include "${NETBSDSRCDIR}/x11/tools/makekeys/Makefile.makekeys"

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
