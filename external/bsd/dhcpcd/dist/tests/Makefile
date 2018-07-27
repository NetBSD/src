SUBDIRS=	crypt eloop-bench

all: 
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

install:

proginstall:

clean:
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

test:
	for x in ${SUBDIRS}; do cd $$x; ${MAKE} $@; cd ..; done

tests: test
