#	from: @(#)Makefile	5.9 (Berkeley) 7/1/91
#	$NetBSD: Makefile,v 1.8 2001/07/05 20:52:11 hubertf Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		makewhatis.sed COPYRIGHT
FILESDIR=	/usr/share/man
.endif
MKOBJ=		no

CLEANFILES+=	man?
CLEANFILES+=	man?pages.tmp
CLEANFILES+=	man?pages.ps
CLEANFILES+=	man?pages.pdf
CLEANFILES+=	toc? tocx?

SECTIONS?=	1 2 3 4 5 6 7 8 9

# Takes out some fluff from the PostScript files created bye
# grops which does some VM saving/restoring that bloats the
# PDF file pdf2ps generates.              By uwe@netbsd.org.
OPTIMIZE_PDF?=	\
			-e '/^\/level0 0 def/d' 		\
			-e '/^\/level0 save/d' 			\
			-e '/^level0 restore/d' 		

man:
	sh mkman

man.ps:
	for i in ${SECTIONS} ; 					\
	do							\
		for f in `grep "$${i}\$$" intros` `cat man$${i}` ; \
		do						\
			if [ ! -f $${f} ]; then 		\
				continue ; 			\
			fi ;					\
			if grep -q ^\\.TS back ;		\
			then					\
				echo >&2 $${f} ; 			\
				cat $${f} | tbl | troff -msafer -man -Tps $${f} ; \
			else 					\
				echo >&2 $${f} "(tbl)" ;	\
				cat $${f} | troff -msafer -man -Tps $${f} ; \
			fi ; 					\
		done >man$${i}pages.tmp ;			\
		grops <man$${i}pages.tmp >man$${i}pages.ps ;	\
		rm man$${i}pages.tmp ; 		\
	done

man.pdf:
	for i in ${SECTIONS} ; 					\
	do							\
		echo Chewing on man$${i}pages.pdf ;		\
		sed 						\
			${OPTIMIZE_PDF} 			\
			<man$${i}pages.ps >man$${i}pages.ps.tmp ; \
		ps2pdf man$${i}pages.ps.tmp man$${i}pages.pdf ;	\
		rm man$${i}pages.ps.tmp ; 			\
	done

man.tgz:
	tar plzvcf manpages-ps.tgz man?pages.ps
	tar plzvcf manpages-pdf.tgz man?pages.pdf

.include <bsd.prog.mk>
