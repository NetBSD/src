#	$NetBSD: Makefile.evbmips.inc,v 1.1.18.1 2005/12/11 10:28:16 christos Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo ${STRIP} -O srec -o $@.srec $@; \
	${STRIP} -O srec -o $@.srec $@
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
