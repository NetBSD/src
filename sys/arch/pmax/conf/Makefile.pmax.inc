#	$NetBSD: Makefile.pmax.inc,v 1.1.2.3 2001/04/23 09:42:00 bouyer Exp $
ELF2ECOFF?=	elf2ecoff

POST_STRIP_SYSTEM_LD_TAIL=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
