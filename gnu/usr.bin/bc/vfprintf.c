#ifndef lint
static char rcsid[] = "$Id: vfprintf.c,v 1.2 1993/08/02 17:25:47 mycroft Exp $";
#endif /* not lint */

#include <lib.h>
#include <stdarg.h>
#include <stdio.h>

int vfprintf(file, format, argp)
FILE *file;
_CONST char *format;
va_list argp;
{
  _doprintf(file, format, argp);
  if (testflag(file, PERPRINTF)) fflush(file);
  return 0;
}
