#	from: @(#)Makefile	5.9 (Berkeley) 7/1/91
#	$NetBSD: Makefile,v 1.17 2012/02/07 19:13:31 joerg Exp $

NOOBJ=		# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no" && ${MKMAKEMANDB} == "no"
FILES=		makewhatis.sed
FILESDIR=	/usr/share/man
.endif

CLEANFILES+=	man?
CLEANFILES+=	man?pages.tmp
CLEANFILES+=	man?pages.ps
CLEANFILES+=	man?pages.pdf
CLEANFILES+=	toc? tocx?

SECTIONS?=	1 2 3 4 5 6 7 8 9

# Takes out some fluff from the PostScript files created by
# grops which does some VM saving/restoring that bloats the
# PDF file pdf2ps generates.              By uwe@NetBSD.org.
OPTIMIZE_PDF?=	\
			-e '/^\/level0 0 def/d' 		\
			-e '/^\/level0 save/d' 			\
			-e '/^level0 restore/d' 		

all: man man.ps man.pdf

man: 
	${HOST_SH} mkman

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
				cat $${f} | tbl | troff -msafer -man -Tps ; \
			else 					\
				echo >&2 $${f} "(tbl)" ;	\
				cat $${f} | troff -msafer -man -Tps ; \
			fi ; 					\
		done >man$${i}pages.tmp ;			\
		cat man$${i}pages.tmp				\
		| grep -v '^x stop' 				\
		| grops >man$${i}pages.ps ;			\
		rm man$${i}pages.tmp ; 		\
	done

man.pdf: 
	for i in ${SECTIONS} ; 					\
	do							\
		echo Chewing on man$${i}pages.pdf ;		\
		${TOOL_SED} 					\
			${OPTIMIZE_PDF} 			\
			<man$${i}pages.ps >man$${i}pages.ps.tmp ; \
		ps2pdf man$${i}pages.ps.tmp man$${i}pages.pdf ;	\
		rm man$${i}pages.ps.tmp ; 			\
	done

man.tgz:
	tar plzvcf manpages-ps.tgz man?pages.ps
	tar plzvcf manpages-pdf.tgz man?pages.pdf

.include <bsd.prog.mk>
