#	$NetBSD: Makefile.arc.inc,v 1.10.2.4 2004/09/21 13:12:59 skrll Exp $

CFLAGS+=	-mips2
AFLAGS+=	-mips2

LINKFORMAT=	-N

# XXX
SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true; \
	echo ${OBJCOPY} -O ecoff-littlemips \
	    -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff; \
	${OBJCOPY} -O ecoff-littlemips \
	    -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ecoff@}
