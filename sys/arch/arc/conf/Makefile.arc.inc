#	$NetBSD: Makefile.arc.inc,v 1.3.2.3 2002/06/23 17:34:39 jdolecek Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2

# XXX
SYSTEM_LD_TAIL_EXTRA=; \
	echo ${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident $@ $@.ecoff; \
	${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident $@ $@.ecoff

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ecoff@}
