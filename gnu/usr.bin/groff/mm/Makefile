# Makefile for mm macros

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	bin
TMACGRP?=	bin
TMACMODE?=	444

FILES=		0.MT 5.MT 4.MT ms.cov se_ms.cov 
LOCALE=		locale se_locale

MAN7=		groff_mm.0 groff_mmse.0
MLINKS=		groff_mm.7 mm.7 groff_mmse.7 mmse.7

afterinstall:
	install -c -o $(TMACOWN) -g $(TMACGRP) -m $(TMACMODE) \
		$(.CURDIR)/tmac.m $(DESTDIR)$(TMACDIR)/tmac.m
	install -c -o $(TMACOWN) -g $(TMACGRP) -m $(TMACMODE) \
		$(.CURDIR)/tmac.m $(DESTDIR)$(TMACDIR)/tmac.mse

	install -d -o $(TMACOWN) -g $(TMACGRP) -m 755 $(DESTDIR)$(TMACDIR)/mm
	for f in $(FILES); do \
		rm -f $(TMACDIR)/mm/$$f; \
		install -c -o $(TMACOWN) -g $(TMACGRP) -m $(TMACMODE) \
			$(.CURDIR)/mm/$$f $(DESTDIR)$(TMACDIR)/mm/$$f; \
	done
	for f in $(LOCALE); do \
		if test ! -f $(DESTDIR)$(TMACDIR)/mm/$$f; then \
			install -o $(TMACOWN) -g $(TMACGRP) -m $(TMACMODE) \
				/dev/null $(DESTDIR)$(TMACDIR)/mm/$$f; \
		fi; \
	done

.include "../Makefile.cfg"
.include <bsd.prog.mk>
