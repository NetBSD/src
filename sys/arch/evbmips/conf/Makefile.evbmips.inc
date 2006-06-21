#	$NetBSD: Makefile.evbmips.inc,v 1.1.34.1 2006/06/21 14:51:02 yamt Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo ${STRIP} -O srec -o $@.srec $@; \
	${STRIP} -O srec -o $@.srec $@
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
