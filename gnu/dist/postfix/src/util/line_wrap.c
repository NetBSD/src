/*++
/* NAME
/*	line_wrap 3
/* SUMMARY
/*	wrap long lines upon output
/* SYNOPSIS
/*	#include <line_wrap.h>
/*
/*	void	line_wrap(string, len, indent, output_fn, context)
/*	const char *buf;
/*	int	len;
/*	int	indent;
/*	void	(*output_fn)(const char *str, int len, int indent, char *context);
/*	char	*context;
/* DESCRIPTION
/*	The \fBline_wrap\fR routine outputs the specified string via
/*	the specified output function, and attempts to keep output lines
/*	shorter than the specified length. The routine does not attempt to
/*	break long words that do not fit on a single line. Upon output,
/*	trailing whitespace is stripped.
/*
/* Arguments
/* .IP string
/*	The input, which cannot contain any newline characters.
/* .IP len
/*	The desired maximal output line length.
/* .IP indent
/*	The desired amount of indentation of the second etc. output lines
/*	with respect to the first output line. A negative indent causes
/*	only the first line to be indented; a positive indent causes all
/*	but the first line to be indented. A zero count causes no indentation.
/* .IP output_fn
/*	The output function that is called with as arguments a string
/*	pointer, a string length, a non-negative indentation count, and
/*	application context. A typical implementation looks like this:
/* .sp
/* .nf
/* .na
void print(const char *str, int len, int indent, char *context)
{
    VSTREAM *fp = (VSTREAM *) context;

    vstream_fprintf(fp, "%*s%.*s", indent, "", len, str);
}
/* .fi
/* .ad
/* .IP context
/*	Application context that is passed on to the output function.
/*	For example, a VSTREAM pointer, or a structure that contains
/*	a VSTREAM pointer.
/* BUGS
/*	No tab expansion and no backspace processing.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <ctype.h>

/* Utility library. */

#include <line_wrap.h>

/* line_wrap - wrap long lines upon output */

void    line_wrap(const char *str, int len, int indent, LINE_WRAP_FN output_fn,
		          char *context)
{
    const char *start_line;
    const char *word;
    const char *next_word;
    const char *next_space;
    int     line_len;
    int     curr_len;
    int     curr_indent;

    if (indent < 0) {
	curr_indent = -indent;
	curr_len = len + indent;
    } else {
	curr_indent = 0;
	curr_len = len;
    }

    /*
     * At strategic positions, output what we have seen, after stripping off
     * trailing blanks.
     */
    for (start_line = word = str; word != 0; word = next_word) {
	next_space = word + strcspn(word, " \t");
	if (word > start_line) {
	    if (next_space - start_line > curr_len) {
		line_len = word - start_line;
		while (line_len > 0 && ISSPACE(start_line[line_len - 1]))
		    line_len--;
		output_fn(start_line, line_len, curr_indent, context);
		while (*word && ISSPACE(*word))
		    word++;
		if (start_line == str) {
		    curr_indent += indent;
		    curr_len -= indent;
		}
		start_line = word;
	    }
	}
	next_word = *next_space ? next_space + 1 : 0;
    }
    line_len = strlen(start_line);
    while (line_len > 0 && ISSPACE(start_line[line_len - 1]))
	line_len--;
    output_fn(start_line, line_len, curr_indent, context);
}
