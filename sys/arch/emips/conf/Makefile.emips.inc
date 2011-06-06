#	$NetBSD: Makefile.emips.inc,v 1.1.8.2 2011/06/06 09:05:15 jruoho Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
