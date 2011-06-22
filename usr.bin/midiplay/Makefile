#	$NetBSD: Makefile,v 1.4 2011/06/22 02:49:44 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	midiplay

CWARNFLAGS.clang+=	-Wno-format-extra-args -Wno-initializer-overrides

.include <bsd.prog.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.midiplay.c+=	-Wno-error
.endif
