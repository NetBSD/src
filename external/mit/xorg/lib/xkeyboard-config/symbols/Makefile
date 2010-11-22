#	$NetBSD: Makefile,v 1.4 2010/11/22 00:27:56 mrg Exp $

.include <bsd.own.mk>

XKBNAME=	symbols

FILES=	ad af al altwin am ara at az \
	ba bd be bg br brai bt bw by \
	ca capslock cd ch cn compose ctrl cz \
	de dk \
	ee empty epo es et eurosign \
	fi fo fr \
	gb ge gh gn gr group \
	hr hu \
	ie il in inet iq ir is it \
	jp \
	ke keypad kg kh kpdl kr kz \
	la latam latin level3 level5 lk lt lv \
	ma mao me mk ml mm mn mt mv \
	nbsp ng nl no np \
	olpc \
	pc pk pl pt \
	ro rs ru \
	se shift si sk sn srvr_ctrl sy \
	terminate th tj tm tr typo tz \
	ua us uz \
	vn \
	za

SUBDIR=		digital_vndr extras fujitsu_vndr hp_vndr macintosh_vndr \
		nec_vndr nokia_vndr sgi_vndr sharp_vndr sony_vndr sun_vndr \
		xfree68_vndr

XKBDIR=		1

.include "../Makefile.xkbdata"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
