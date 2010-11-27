# $Id: Makefile,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $

CC=gcc
SRC_PREFIX=../src
CFLAGS=-I../include -I$(SRC_PREFIX)
ENV=PATH=/usr/local/bin:${PATH}
ATFLIBS=-latf-c -lssl
LDFLAGS=`pwd`/$(SRC_PREFIX)/libsaslc.so -ggdb
TEST_CASES=t_dict t_session t_crypto t_error t_saslc t_parser
VALGRIND=valgrind
VALGRIND_OPTS=--leak-check=full -q

all: build

build: $(TEST_CASES) example_client

$(TEST_CASES):
		$(CC) -o $@.o -c $@.c $(CFLAGS) -I/usr/include/local
		$(CC) -o $@ $@.o $(LDFLAGS) $(ATFLIBS)

atf:
		@echo "atf tests:"
		@$(ENV) atf-run | $(ENV) atf-report

valgrind:
		@echo "valgrind tests:"
		@for prog in $(TEST_CASES); do \
			echo "$$prog:"; \
			for testcase in `./$$prog -l | grep ident | awk {'print $$2'}`; do \
				echo " * $$testcase"; \
				$(VALGRIND) $(VALGRIND_OPTS) ./$$prog $$testcase ; \
			done \
		done

tests: build atf valgrind

example_client: example_client.c

clean:
		rm -f $(TEST_CASES) *.o resfile example_client
