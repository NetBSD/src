/* Check exit_group(2) trivially with non-zero status.
#notarget: cris-*-* *-*-elf
#output: exit_group\n
#xerror:
*/
#define EXITVAL 1
#include "exitg1.c"
