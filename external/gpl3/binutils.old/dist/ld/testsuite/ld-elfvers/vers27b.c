#include "vers.h"
void foo () {}
asm (".hidden " SYMPFX(foo));
