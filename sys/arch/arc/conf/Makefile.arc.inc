#	$NetBSD: Makefile.arc.inc,v 1.8 2002/12/07 21:35:58 tsutsui Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2

# XXX
SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true; \
	echo ${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident $@ $@.ecoff; \
	${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident $@ $@.ecoff

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ecoff@}
