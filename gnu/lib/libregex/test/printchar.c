#ifndef lint
static char rcsid[] = "$Id: printchar.c,v 1.2 1993/08/02 17:24:23 mycroft Exp $";
#endif /* not lint */

void
printchar (c)
     char c;
{
  if (c < 040 || c >= 0177)
    {
      putchar ('\\');
      putchar (((c >> 6) & 3) + '0');
      putchar (((c >> 3) & 7) + '0');
      putchar ((c & 7) + '0');
    }
  else
    putchar (c);
}
