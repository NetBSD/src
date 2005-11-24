#	$NetBSD: Makefile.evbmips.inc,v 1.2 2005/11/24 12:54:29 dbj Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo ${STRIP} -O srec -o $@.srec $@; \
	${STRIP} -O srec -o $@.srec $@
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
