#	$Id: Makefile,v 1.6 1993/08/02 20:43:57 jtc Exp $

.if exists(../obj)
ED=	../obj/ed
.else
ED=	../ed
.endif

all:	${ED} build test
	@echo done

build: mkscripts.sh
	@echo building test scripts...
	@chmod +x mkscripts.sh
	@./mkscripts.sh ${ED}

test: build ckscripts.sh
	@echo running test scripts...
	@chmod +x ckscripts.sh
	@./ckscripts.sh ${ED}

clean:
	rm -f *.ed *.[oz] *~
