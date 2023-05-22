#	from: @(#)Makefile	5.9 (Berkeley) 7/1/91
#	$NetBSD: Makefile,v 1.18 2023/05/22 12:55:44 lukem Exp $

#
# Note: Needs ps2pdf to convert .ps to .pdf
#

.include <bsd.own.mk>

.if ${MKSHARE} != "no" && ${MKMAKEMANDB} == "no"
FILES=		makewhatis.sed
FILESDIR=	/usr/share/man
.endif

SECTIONS?=	1 2 3 4 5 6 7 8 9

MANSRCDIRS=	"${.CURDIR}/../../.." ../../..

# Takes out some fluff from the PostScript files created by
# grops which does some VM saving/restoring that bloats the
# PDF file pdf2ps generates.              By uwe@NetBSD.org.
OPTIMIZE_PDF?=	\
			-e '/^\/level0 0 def/d'		\
			-e '/^\/level0 save/d'		\
			-e '/^level0 restore/d'

MAN0SRCS=
.if ${MKMAN} != "no"
MAN0SRCS+=	all.files
MAN0SRCS+=	${SECTIONS:@S@man${S}pages.files@}
MAN0SRCS+=	${SECTIONS:@S@man${S}pages.ps @}
MAN0SRCS+=	${SECTIONS:@S@man${S}pages.pdf@}
.endif

CLEANFILES+=	${MAN0SRCS}
CLEANFILES+=	*.tmp
CLEANFILES+=	toc? tocx?

all: ${MAN0SRCS}

all.files:
	${_MKTARGET_CREATE}
	find ${MANSRCDIRS} -name '*.[1-9]' -print	\
	| xargs ${TOOL_MKMAGIC}				\
	| ${TOOL_GREP} troff				\
	| ${TOOL_SED} 's/:[ 	]*.*$$//'		\
	| ${TOOL_GREP} -F -v -f "${.CURDIR}/noman"	\
	| ${TOOL_GREP} -v '#'				\
	| ${TOOL_GREP} -v '/obj\.'			\
	| ${TOOL_GREP} -v '/destdir\.'			\
	| ${TOOL_GREP} -v '/tooldir\.'			\
	> ${.TARGET}.tmp				\
	&& ${MV} ${.TARGET}.tmp ${.TARGET} ;		\
	rm -f ${.TARGET}.tmp


.for _S in ${SECTIONS}

man${_S}pages.files: all.files
	${_MKTARGET_CREATE}
	cat ${.ALLSRC}					\
	| ${TOOL_GREP} "${_S}\$$"			\
	| ${TOOL_SED} 's,\(.*/\)\([^/]*\),\2 \1\2,'	\
	| sort						\
	| ${TOOL_SED} 's/^.* //'			\
	> ${.TARGET}.tmp				\
	&& ${MV} ${.TARGET}.tmp ${.TARGET} ;		\
	rm -f ${.TARGET}.tmp

man${_S}pages.ps: man${_S}pages.files
	${_MKTARGET_CREATE}
	for f in `${TOOL_GREP} "${_S}\$$" "${.CURDIR}/intros"` `cat ${.ALLSRC}` ; \
	do						\
		if [ ! -f $${f} ]; then			\
			continue ;			\
		fi ;					\
		if ${TOOL_GREP} -q ^\\.TS "${.CURDIR}/back" ; \
		then					\
			${_MKSHMSG_FORMAT} >&2 $${f} ;	\
			cat $${f} | ${TOOL_TBL} | ${TOOL_GROFF} -Z -msafer -man -Tps ; \
		else					\
			${_MKSHMSG_FORMAT} >&2 $${f} "(tbl)";	\
			cat $${f} | ${TOOL_GROFF} -Z -msafer -man -Tps ; \
		fi ;					\
	done > ${.TARGET}.troff.tmp			\
	&& cat ${.TARGET}.troff.tmp			\
	| ${TOOL_GREP} -v '^x stop'			\
	| ${TOOL_GROPS} 				\
	> ${.TARGET}.tmp				\
	&& ${MV} ${.TARGET}.tmp ${.TARGET} ;		\
	rm -f ${.TARGET}.tmp ${.TARGET}.troff.tmp

man${_S}pages.pdf: man${_S}pages.ps
	${_MKTARGET_CREATE}
	cat ${.ALLSRC}					\
	| ${TOOL_SED} ${OPTIMIZE_PDF} 			\
	| ps2pdf - -					\
	> ${.TARGET}.tmp				\
	&& ${MV} ${.TARGET}.tmp ${.TARGET} ;		\
	rm -f ${.TARGET}.tmp

.endfor

man.tgz:
	tar plzvcf manpages-ps.tgz man?pages.ps
	tar plzvcf manpages-pdf.tgz man?pages.pdf

.include <bsd.files.mk>
