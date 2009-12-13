#	$NetBSD: Makefile,v 1.2 2009/12/13 04:57:16 nonaka Exp $

.include <bsd.own.mk>

XKBNAME=	symbols

FILES=	ad af al altwin am ara az \
	ba bd be bg br braille bt by \
	ca capslock cd ch cn compose ctrl cz \
	de dk \
	ee epo es et eurosign \
	fi fo fr \
	gb ge gh gn gr group \
	hr hu \
	ie il in inet iq ir is it \
	jp \
	keypad kg kh kpdl kr kz \
	la latam latin level3 level5 lk lt lv \
	ma mao me mk mm mn mt mv \
	nbsp ng nl no np \
	olpc \
	pc pk pl pt \
	ro rs ru \
	se shift si sk srvr_ctrl sy \
	th tj tr \
	ua us uz \
	vn \
	za

SUBDIR=		digital_vndr fujitsu_vndr hp_vndr macintosh_vndr nec_vndr \
		sgi_vndr sharp_vndr sony_vndr sun_vndr xfree68_vndr

XKBDIR=		1

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
