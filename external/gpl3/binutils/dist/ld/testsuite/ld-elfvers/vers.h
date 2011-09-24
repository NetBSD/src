/* Add the symbol prefix to the symbol as needed.
   Usage: SYMPFX(foo);  */
#define __SYMPFX(pfx, sym) #pfx sym
#define _SYMPFX(pfx, sym) __SYMPFX(pfx, sym)
#define SYMPFX(sym) _SYMPFX(__USER_LABEL_PREFIX__, #sym)

/* Generate a .symver reference with symbol prefixes.
   Usage: SYMVER(foo, foobar@ver);  */
#define SYMVER(name, name2) __asm__(".symver " SYMPFX(name) "," SYMPFX(name2))
