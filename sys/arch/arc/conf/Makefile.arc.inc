#	$NetBSD: Makefile.arc.inc,v 1.10 2003/06/14 09:55:10 tsutsui Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2

LINKFORMAT=	-N

# XXX
SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true; \
	echo ${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident -R .pdr \
	$@ $@.ecoff; \
	${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident -R .pdr $@ $@.ecoff

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ecoff@}
