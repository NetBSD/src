#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "speed.h"

const struct test {
	char *name; 
	void (*func)__P((int));
	char *comment;
	int count;
} testlist[] = {
	{"Illegal", illegal, "(test: unimplemented)", 1},
	{"mulsl Da,Db", mul32sreg, "(test: should be native)", 200000000},
	{"mulsl sp@(8),Da", mul32smem, "(test: should be native)\n", 
	    200000000},
	
	{"mulsl Dn,Da:Db", mul64sreg, "emulated on 68060", 2000000},
	{"mulul Dn,Da:Db", mul64ureg, "\t\"", 2000000},
	{"mulsl sp@(8),Da:Db", mul64smem, "\t\"", 1000000},
	{"mulul sp@(8),Da:Db", mul64umem, "\t\"\n", 1000000},

	{"divsl Da:Db,Dn", div64sreg, "\t\"", 500000},
	{"divul Da:Db,Dn", div64ureg, "\t\"", 500000},
	{"divsl Da:Db,sp@(8)", div64smem, "\t\"", 300000},
	{"divul Da:Db,sp@(8)", div64umem, "\t\"\n", 300000},

	{NULL, NULL, NULL}
};

jmp_buf jbuf;
void illhand (int);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	const struct test *t;
	clock_t start, stop;


	if (signal(SIGILL, &illhand))
		warn("%s: can't install illegal instruction handler.",
		    argv[0]);

	printf("Speed of instructions which are emulated on some cpus:\n\n");
	(void)sleep(1);
	for (t=testlist; t->name; t++) {
		printf("%-20s", t->name);
		fflush(stdout);

		if (setjmp(jbuf)) {
			printf("%15s    %s\n", "[unimplemented]", t->comment);
			continue;
		}
			
		start = clock();
		t->func(t->count);
		stop = clock();
		printf("%13d/s    %s\n",
		    CLOCKS_PER_SEC*(t->count /(stop - start)),
		    t->comment);
	}
	exit (0);
}

void
illhand(int i)
{
	longjmp(jbuf, 1);
}
