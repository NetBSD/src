DEMOS=	chardemo.vt colors.vt sgr.vt

all: $(DEMOS)

CLEANFILES= ${DEMOS}

install: ${DEMOS}
	@echo "to look at the demos, execute \"cat <filename>.vt\""

.SUFFIXES: .uu

.uu:
	uudecode < ${.IMPSRC}

clean:
	rm -f ${CLEANFILES}

.include <bsd.prog.mk>
