#	$NetBSD: Makefile.evbmips.inc,v 1.1.24.1 2005/02/06 07:45:24 jmc Exp $

CPPFLAGS+=	-I$S/dist/ipf

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true; \
	echo ${STRIP} -O srec -o $@.srec $@; \
	${STRIP} -O srec -o $@.srec $@
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
