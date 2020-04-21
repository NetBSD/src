#	$NetBSD: Makefile,v 1.12.98.2 2020/04/21 19:37:32 martin Exp $

.include <bsd.own.mk>

ED?=	../obj/ed

all: check
	@:

check: build test
	@if grep -h '\*\*\*' errs.o scripts.o; then :; else \
		echo "tests completed successfully."; \
	fi

build: mkscripts.sh
	@if [ -f errs.o ]; then :; else \
		echo "building test scripts for $(ED) ..."; \
		${HOST_SH} ${.CURDIR}/mkscripts.sh $(ED); \
	fi

test: build ckscripts.sh
	@echo testing $(ED) ...
	@${HOST_SH} ckscripts.sh $(ED)

clean:
	rm -f *.ed *.red *.[oz] *~

.include <bsd.prog.mk>
