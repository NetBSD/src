/*	$NetBSD: comment.c,v 1.2 2022/10/08 16:12:44 christos Exp $	*/

#include <stdio.h>

void    copy_comment()
{
    int     c;

    while ((c = getchar()) != EOF) {
	if (c == '*') {
	    if ((c = getchar()) == '/') {
		putchar('\n');
		return;
	    }
	    if (c != EOF)
		ungetc(c, stdin);
	    putchar('*');
	} else {
	    putchar(c);
	}
    }
}

void    skip_string(int quote)
{
    int     c;

    while ((c = getchar()) != EOF) {
	if (c == quote) {
	    return;
	} else if (c == '\\') {
	    getchar();
	}
    }
}

int     main()
{
    int     c;

    while ((c = getchar()) != EOF) {
	switch (c) {
	case '/':
	    if ((c = getchar()) == '*') {
		copy_comment();
	    } else if (c == '/') {
		while ((c = getchar()) != EOF) {
		    putchar(c);
		    if (c == '\n')
			break;
		}
	    } else {
		if (c != EOF)
		    ungetc(c, stdin);
	    }
	    break;
	case '"':
	case '\'':
	    skip_string(c);
	    break;
	case '\\':
	    (void) getchar();
	    break;
	default:
	    break;
	}
    }
}
