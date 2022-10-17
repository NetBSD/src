#	$NetBSD: Makefile,v 1.6 2022/10/17 07:40:23 mrg Exp $
#
#	When upgrading, re-assemble the files base, base.lst, evdev and
#	evdev.lst by running ./configure && make in the dist directory,
#	and copy the resulting files back here.
#
#	% env PKG_CONFIG_PATH=/usr/X11R7/lib/pkgconfig meson --prefix=/usr/X11R7 build
#	% env PKG_CONFIG_PATH=/usr/X11R7/lib/pkgconfig meson compile -C build
#	% cp build/xkeyboard-config.7 src/external/mit/xorg/lib/xkeyboard-config/xkeyboard-config.man
#	% cp build/rules/{base,evdev}{,.lst} src/external/mit/xorg/lib/xkeyboard-config/rules

.include <bsd.own.mk>

XKBNAME=	rules
GENFILES=	base.extras.xml base.xml evdev.extras.xml evdev.xml
FILES=		README base base.lst evdev evdev.lst xfree98 xkb.dtd
FILES+=		${GENFILES}
CLEAN+=		${GENFILES}

.for i in ${GENFILES}
FILESBUILD_${i}=	yes
.endfor

SYMLINKS=	base ${FILESDIR}/xorg \
		base.lst ${FILESDIR}/xorg.lst \
		base.xml ${FILESDIR}/xorg.xml

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
