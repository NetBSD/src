#	$NetBSD: Makefile.inc,v 1.40.2.2 2007/12/11 20:57:20 jnemeth Exp $
#

# Ross Harvey <ross@NetBSD.org>

.include <bsd.own.mk>	# So we get /etc/mk.conf vars.
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

# Whether or not to regenerate tables of contents
#
MKTOCS?=yes

M?=	${.CURDIR:T}
COMMON=	${.CURDIR}/../common

MAIN=	${COMMON}/main ${COMMON}/macros ${EXTRA}
TARG=	INSTALL
TARGS=	${TARG}.ps ${TARG}.txt ${TARG}.html ${TARG}.more
TOCS=	${TARG}.PostScript.toc ${TARG}.ASCII.toc ${TARG}.HTML.toc \
	${TARG}.more.toc
SRCS=	${MAIN} whatis ${COMMON}/contents hardware xfer prep \
	${.CURDIR}/install ${COMMON}/postinstall upgrade \
	${COMMON}/legal.common legal ${COMMON}/netboot \
	${COMMON}/sysinst \
	${.CURDIR}/../Makefile.inc ${MERGED_SRCS}

whatis: .OPTIONAL

PRESET=	${GFLAGS} -U -dM=${M} -dV=${DISTRIBVER} -d.CURDIR=${.CURDIR} -r${M}=1

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
TOC.ps=		${TOCS:M*.PostScript.*}
TOC.txt=	${TOCS:M*.ASCII.*}
TOC.html=	${TOCS:M*.HTML.*}
TOC.more=	${TOCS:M*.more.*}
.endif


${TARG}.ps: ${SRCS} ${TOC.ps} ${DISTRIBVERDEP}
	${TOOL_GROFF} ${ARGS_PS}   -mdoc      ${MAIN} > $@

${TARG}.pdf: ${TARG}.ps
	ps2pdf ${TARG}.ps $@

${TARG}.txt: ${SRCS} ${TOC.txt} ${DISTRIBVERDEP}
	${TOOL_GROFF} ${ARGS_TXT}  -mdoc      ${MAIN} > $@

${TARG}.html: ${SRCS} ${TOC.html} ${DISTRIBVERDEP}
	${TOOL_GROFF} ${ARGS_HTML} -mdoc2html ${MAIN} > $@

${TARG}.more: ${SRCS} ${TOC.more} ${DISTRIBVERDEP}
	${TOOL_GROFF} ${ARGS_MORE} -mdoc      ${MAIN} > $@


# Rules to build the table of contents (.toc) files.
# They are generated twice to take into account the
# space taken by the TOC itself.
#

TOCPROC=	   2>&1 >/dev/null |\
           sed -n '/^\.Ti/{s/ \([A-Za-z]\)/ "\1/; s/ *$$/"/; p; }'

${TARG}.PostScript.toc: ${SRCS}
	${TOOL_GROFF} -dTOC=1 ${ARGS_PS} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	${TOOL_GROFF} -dTOC=1 ${ARGS_PS} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

${TARG}.ASCII.toc: ${SRCS}
	${TOOL_GROFF} -dTOC=1 ${ARGS_TXT} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	${TOOL_GROFF} -dTOC=1 ${ARGS_TXT} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

${TARG}.HTML.toc: ${SRCS}
	${TOOL_GROFF} -dTOC=1 ${ARGS_HTML} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	${TOOL_GROFF} -dTOC=1 ${ARGS_HTML} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

${TARG}.more.toc: ${SRCS}
	${TOOL_GROFF} -dTOC=1 ${ARGS_MORE} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	${TOOL_GROFF} -dTOC=1 ${ARGS_MORE} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@


release: check_RELEASEDIR .WAIT ${TARGS}
	${INSTALL} -d -m 755 ${RELEASEDIR}/${RELEASEMACHINEDIR}
	${RELEASE_INSTALL} ${TARGS} ${RELEASEDIR}/${RELEASEMACHINEDIR}

cleannotes:
	rm -f [Ee]rrs mklog core *.core ${TARGS} ${TOCS} \
	    ${TOCS:S/.toc$/.toc.tmp/g}

clean cleandir distclean: cleannotes
