#	$NetBSD: Makefile,v 1.6 2008/05/21 17:09:29 ghen Exp $

.include <bsd.own.mk>

XKBNAME=	symbols

FILES=	README \
	al altwin am ar az \
	be ben bg br bs by \
	ca ca_enhanced colemak compose ctrl \
	czsk cz cz_qwerty \
	de de_CH dev dk dvorak \
	ee el en_US es \
	fi fo fr fr_CH \
	gb ge_la ge_ru group guj gur \
	hr hr_US hu hu_US hu_qwerty \
	ie il il_phonetic iso9995-3 inet ir is it iu \
	jp \
	kan keypad ky \
	la lat level3 lo lock lt lt_std lv \
	ml mk mm mt mt_us \
	nl no \
	ogham ori \
	pc104 pl pl2 pt \
	ralt ro ro2 ru \
	sapmi se se_FI se_NO se_SE si sk sk_qwerty sr srvr_ctrl \
	syr syr_phonetic \
	tel th th_tis th_pat tj tml tr tr_f \
	ua us us_intl us_group2 us_group3 uz \
	vn \
	yu \
	apple

SUBDIR=		digital fujitsu hp macintosh nec sgi sharp sony sun xfree68 pc

XKBDIR=		1

.include "../Makefile.xkbcomp"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
