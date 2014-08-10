#	$NetBSD: Makefile.ibmnws.inc,v 1.1.6.2 2014/08/10 06:54:00 tls Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo "Convert kernel $@ into NCD firmware image."; \
		${TOOL_NCDCS} $@ $@.ncd; \
		mv $@.ncd $@
