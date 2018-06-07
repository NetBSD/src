#	$NetBSD: Makefile,v 1.3.38.1 2018/06/07 17:53:44 martin Exp $
#
#	When upgrading, re-assemble the files base, base.lst, evdev and
#	evdev.lst by running ./configure && make in the dist directory,
#	and copy the resulting files back here.

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
