#	$NetBSD: Makefile.sgimips.inc,v 1.10 2005/12/11 12:18:52 christos Exp $

WANT_ECOFF?=no

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${OBJCOPY} --impure -O ecoff-bigmips \
		  -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff; \
		${OBJCOPY} --impure -O ecoff-bigmips \
		  -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff
.endif
