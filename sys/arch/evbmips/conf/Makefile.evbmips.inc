#	$NetBSD: Makefile.evbmips.inc,v 1.1.10.2 2002/06/23 17:35:47 jdolecek Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true; \
	echo ${STRIP} -O srec -o $@.srec $@; \
	${STRIP} -O srec -o $@.srec $@
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
