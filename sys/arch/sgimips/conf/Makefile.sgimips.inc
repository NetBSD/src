#	$NetBSD: Makefile.sgimips.inc,v 1.9 2005/02/28 23:44:58 sekiya Exp $

WANT_ECOFF?=no

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${OBJCOPY} --impure -O ecoff-bigmips \
		  -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff; \
		${OBJCOPY} --impure -O ecoff-bigmips \
		  -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff
.endif
