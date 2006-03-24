#	$NetBSD: Makefile,v 1.1 2006/03/24 13:54:58 pavel Exp $

.ifdef REGRESS_LOG
regress:
	@echo sys/net/bpf/out-of-bounds out-of-bounds \
	`./out-of-bounds.sh` | tee ${REGRESS_LOG} ; true
.else
regress:
	@echo sys/net/bpf/out-of-bounds out-of-bounds \
	`./out-of-bounds.sh`; true 
.endif

.include <bsd.prog.mk>
