#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:07 lukem Exp $

.include <bsd.own.mk>

PROG=	xkbcomp
SRCS=	xkbcomp.c xkbscan.c expr.c vmod.c indicators.c misc.c \
	keymap.c keycodes.c keytypes.c compat.c action.c alias.c \
	symbols.c geometry.c xkbpath.c listing.c \
	xkbparse.y parseutils.c utils.c

CPPFLAGS.xkbpath.c=	-DDFLT_XKB_CONFIG_ROOT=\"${X11LIBDIR}/xkb\"

FILESDIR=${X11LIBDIR}/xkb
FILES=	README README.config README.enhancing

SYMLINKS+=${BINDIR}/xkbcomp ${X11LIBDIR}/xkb/xkbcomp

SUBDIR=	compat geometry keycodes keymap semantics symbols types compiled rules

LDADD+=	-lxkbfile -lXext -lX11
DPADD+=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
.include <bsd.x11.mk>
