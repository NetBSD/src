#	$NetBSD: Makefile,v 1.8.8.1 1999/06/21 00:50:02 thorpej Exp $

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
