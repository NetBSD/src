BINDIR=		/usr/libexec
NOMAN=		noman

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		$(.CURDIR)/makewhatis.sh $(DESTDIR)$(BINDIR)/makewhatis

.include <bsd.prog.mk>
