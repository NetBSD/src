#	$NetBSD: Makefile.inc,v 1.25 2002/04/26 15:50:42 lukem Exp $
#

# Ross Harvey <ross@netbsd.org>

.include <bsd.own.mk>	# So we get /etc/mk.conf vars.
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

# Whether or not to regenerate tables of contents
MKTOCS?=yes

M?=	${.CURDIR:T}

MAIN=	${.CURDIR}/../common/main ${.CURDIR}/../common/macros ${EXTRA}
TARG=	INSTALL
TARGS=	${TARG}.ps ${TARG}.txt ${TARG}.html ${TARG}.more
TOCS=	${TARG}.PostScript.toc ${TARG}.ASCII.toc ${TARG}.HTML.toc \
	${TARG}.more.toc
SRCS=	${MAIN} whatis contents hardware xfer prep install\
	upgrade donations legal.common legal postinstall ../Makefile.inc\
	${MERGED_SRCS}

PRESET=	${GFLAGS} -dM=${M} -dV=${DISTRIBVER} -d.CURDIR=${.CURDIR} -r${M}=1

POST_PLAIN= -P-b -P-u -P-o

ARGS_PS=	${PRESET} -dformat=PostScript
ARGS_TXT=	${PRESET} -dformat=ASCII ${POST_PLAIN} -Tascii -mtty-char
ARGS_HTML=	${PRESET} -dformat=HTML ${POST_PLAIN} -Tlatin1 -ww
ARGS_MORE=	${PRESET} -dformat=more -P-h -Tascii -mtty-char

#
# For example...
#
#	.if ri386 ...stuff...
#	.Ss "Install notes for NetBSD/\*[MACHINE]"
#

all: ${TARGS}

.if ${MKTOCS} != "no"
  TOCDEPS = ${TOCS}
.else
  TOCDEPS =
.endif

${TARG}.ps: ${SRCS} ${TOCDEPS}
	${GROFF} ${ARGS_PS}   -mdoc      ${MAIN} > $@

${TARG}.pdf: ${TARG}.ps
	ps2pdf ${TARG}.ps $@

${TARG}.txt: ${SRCS} ${TOCDEPS}
	${GROFF} ${ARGS_TXT}  -mdoc      ${MAIN} > $@

${TARG}.html: ${SRCS} ${TOCDEPS}
	${GROFF} ${ARGS_HTML} -mdoc2html ${MAIN} > $@

${TARG}.more: ${SRCS} ${TOCDEPS}
	${GROFF} ${ARGS_MORE} -mdoc      ${MAIN} > $@

TOCPROC=	   2>&1 >/dev/null |\
           sed -n '/^\.Ti/{s/ \([A-Za-z]\)/ "\1/; s/ *$$/"/; p; }'

${TARG}.PostScript.toc: ${SRCS}
	${GROFF} -dTOC=1 ${ARGS_PS} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	${GROFF} -dTOC=1 ${ARGS_PS} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

${TARG}.ASCII.toc: ${SRCS}
	${GROFF} -dTOC=1 ${ARGS_TXT} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	${GROFF} -dTOC=1 ${ARGS_TXT} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

${TARG}.HTML.toc: ${SRCS}
	${GROFF} -dTOC=1 ${ARGS_HTML} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	${GROFF} -dTOC=1 ${ARGS_HTML} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

${TARG}.more.toc: ${SRCS}
	${GROFF} -dTOC=1 ${ARGS_MORE} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	${GROFF} -dTOC=1 ${ARGS_MORE} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

echosrcs! ${SRCS}
	@echo ${.ALLSRC}

echomore!
	@echo ${.OBJDIR}/${TARG}.more

release: check_RELEASEDIR
	${RELINSTALL} ${TARGS} ${RELEASEDIR}/.

.PATH: ${.CURDIR}/../common

cleannotes:
	rm -f [Ee]rrs mklog core *.core ${TARGS} ${TOCS}

clean cleandir distclean: cleannotes
