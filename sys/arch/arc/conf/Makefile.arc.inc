#	$NetBSD: Makefile.arc.inc,v 1.4.2.3 2002/02/28 04:07:12 nathanw Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2

# XXX
SYSTEM_LD_TAIL_EXTRA=; \
	echo ${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident $@ $@.ecoff; \
	${OBJCOPY} -O ecoff-littlemips -R .mdebug -R .ident $@ $@.ecoff

# XXX works *only* for kernels with one config line named "netbsd"
EXTRA_CLEAN+= netbsd.ecoff
