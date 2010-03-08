LIBTOOL=../../../libtool

CPPFLAGS+=-I../../../include -I../../../servers/slapd

all: autogroup.la

autogroup.lo:	autogroup.c
	$(LIBTOOL) --mode=compile $(CC) $(CPPFLAGS) -Wall -c $?

autogroup.la:	autogroup.lo
	$(LIBTOOL) --mode=link $(CC) -version-info 0:0:0 \
	-rpath $(PREFIX)/lib -module -o $@ $? 

clean:
	rm -f autogroup.lo autogroup.la

install: autogroup.la
	mkdir -p $(PREFIX)/lib/openldap
	$(LIBTOOL) --mode=install cp autogroup.la $(PREFIX)/lib/openldap
	$(LIBTOOL) --finish $(PREFIX)/lib
