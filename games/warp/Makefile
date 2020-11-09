CFLAGS+=-O
SRCS=	bang.c init.c intrp.c move.c object.c play.c score.c sig.c term.c \
	them.c us.c util.c version.c warp.c weapon.c
OBJS=	bang.o init.o intrp.o move.o object.o play.o score.o sig.o term.o \
	them.o us.o util.o version.o warp.o weapon.o

warp: ${OBJS}
	${CC} ${OBJS} -o $@ -ltermlib -lm

install: warp
	export PATH || exit 1
	- mv $(bin)/warp $(bin)/warp.old
	- if test `pwd` != $(bin); then cp $(public) $(bin); fi
	- cd $(bin); chmod 755 $(public); chown '$(CHOWNER)' $(public)
	- chmod 4711 $(bin)/warp
	- chmod 755 makedir
	- ./makedir `./filexp $(privlib)`
	- \
if test `pwd` != `./filexp $(privlib)`; then \
cp $(private) `./filexp $(privlib)`; \
fi
	- \
cd `./filexp $(privlib)`; \
chmod 755 $(private); \
chown '$(CHOWNER)' . $(private); \
cp /dev/null save.blurfl
	- \
if test ! -f `./filexp $(privlib)/warp.news`; then \
cp warp.news `./filexp $(privlib)`; \
fi
	- \
if test `pwd` != $(mansrc); then \
for page in $(manpages); do \
cp $$page $(mansrc)/`basename $$page .man`.$(manext); \
done; \
fi

clean:
	rm -f warp core ${OBJS}

smap.0: smp.0 sm
	sm <smp.0 >smap.0
smap.1: smp.1 sm
	sm <smp.1 >smap.1
smap.2: smp.2 sm
	sm <smp.2 >smap.2
smap.3: smp.3 sm
	sm <smp.3 >smap.3
smap.4: smp.4 sm
	sm <smp.4 >smap.4
smap.5: smp.5 sm
	sm <smp.5 >smap.5
smap.6: smp.6 sm
	sm <smp.6 >smap.6
smap.7: smp.7 sm
	sm <smp.7 >smap.7
sm: sm.c
	${CC} sm.c -o $@

depend: ${SRCS}
	mkdep ${SRCS}
