#	$NetBSD: Makefile.arc.inc,v 1.7 2002/03/19 00:57:53 atatat Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2

# XXX
SYSTEM_LD_TAIL_EXTRA=; \
	echo ${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident $@ $@.ecoff; \
	${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident $@ $@.ecoff

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ecoff@}
