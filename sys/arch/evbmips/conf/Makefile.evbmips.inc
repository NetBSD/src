#	$NetBSD: Makefile.evbmips.inc,v 1.1.12.2 2002/03/07 14:43:58 simonb Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true; \
	echo ${STRIP} -O srec -o $@.srec $@; \
	${STRIP} -O srec -o $@.srec $@
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
