#include <machine/cpufunc.h>
#include <machine/mon.h>
#include "vector.h"


unsigned int *old_vector_table;

static void initialize_vector_table()
{
    int i;

    old_vector_table = getvbr();
    for (i = 0; i < NVECTORS; i++) {
	if (vector_table[i] == COPY_ENTRY)
	    vector_table[i] = old_vector_table[i];
    }
    setvbr(vector_table);
}


void bootstrap()
{
    static char hello[] = "hello world";
    int i;

    for (i=0; i < 11; i++) {
	mon_putchar(hello[i]);
    }
    mon_printf("\nPROM Version: %d\n", romp->romvecVersion);
    initialize_vector_table();
    
    mon_exit_to_mon();
}

