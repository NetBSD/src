#	$NetBSD: Makefile.pmax.inc,v 1.5.16.1 2005/02/06 07:45:25 jmc Exp $

CPPFLAGS+=	-I$S/dist/ipf

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
