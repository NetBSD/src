#	from: @(#)Makefile	5.9 (Berkeley) 7/1/91
#	$NetBSD: Makefile,v 1.5 2001/06/26 02:26:06 hubertf Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		makewhatis.sed COPYRIGHT
FILESDIR=	/usr/share/man
.endif
MKOBJ=		no

CLEANFILES+=	man?
CLEANFILES+=	man?pages.tmp
CLEANFILES+=	man?pages.ps
CLEANFILES+=	toc? tocx?

man:
	sh mkman

man.ps:
	for i in 1 2 3 4 5 6 7 8 9 ; 				\
	do							\
		for f in `cat man$${i}` ; 			\
		do						\
			echo >&2 $${f} ; 			\
			troff -msafer -man -Tps $${f} ;		\
		done >man$${i}pages.tmp ;			\
		grops <man$${i}pages.tmp >man$${i}pages.ps ;	\
		rm man$${i}pages.tmp ; 				\
	done

.include <bsd.prog.mk>
