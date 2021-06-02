#	$NetBSD: Makefile,v 1.11.4.1 2021/06/02 14:46:21 cjep Exp $

.include <bsd.own.mk>
.include "Makefile.common"

PROG=	xkbcomp
SRCS=	xkbcomp.c xkbscan.c expr.c vmod.c indicators.c misc.c \
	keymap.c keycodes.c keytypes.c compat.c action.c alias.c \
	symbols.c geometry.c xkbpath.c listing.c \
	xkbparse.y parseutils.c utils.c

CPPFLAGS.xkbpath.c=	-DDFLT_XKB_CONFIG_ROOT=\"${XKBROOTDIR}\"

CPPFLAGS+=		-I${X11SRCDIR.${PROG}}
CPPFLAGS+=		-DHAVE_STRDUP -DHAVE_STRCASECMP
CPPFLAGS.geometry.c=	-Wno-error	# XXX
COPTS.xkbscan.c+=	-Wno-stack-protector

FILESDIR=${XKBROOTDIR}
FILES=	README

SYMLINKS+=	${BINDIR}/xkbcomp ${XKBROOTDIR}/xkbcomp
SYMLINKS+=	${XKBDATADIR} ${XKBCOMPDIR}

LDADD+=	${XLIBLDADD_KDFILE}
DPADD+=	${XLIBDPADD_KDFILE}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

CWARNFLAGS.clang+=	-Wno-parentheses -Wno-string-compare

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
