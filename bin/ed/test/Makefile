ED=	../ed

all: mkscripts ckscripts

mkscripts: mkscripts.sh
	@echo building test scripts...
	@chmod +x mkscripts.sh
	@ED=${ED} mkscripts.sh

ckscripts: ckscripts.sh
	@echo running test scripts...
	@chmod +x ckscripts.sh
	@ED=${ED} ckscripts.sh

clean:
	rm -f *.ed *.[oz]
