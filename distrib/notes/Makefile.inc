#	$NetBSD: Makefile.inc,v 1.11.4.3 2002/02/23 21:02:37 he Exp $
#

# Ross Harvey <ross@netbsd.org>

.include <bsd.own.mk>	# So we get /etc/mk.conf vars.

# Whether or not to regenerate tables of contents
MKTOCS?=yes

M?=	${.CURDIR:T}
V!=	/bin/sh ${.CURDIR}/../../../sys/conf/osrelease.sh
V_S!=	/bin/sh ${.CURDIR}/../../../sys/conf/osrelease.sh -s

MAIN=	${.CURDIR}/../common/main ${.CURDIR}/../common/macros ${EXTRA}
TARG=	INSTALL
TARGS=	${TARG}.ps ${TARG}.txt ${TARG}.html ${TARG}.more
TOCS=	${TARG}.PostScript.toc ${TARG}.ASCII.toc ${TARG}.HTML.toc \
	${TARG}.more.toc
SRCS=	${MAIN} whatis contents hardware xfer prep install\
	upgrade donations legal.common legal postinstall ../Makefile.inc\
	${MERGED_SRCS}
ICMD=	${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
	-o root -g wheel -m ${NONBINMODE} 

.if defined(DESTDIR)
PRESET=	${GFLAGS} -dM=${M} -dV=${V} -d.CURDIR=${.CURDIR} -r${M}=1 -M${DESTDIR}/usr/share/tmac
.else
PRESET=	${GFLAGS} -dM=${M} -dV=${V} -d.CURDIR=${.CURDIR} -r${M}=1
.endif

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
	groff ${ARGS_PS}   -mdoc      ${MAIN} > $@

${TARG}.pdf: ${TARG}.ps
	ps2pdf ${TARG}.ps $@

${TARG}.txt: ${SRCS} ${TOCDEPS}
	groff ${ARGS_TXT}  -mdoc      ${MAIN} > $@

${TARG}.html: ${SRCS} ${TOCDEPS}
	groff ${ARGS_HTML} -mdoc2html ${MAIN} > $@

${TARG}.more: ${SRCS} ${TOCDEPS}
	groff ${ARGS_MORE} -mdoc      ${MAIN} > $@

TOCPROC=	   2>&1 >/dev/null |\
           sed -n '/^\.Ti/{s/ \([A-Za-z]\)/ "\1/; s/ *$$/"/; p; }'

${TARG}.PostScript.toc: ${SRCS}
	groff -dTOC=1 ${ARGS_PS} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	groff -dTOC=1 ${ARGS_PS} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

${TARG}.ASCII.toc: ${SRCS}
	groff -dTOC=1 ${ARGS_TXT} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	groff -dTOC=1 ${ARGS_TXT} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

${TARG}.HTML.toc: ${SRCS}
	groff -dTOC=1 ${ARGS_HTML} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	groff -dTOC=1 ${ARGS_HTML} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

${TARG}.more.toc: ${SRCS}
	groff -dTOC=1 ${ARGS_MORE} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@
	groff -dTOC=1 ${ARGS_MORE} -mdoc ${MAIN} ${TOCPROC} > $@.tmp
	mv -f $@.tmp $@

echosrcs! ${SRCS}
	@echo ${.ALLSRC}

echomore!
	@echo ${.OBJDIR}/${TARG}.more

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release:
	${ICMD} ${TARGS} ${RELEASEDIR}/.
.endif

.PATH: ${.CURDIR}/../common

cleannotes:
	rm -f [Ee]rrs mklog core *.core ${TARGS} ${TOCS}

clean cleandir distclean: cleannotes
