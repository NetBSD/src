ED!=cd $(.CURDIR)/..; \
	printf "xxx:\n\techo \$${.OBJDIR}/ed\n" | make -r -s -f - xxx

all: build test
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
