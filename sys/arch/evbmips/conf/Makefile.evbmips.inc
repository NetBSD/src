#	$NetBSD: Makefile.evbmips.inc,v 1.1.40.1 2005/11/29 21:22:57 yamt Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo ${STRIP} -O srec -o $@.srec $@; \
	${STRIP} -O srec -o $@.srec $@
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
