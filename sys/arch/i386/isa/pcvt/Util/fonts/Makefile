
.if !exists(${.CURDIR}/../Makefile.inc)
error:
	@echo
	@echo "                      You MUST link/copy"
	@echo
	@echo "       ../Makefile.inc.NetBSD    or   ../Makefile.inc.FreeBSD"
	@echo
	@echo "             to ../Makefile.inc before you can proceed !"
	@echo
.else
FONTS	= vt100pc.814 vt100sg.814 vt220l.814 vt220h.814 vt220l.808\
	  vt220h.808 vt220l.816 vt220h.816 vt220l.810 vt220h.810
LIBMODE= 644
LIBOWN=	bin
LIBGRP=	bin

all: $(FONTS)

CLEANFILES= ${FONTS}

install: ${FONTS}
	@if [ ! -d ${FONTDIR} ]; then mkdir ${FONTDIR};fi
	@for i in ${FONTS}; do \
		echo "installing font $$i into ${FONTDIR}"; \
		install -c -m ${LIBMODE} -o ${LIBOWN} -g ${LIBGRP} \
		$$i ${FONTDIR}; \
	done

.SUFFIXES: .uu

.uu:
	uudecode < ${.IMPSRC}

clean:
	rm -f ${CLEANFILES}

.include <bsd.prog.mk>

.endif
