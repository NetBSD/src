#	$NetBSD: Makefile,v 1.2.2.1 2004/04/29 05:46:53 jmc Exp $

.include <bsd.own.mk>

FILES=	C armscii-8 en_US.UTF-8 georgian-academy georgian-ps \
	ibm-cp1133 iscii-dev isiri-3342 iso8859-1 iso8859-2 iso8859-3 \
	iso8859-4 iso8859-5 iso8859-6 iso8859-7 iso8859-8 iso8859-9 \
	iso8859-9e iso8859-10 iso8859-11 iso8859-13 iso8859-14 \
	iso8859-15 ja ja.JIS ja.SJIS ja_JP.UTF-8 ko ko_KR.UTF-8 \
	koi8-c koi8-r koi8-u microsoft-cp1251 microsoft-cp1255 \
	microsoft-cp1256 mulelao-1 nokhchi-1 tatar-cyr th_TH \
	th_TH.UTF-8 tscii-0 vi_VN.tcvn vi_VN.viscii \
	zh_CN zh_CN.UTF-8 zh_CN.gbk \
	zh_HK.big5hkscs zh_TW zh_TW.UTF-8 zh_TW.big5

FILESNAME=	XI18N_OBJS

.for _F in ${FILES}
FILESDIR_${_F}=	${X11LIBDIR}/locale/${_F}
.endfor

.PATH: ${X11SRCDIR.xc}/nls/XI18N_OBJS

.include <bsd.x11.mk>
.include <bsd.prog.mk>
