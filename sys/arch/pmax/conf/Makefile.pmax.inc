#	$NetBSD: Makefile.pmax.inc,v 1.2 2001/04/21 05:54:02 tsutsui Exp $
ELF2ECOFF?=	elf2ecoff

POST_STRIP_SYSTEM_LD_TAIL=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
