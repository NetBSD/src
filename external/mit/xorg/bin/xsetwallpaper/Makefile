#	$NetBSD: Makefile,v 1.2.40.1 2021/06/02 14:46:24 cjep Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=		xsetwallpaper
SRCS=		xsetwallpaper.c
SRCS+=		stb_image.c

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys -I${NETBSDSRCDIR}/sys/dev/stbi

SRCPATH=        ${X11SRCDIR.local}/programs/xsetwallpaper

LDADD+=		-lm ${XLIBLDADD_X11}
DPADD+=		${LIBM} ${XLIBDPADD_X11}

.PATH:		${X11SRCDIR.local}/programs/xsetwallpaper
.PATH:		${NETBSDSRCDIR}/sys/dev/stbi

WARNS=		4

CPPFLAGS.stb_image.c+=	-Wno-shadow
CPPFLAGS.stb_image.c+=	-Wno-sign-compare
CPPFLAGS.stb_image.c+=	-Wno-missing-prototypes
CPPFLAGS.stb_image.c+=	-Wno-missing-field-initializers

CWARNFLAGS.clang+=	-Wno-missing-noreturn -Wno-self-assign

.include <bsd.x11.mk>
.include <bsd.prog.mk>
