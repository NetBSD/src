#	$NetBSD: Makefile.sgimips.inc,v 1.8.6.1 2005/03/19 08:33:13 yamt Exp $

WANT_ECOFF?=no

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${OBJCOPY} --impure -O ecoff-bigmips \
		  -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff; \
		${OBJCOPY} --impure -O ecoff-bigmips \
		  -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff
.endif
