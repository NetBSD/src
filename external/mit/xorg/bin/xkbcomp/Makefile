#	$NetBSD: Makefile,v 1.1.1.1.8.2 2009/09/28 01:57:05 snj Exp $

.include <bsd.own.mk>
.include "Makefile.common"

PROG=	xkbcomp
SRCS=	xkbcomp.c xkbscan.c expr.c vmod.c indicators.c misc.c \
	keymap.c keycodes.c keytypes.c compat.c action.c alias.c \
	symbols.c geometry.c xkbpath.c listing.c \
	xkbparse.y parseutils.c utils.c

CPPFLAGS.xkbpath.c=	-DDFLT_XKB_CONFIG_ROOT=\"${XKBCOMPDIR}\"

CPPFLAGS+=		-I${X11SRCDIR.${PROG}} -I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H
CPPFLAGS.geometry.c=	-Wno-error	# XXX

FILESDIR=${X11LIBDIR}/xkb
FILES=	README README.config README.enhancing

SYMLINKS+=${BINDIR}/xkbcomp ${X11LIBDIR}/xkb/xkbcomp

LDADD+=	-lxkbfile -lXext -lX11
DPADD+=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
