ED=	../ed

all: build test
	@echo done

build: mkscripts.sh
	@echo building test scripts...
	@chmod +x mkscripts.sh
	@ED=${ED} mkscripts.sh

test: build ckscripts.sh
	@echo running test scripts...
	@chmod +x ckscripts.sh
	@ED=${ED} ckscripts.sh

clean:
	rm -f *.ed *.[oz] *~
