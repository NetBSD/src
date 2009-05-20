#	$NetBSD: Makefile,v 1.6 2009/05/20 03:36:36 dyoung Exp $

# Makefile for xen tags file

.include "../../kern/Make.tags.inc"

.ifmake tags
.include "${SYSDIR}/arch/xen/conf/Makefile.arch.inc"
.endif

TXEN=	${SYSDIR}/arch/xen/tags

SXEN!=	${TOOL_AWK:Uawk} '($$2 ~ /^arch.*\.[c]$$/) {print "${SYSDIR}/" $$2}' \
		${SYSDIR}/arch/xen/conf/files.xen
AXEN!=	${TOOL_AWK:Uawk} '($$2 ~ /^arch.*\.[sS]$$/) {print "${SYSDIR}/" $$2}' \
		${SYSDIR}/arch/xen/conf/files.xen
AXEN+=	${SYSDIR}/arch/xen/${XEN_BUILD}/*.[sS]

# Directories in which to place tags links
DXEN=	xen ${XEN_MACHINE_ARCHS} include

tags:
	-rm -f ${TXEN}
	-echo ${SXEN} | xargs ctags -wadtf ${TXEN}
	${FINDCOMM} | xargs ctags -wadtf ${TXEN}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AXEN} | \
	    ${TOOL_SED:Used} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TXEN}
	sort -o ${TXEN} ${TXEN}

links:
	-for i in ${DXEN}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include

.include <bsd.subdir.mk>
