#	$NetBSD: Makefile.arc.inc,v 1.11.2.1.2.1 2005/02/06 07:45:24 jmc Exp $

CFLAGS+=	-mips2
AFLAGS+=	-mips2
CFLAGS+=	-I$S/dist/ipf

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
