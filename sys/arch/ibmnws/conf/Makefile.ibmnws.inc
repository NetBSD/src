#	$NetBSD: Makefile.ibmnws.inc,v 1.1 2014/05/08 13:58:34 phx Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo "Convert kernel $@ into NCD firmware image."; \
		${TOOL_NCDCS} $@ $@.ncd; \
		mv $@.ncd $@
