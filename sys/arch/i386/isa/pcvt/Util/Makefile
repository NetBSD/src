#	$NetBSD: Makefile,v 1.9 1999/06/04 18:47:33 thorpej Exp $

#SUBDIR=keycap
#SUBDIR+=cursor
SUBDIR+=fontedit
SUBDIR+=fonts
#SUBDIR+=kcon
#SUBDIR+=loadfont
#SUBDIR+=scon
#SUBDIR+=userkeys
SUBDIR+=vttest
#SUBDIR+=ispcvt
#SUBDIR+=mcon

.if make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=	vgaio kbdio set2061
SUBDIR+=	demo
.endif

.include <bsd.subdir.mk>
