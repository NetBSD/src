#	$Id: Makefile,v 1.5 1993/08/02 17:15:26 mycroft Exp $

ED=	../obj/ed

all: build test
	@echo done

build: mkscripts.sh
	@echo building test scripts...
	@chmod +x mkscripts.sh
	@mkscripts.sh ${ED}

test: build ckscripts.sh
	@echo running test scripts...
	@chmod +x ckscripts.sh
	@ckscripts.sh ${ED}

clean:
	rm -f *.ed *.[oz] *~
