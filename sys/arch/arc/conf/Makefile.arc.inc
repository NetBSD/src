#	$NetBSD: Makefile.arc.inc,v 1.11 2003/10/26 11:27:54 tsutsui Exp $

CFLAGS+=	-mips2
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
