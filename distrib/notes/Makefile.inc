#	$NetBSD: Makefile.inc,v 1.3 1999/02/13 23:53:56 lukem Exp $

# Ross Harvey <ross@netbsd.org>

M?=	${.CURDIR:T}
V!=	/bin/sh ${.CURDIR}/../../../sys/conf/osrelease.sh
V_S!=	/bin/sh ${.CURDIR}/../../../sys/conf/osrelease.sh -s

MAIN=	${.CURDIR}/../common/main ${EXTRA}
TARG=	INSTALL
TARGS=	${TARG}.ps ${TARG}.txt ${TARG}.html ${TARG}.more
SRCS=	${MAIN} mirrors whatis contents hardware xfer prep install\
	upgrade donations legal.common legal postinstall ../Makefile.inc\
	${MERGED_SRCS}

PRESET=	${GFLAGS} -dV=$V -dV_S=${V_S} -dMACHINE=$M -d.CURDIR=${.CURDIR} -r$M=1 

POST_PLAIN= -P-b -P-u -P-o -Tascii

ARGS_PS=	${PRESET} -dformat=PostScript
ARGS_TXT=	${PRESET} -dformat=ASCII ${POST_PLAIN}
ARGS_HTML=	${PRESET} -dformat=HTML  ${POST_PLAIN} -ww
ARGS_MORE=	${PRESET} -dformat=more -P-h -Tascii

#
# For example...
#
#	.if ri386 ...stuff...
#	.Ss "Install notes for NetBSD/\*[MACHINE]"
#

CLEANFILES+=${TARGS}

.for i in ps txt html more
all: ${TARG}.$i
.endfor

${TARG}.ps: ${SRCS}
	groff ${ARGS_PS}   -mdoc      ${MAIN} > $@

${TARG}.txt: ${SRCS}
	groff ${ARGS_TXT}  -mdoc      ${MAIN} > $@

${TARG}.html: ${SRCS}
	groff ${ARGS_HTML} -mdoc2html ${MAIN} > $@

${TARG}.more: ${SRCS}
	groff ${ARGS_MORE} -mdoc      ${MAIN} > $@

echosrcs! ${SRCS}
	@echo ${.ALLSRC}

.PATH: ${.CURDIR}/../common
