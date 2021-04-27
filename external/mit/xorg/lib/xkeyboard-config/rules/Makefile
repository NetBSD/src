#	$NetBSD: Makefile,v 1.5 2021/04/27 03:58:55 mrg Exp $
#
#	When upgrading, re-assemble the files base, base.lst, evdev and
#	evdev.lst by running ./configure && make in the dist directory,
#	and copy the resulting files back here.
#
#	% env PKG_CONFIG_PATH=/usr/X11R7/lib/pkgconfig ./configure --prefix=/usr/X11R7
#	% cd man
#	% gmake xkeyboard-config.7
#	% cd ../rules
#	% ln -s /usr/pkg/bin/python3.8 ./python3
#	% env PATH=${PATH}:. PYTHON=python3.8 gmake base base.lst evdev evdev.lst

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
