/*
** Calculate a percentage without resorting to floating point
** and return a pointer to a string
**
** "digits" is the number of digits past the decimal place you want
** (zero being the straight percentage with no decimals)
**
** Erik E. Fair <fair@clock.org>, May 8, 1997
*/

#include <stdio.h>
#include <machine/limits.h>
#include <sys/types.h>

extern char * strpct(u_long num, u_long denom, u_int digits);

char *
strpct(numerator, denominator, digits)
u_long  numerator, denominator;
u_int   digits;
{
        register int i;
        u_long result, factor = 100L;
        static char     percent[32];

        /* I should check for digit overflow here, too XXX */
        for(i = 0; i < digits; i++) {
                factor *= 10;
        }

        /* watch out for overflow! */
        if (numerator < (ULONG_MAX / factor)) {
                numerator *= factor;
        } else {
                /* toss some of the bits of lesser significance */
                denominator /= factor;
        }

        /* divide by zero is just plain bad */
        if (denominator == 0L) {
                denominator = 1L;
        }

        result = numerator / denominator;

        if (digits == 0) {
                (void) snprintf(percent, sizeof(percent), "%lu%%", result);
        } else {
                char    fmt[32];

                /* indirection to produce the right output format */
                (void) snprintf(fmt, sizeof(fmt), "%%lu.%%0%ulu%%%%", digits);

                factor /= 100L;         /* undo initialization */

                (void) snprintf(percent, sizeof(percent),
                        fmt, result / factor, result % factor);
        }       

        return(percent);
}
