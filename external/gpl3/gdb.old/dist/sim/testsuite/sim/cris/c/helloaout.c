/* Make sure we don't just assume ELF all over.  (We have to jump
   through hoops to get runnable a.out out of the ELF setup, and
   having problems with a.out and discontinous section arrangements
   doesn't help.  Adjust as needed to get a.out which says "pass".  If
   necessary, move to the asm subdir.  By design, it doesn't work with
   CRIS v32.)
#target: cris-*-elf
#cc: ldflags=-Wl,-mcrisaout\ -sim\ -Ttext=0
*/
#include "hello.c"
