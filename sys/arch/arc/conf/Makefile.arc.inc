#	$NetBSD: Makefile.arc.inc,v 1.4.2.2 2002/01/08 00:23:04 nathanw Exp $

CFLAGS+=	-mips2 -mcpu=r4000
AFLAGS+=	-mips2

# XXX
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${OBJCOPY} -O ecoff-littlemips $@ $@.ecoff; \
		${OBJCOPY} -O ecoff-littlemips $@ $@.ecoff

# XXX works *only* for kernels with one config line named "netbsd"
EXTRA_CLEAN+= netbsd.ecoff
