#	$NetBSD: Makefile.pmax.inc,v 1.1.2.2 2000/12/08 09:30:14 bouyer Exp $

POST_STRIP_SYSTEM_LD_TAIL=; \
		echo elf2ecoff $@ $@.ecoff; elf2ecoff $@ $@.ecoff; \
		chmod 755 $@.ecoff
