/*
 * Functions to provide access to special 68k instructions.
 */

#include <sys/cdefs.h>
#include <sys/types.h>


unsigned int *getvbr __P((void));
void setvbr __P((unsigned int *));
