#	$NetBSD: Makefile,v 1.1.1.1.10.1 2009/05/13 18:53:24 jym Exp $

.include <bsd.own.mk>

PROG=	xkbcomp
SRCS=	xkbcomp.c xkbscan.c expr.c vmod.c indicators.c misc.c \
	keymap.c keycodes.c keytypes.c compat.c action.c alias.c \
	symbols.c geometry.c xkbpath.c listing.c \
	xkbparse.y parseutils.c utils.c

XKBCOMPDIR=		/var/db/xkb
CPPFLAGS.xkbpath.c=	-DDFLT_XKB_CONFIG_ROOT=\"${XKBCOMPDIR}\"

CPPFLAGS+=		-I${X11SRCDIR.${PROG}}
CPPFLAGS.xkbpath.c=	-DDFLT_XKB_CONFIG_ROOT=\"${X11LIBDIR}/xkb\"
CPPFLAGS.geometry.c=	-Wno-error	# XXX

FILESDIR=${X11LIBDIR}/xkb
FILES=	README README.config README.enhancing

SYMLINKS+=${BINDIR}/xkbcomp ${X11LIBDIR}/xkb/xkbcomp

LDADD+=	-lxkbfile -lXext -lX11
DPADD+=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
