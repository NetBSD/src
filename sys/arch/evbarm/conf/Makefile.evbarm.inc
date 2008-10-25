#	$NetBSD: Makefile.evbarm.inc,v 1.19 2008/10/25 22:27:37 apb Exp $

.if defined(BOARDMKFRAG)	# Must be a full pathname.
.include "${BOARDMKFRAG}"
.endif

.if defined(KERNEL_BASE_PHYS)

LINKFLAGS=	-T ldscript

netbsd: ldscript             # XXX
EXTRA_CLEAN+= ldscript tmp

# generate ldscript from common template 
ldscript: ${THISARM}/conf/ldscript.evbarm ${THISARM}/conf/Makefile.evbarm.inc Makefile ${BOARDMKFRAG}
	echo ${KERNELS}
	${TOOL_SED} -e 's/@KERNEL_BASE_PHYS@/${KERNEL_BASE_PHYS}/' \
	    -e 's/@KERNEL_BASE_VIRT@/${KERNEL_BASE_VIRT}/' \
	    ${THISARM}/conf/ldscript.evbarm > tmp && mv tmp $@

.endif	# KERNEL_BASE_PHYS
