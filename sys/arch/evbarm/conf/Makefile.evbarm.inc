#	$NetBSD: Makefile.evbarm.inc,v 1.19.22.1 2011/06/06 09:05:24 jruoho Exp $

.if defined(BOARDMKFRAG)	# Must be a full pathname.
.include "${BOARDMKFRAG}"
.endif

EXTRA_CLEAN+= ldscript tmp ${KERNELS:=.map}

.if defined(KERNEL_BASE_PHYS)

LINKFLAGS=	-T ldscript

netbsd: ldscript             # XXX

# generate ldscript from common template
ldscript: ${THISARM}/conf/ldscript.evbarm ${THISARM}/conf/Makefile.evbarm.inc Makefile ${BOARDMKFRAG}
	echo ${KERNELS}
	${TOOL_SED} -e 's/@KERNEL_BASE_PHYS@/${KERNEL_BASE_PHYS}/' \
	    -e 's/@KERNEL_BASE_VIRT@/${KERNEL_BASE_VIRT}/' \
	    ${THISARM}/conf/ldscript.evbarm > tmp && mv tmp $@

.endif	# KERNEL_BASE_PHYS
