#	$NetBSD: Makefile,v 1.4.4.1 2004/11/15 09:12:45 rtr Exp $

.include <bsd.own.mk>

XKBNAME=	symbols

FILES=	README \
	al altwin am ar az \
	be ben bg br bs by \
	ca ca_enhanced compose ctrl \
	czsk cz cz_qwerty \
	de de_CH dev dk dvorak \
	ee el en_US es \
	fi fr fr_CH \
	gb ge_la ge_ru group guj gur \
	hr hr_US hu hu_US hu_qwerty \
	ie il il_phonetic iso9995-3 inet ir is it iu \
	jp \
	kan keypad \
	la level3 lo lock lt lt_std lv \
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

SUBDIR=		digital fujitsu hp macintosh nec sgi sony sun xfree68 pc
# not in x.org SUBDIR+=	sharp

XKBDIR=		1

.include "../Makefile.xkbcomp"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
