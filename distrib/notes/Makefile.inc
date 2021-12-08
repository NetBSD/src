#	$NetBSD: Makefile.inc,v 1.57 2021/12/08 13:06:44 uwe Exp $
#

# Ross Harvey <ross@NetBSD.org>

.include <bsd.own.mk>	# So we get /etc/mk.conf vars.
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.if ${MKREPRO_TIMESTAMP:Uno} != "no"
GROFF_TIMESTAMP=--timestamp=${MKREPRO_TIMESTAMP}
DATE_TIMESTAMP=-r ${MKREPRO_TIMESTAMP}
.endif

# Whether or not to regenerate tables of contents
#
MKTOCS?=yes

M?=	${.CURDIR:T}
COMMON=	${.CURDIR}/../common

MAIN=	${COMMON}/main ${EXTRA}
TARG=	INSTALL
TARGS=	${TARG}.ps ${TARG}.txt ${TARG}.html ${TARG}.more
TOCS=	${TARG}.PostScript.toc ${TARG}.ASCII.toc ${TARG}.HTML.toc \
	${TARG}.more.toc
SRCS=	${MAIN} ${COMMON}/macros \
	whatis ${COMMON}/contents hardware xfer prep \
	${.CURDIR}/install ${COMMON}/postinstall upgrade \
	${COMMON}/legal.common ${COMMON}/netboot \
	${COMMON}/sysinst \
	${.CURDIR}/../Makefile.inc ${MERGED_SRCS}

whatis: .OPTIONAL

PRESET=	${GFLAGS} -U -dM=${M} -dV=${DISTRIBVER} -d.CURDIR=${.CURDIR} -r${M}=1
PRESET+=	-rmajor=${DISTRIBVER:C/\..*$//}
.if ${USE_XZ_SETS:Uno} == "yes"
PRESET+=	-dsetsuffix=tar.xz
PRESET+=	-dcompressor=xz
.else
PRESET+=	-dsetsuffix=tgz
PRESET+=	-dcompressor=gzip
.endif
.if ${NETBSD_OFFICIAL_RELEASE:Uno} == "yes"
PRESET+=	-rFOR_RELEASE=1
.else
PRESET+=	-rFOR_RELEASE=0
.endif
.if (${DISTRIBVER:M*.99.*})
PRESET+=	-rRELEASE_BRANCH=0
# make .Nx \*V happy
PRESET+=	-doperating-system-NetBSD-${DISTRIBVER}=${DISTRIBVER}
.elif (${DISTRIBVER:M*.[0-9]*})
PRESET+=	-rRELEASE_BRANCH=1
PRESET+=	-rminor=${DISTRIBVER:C/^[0-9]+\.([0-9])+.*$/\1/}
nextmin!=	expr ${DISTRIBVER:C/^[0-9]+\.([0-9])+.*$/\1/} + 1
.if exists(${.CURDIR}/../../../doc/CHANGES-${DISTRIBVER:C/\..*$//}.${nextmin})
PRESET+=	-rnextminor=${nextmin}
.endif
.endif
.if defined(BUILDID) && "${BUILDID:M20*Z}" != ""
curdate!=	env LANG="C" date -d "${BUILDID:C/[0-9][0-9][0-9][0-9]Z$//}" "+%b %d, %Y"
.else
.if ${NETBSD_OFFICIAL_RELEASE:Uno} == "yes"
.error "No release date could be derived from BUILDID"
.else
curdate!=	env LANG="C" date ${DATE_TIMESTAMP} "+%b %d, %Y"
.endif
.endif
PRESET+=	-dcur_date="${curdate}"

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

REMOVE_CREATION_DATE= ${TOOL_SED} -e '/^%%CreationDate:/d'

${TARG}.ps: ${SRCS} ${TOC.ps} ${DISTRIBVERDEP}
	${TOOL_GROFF} ${ARGS_PS} ${GROFF_TIMESTAMP} -mdoc      ${MAIN} \
	    ${${MKREPRO:Uno} == "yes":? | ${REMOVE_CREATION_DATE} :} \
	    > $@

${TARG}.pdf: ${TARG}.ps
	ps2pdf ${TARG}.ps $@

${TARG}.txt: ${SRCS} ${TOC.txt} ${DISTRIBVERDEP}
	${TOOL_GROFF} ${ARGS_TXT} ${GROFF_TIMESTAMP}  -mdoc      ${MAIN} > $@

${TARG}.html: ${SRCS} ${TOC.html} ${DISTRIBVERDEP}
	${TOOL_GROFF} ${ARGS_HTML} ${GROFF_TIMESTAMP} -mdoc2html ${MAIN} > $@

${TARG}.more: ${SRCS} ${TOC.more} ${DISTRIBVERDEP}
	${TOOL_GROFF} ${ARGS_MORE} ${GROFF_TIMESTAMP} -mdoc      ${MAIN} > $@


# Rules to build the table of contents (.toc) files.  For the
# PostScript version it's generated twice to take into account the
# space taken by the TOC itself.  Other versions are not paginated.

${TARG}.PostScript.toc: ${SRCS}
	${TOOL_GROFF} -dTOC=1 ${ARGS_PS} ${GROFF_TIMESTAMP} -mdoc ${MAIN} > /dev/null
	mv -f $@.tmp $@
	${TOOL_GROFF} -dTOC=1 ${ARGS_PS} ${GROFF_TIMESTAMP} -mdoc ${MAIN} > /dev/null
	mv -f $@.tmp $@

${TARG}.ASCII.toc: ${SRCS}
	${TOOL_GROFF} -dTOC=1 ${ARGS_TXT} ${GROFF_TIMESTAMP} -mdoc ${MAIN} > /dev/null
	mv -f $@.tmp $@

${TARG}.HTML.toc: ${SRCS}
	${TOOL_GROFF} -dTOC=1 ${ARGS_HTML} ${GROFF_TIMESTAMP} -mdoc2html ${MAIN} > /dev/null
	mv -f $@.tmp $@

${TARG}.more.toc: ${SRCS}
	${TOOL_GROFF} -dTOC=1 ${ARGS_MORE} ${GROFF_TIMESTAMP} -mdoc ${MAIN} > /dev/null
	mv -f $@.tmp $@


release: check_RELEASEDIR .WAIT ${TARGS}
	${INSTALL} -d -m 755 ${RELEASEDIR}/${RELEASEMACHINEDIR}
	${RELEASE_INSTALL} ${TARGS} ${RELEASEDIR}/${RELEASEMACHINEDIR}

cleannotes:
	rm -f [Ee]rrs mklog core *.core ${TARGS} ${TOCS} \
	    ${TOCS:S/.toc$/.toc.tmp/g}

clean cleandir distclean: cleannotes
