/*	$NetBSD: syntax.c,v 1.8 2019/02/04 09:56:48 kre Exp $	*/

#include <sys/cdefs.h>
__RCSID("$NetBSD: syntax.c,v 1.8 2019/02/04 09:56:48 kre Exp $");

#include <limits.h>
#include "shell.h"
#include "syntax.h"
#include "parser.h"

#if CWORD != 0
#error initialisation assumes 'CWORD' is zero
#endif

#define ndx(ch) (ch + 2 - CHAR_MIN)
#define set(ch, val) [ndx(ch)] = val,
#define set_range(s, e, val) [ndx(s) ... ndx(e)] = val,

/* syntax table used when not in quotes */
const char basesyntax[258] = { CFAKE, CEOF,
    set_range(CTL_FIRST, CTL_LAST, CCTL)
/* Note code assumes that only the above are CCTL in basesyntax */
    set('\n', CNL)
    set('\\', CBACK)
    set('\'', CSQUOTE)
    set('"', CDQUOTE)
    set('`', CBQUOTE)
    set('$', CVAR)
    set('}', CENDVAR)
    set('<', CSPCL)
    set('>', CSPCL)
    set('(', CSPCL)
    set(')', CSPCL)
    set(';', CSPCL)
    set('&', CSPCL)
    set('|', CSPCL)
    set(' ', CSPCL)
    set('\t', CSPCL)
};

/* syntax table used when in double quotes */
const char dqsyntax[258] = { CFAKE, CEOF,
    set_range(CTL_FIRST, CTL_LAST, CCTL)
    set('\n', CNL)
    set('\\', CBACK)
    set('"', CDQUOTE)
    set('`', CBQUOTE)
    set('$', CVAR)
    set('}', CENDVAR)
    /* ':/' for tilde expansion, '-]' for [a\-x] pattern ranges */
    set('!', CCTL)
    set('*', CCTL)
    set('?', CCTL)
    set('[', CCTL)
    set('=', CCTL)
    set('~', CCTL)
    set(':', CCTL)
    set('/', CCTL)
    set('-', CCTL)
    set(']', CCTL)
};

/* syntax table used when in single quotes */
const char sqsyntax[258] = { CFAKE, CEOF,
/* CCTL includes anything that might perhaps need to be escaped if quoted */
    set_range(CTL_FIRST, CTL_LAST, CCTL)
    set('\n', CNL)
    set('\'', CSQUOTE)
    set('\\', CSBACK)
    /* ':/' for tilde expansion, '-]' for [a\-x] pattern ranges */
    set('!', CCTL)
    set('*', CCTL)
    set('?', CCTL)
    set('[', CCTL)
    set('=', CCTL)
    set('~', CCTL)
    set(':', CCTL)
    set('/', CCTL)
    set('-', CCTL)
    set(']', CCTL)
};

/* syntax table used when in arithmetic */
const char arisyntax[258] = { CFAKE, CEOF,
    set_range(CTL_FIRST, CTL_LAST, CCTL)
    set('\n', CNL)
    set('\\', CBACK)
    set('`', CBQUOTE)
    set('\'', CSQUOTE)
    set('"', CDQUOTE)
    set('$', CVAR)
    set('}', CENDVAR)
    set('(', CLP)
    set(')', CRP)
};

/* character classification table */
const char is_type[258] = { 0, 0,
    set_range('0', '9', ISDIGIT)
    set_range('a', 'z', ISLOWER)
    set_range('A', 'Z', ISUPPER)
    set('_', ISUNDER)
    set('#', ISSPECL)
    set('?', ISSPECL)
    set('$', ISSPECL)
    set('!', ISSPECL)
    set('-', ISSPECL)
    set('*', ISSPECL)
    set('@', ISSPECL)
    set(' ', ISSPACE)
    set('\t', ISSPACE)
    set('\n', ISSPACE)
};
