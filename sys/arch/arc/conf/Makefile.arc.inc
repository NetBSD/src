#	$NetBSD: Makefile.arc.inc,v 1.3.2.2 2002/02/11 20:07:14 jdolecek Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2

# XXX
SYSTEM_LD_TAIL_EXTRA=; \
	echo ${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident $@ $@.ecoff; \
	${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident $@ $@.ecoff

# XXX works *only* for kernels with one config line named "netbsd"
EXTRA_CLEAN+= netbsd.ecoff
