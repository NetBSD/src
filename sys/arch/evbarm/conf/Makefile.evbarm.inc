#	$NetBSD: Makefile.evbarm.inc,v 1.16.8.1 2005/02/06 07:45:24 jmc Exp $

CPPFLAGS+=	-I$S/dist/ipf

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

.if defined(BOARDMKFRAG)	# Must be a full pathname.
.include "${BOARDMKFRAG}"
.endif

.if defined(KERNEL_BASE_PHYS)

LINKFLAGS=	-T ldscript

netbsd: ldscript             # XXX
EXTRA_CLEAN+= ldscript tmp

# generate ldscript from common template 
ldscript: ${THISARM}/conf/ldscript.evbarm ${THISARM}/conf/Makefile.evbarm.inc
	echo ${KERNELS}
	sed -e 's/@KERNEL_BASE_PHYS@/${KERNEL_BASE_PHYS}/' \
	    -e 's/@KERNEL_BASE_VIRT@/${KERNEL_BASE_VIRT}/' \
	    ${THISARM}/conf/ldscript.evbarm > tmp && mv tmp $@

.endif	# KERNEL_BASE_PHYS
