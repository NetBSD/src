#	$NetBSD: Makefile.pmax.inc,v 1.1 2000/12/03 07:39:34 simonb Exp $

POST_STRIP_SYSTEM_LD_TAIL=; \
		echo elf2ecoff $@ $@.ecoff; elf2ecoff $@ $@.ecoff; \
		chmod 755 $@.ecoff
