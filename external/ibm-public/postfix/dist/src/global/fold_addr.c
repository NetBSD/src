/*	$NetBSD: fold_addr.c,v 1.1.1.1.36.1 2017/04/21 16:52:48 bouyer Exp $	*/

/*++
/* NAME
/*	fold_addr 3
/* SUMMARY
/*	address case folding
/* SYNOPSIS
/*	#include <fold_addr.h>
/*
/*	char	*fold_addr(result, addr, flags)
/*	VSTRING *result;
/*	const char *addr;
/*	int	flags;
/* DESCRIPTION
/*	fold_addr() case folds an address according to the options
/*	specified with \fIflags\fR. The result value is the output
/*	address.
/*
/*	Arguments
/* .IP result
/*	Result buffer with the output address. Note: casefolding
/*	may change the string length.
/* .IP addr
/*	Null-terminated read-only string with the input address.
/* .IP flags
/*	Zero or the bit-wise OR of:
/* .RS
/* .IP FOLD_ADDR_USER
/*	Case fold the address local part.
/* .IP FOLD_ADDR_HOST
/*	Case fold the address domain part.
/* .IP FOLD_ADDR_ALL
/*	Alias for (FOLD_ADDR_USER | FOLD_ADDR_HOST).
/* .RE
/* SEE ALSO
/*	msg(3) diagnostics interface
/*	casefold(3) casefold text
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
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

/* Utility library. */

#include <stringops.h>

/* Global library. */

#include <fold_addr.h>

#define STR(x)	vstring_str(x)

/* fold_addr - case fold mail address */

char   *fold_addr(VSTRING *result, const char *addr, int flags)
{
    char   *cp;

    /*
     * Fold the address as appropriate.
     */
    switch (flags & FOLD_ADDR_ALL) {
    case FOLD_ADDR_HOST:
	if ((cp = strrchr(addr, '@')) != 0) {
	    cp += 1;
	    vstring_strncpy(result, addr, cp - addr);
	    casefold_append(result, cp);
	    break;
	}
	/* FALLTHROUGH */
    case 0:
	vstring_strcpy(result, addr);
	break;
    case FOLD_ADDR_USER:
	if ((cp = strrchr(addr, '@')) != 0) {
	    casefold_len(result, addr, cp - addr);
	    vstring_strcat(result, cp);
	    break;
	}
	/* FALLTHROUGH */
    case FOLD_ADDR_USER | FOLD_ADDR_HOST:
	casefold(result, addr);
	break;
    }
    return (STR(result));
}

#ifdef TEST
#include <stdlib.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <msg_vstream.h>
#include <argv.h>

int     main(int argc, char **argv)
{
    VSTRING *line_buffer = vstring_alloc(1);
    VSTRING *fold_buffer = vstring_alloc(1);
    ARGV   *cmd;
    char  **args;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    util_utf8_enable = 1;
    while (vstring_fgets_nonl(line_buffer, VSTREAM_IN)) {
	vstream_printf("> %s\n", STR(line_buffer));
	cmd = argv_split(STR(line_buffer), CHARS_SPACE);
	if (cmd->argc == 0 || cmd->argv[0][0] == '#') {
	    argv_free(cmd);
	    continue;
	}
	args = cmd->argv;

	/*
	 * Fold the host.
	 */
	if (strcmp(args[0], "host") == 0 && cmd->argc == 2) {
	    vstream_printf("\"%s\" -> \"%s\"\n", args[1], fold_addr(fold_buffer,
						  args[1], FOLD_ADDR_HOST));
	}

	/*
	 * Fold the user.
	 */
	else if (strcmp(args[0], "user") == 0 && cmd->argc == 2) {
	    vstream_printf("\"%s\" -> \"%s\"\n", args[1], fold_addr(fold_buffer,
						  args[1], FOLD_ADDR_USER));
	}

	/*
	 * Fold user and host.
	 */
	else if (strcmp(args[0], "all") == 0 && cmd->argc == 2) {
	    vstream_printf("\"%s\" -> \"%s\"\n", args[1], fold_addr(fold_buffer,
						   args[1], FOLD_ADDR_ALL));
	}

	/*
	 * Fold none.
	 */
	else if (strcmp(args[0], "none") == 0 && cmd->argc == 2) {
	    vstream_printf("\"%s\" -> \"%s\"\n", args[1], fold_addr(fold_buffer,
							       args[1], 0));
	}

	/*
	 * Usage.
	 */
	else {
	    vstream_printf("Usage: %s host <addr> | user <addr> | all <addr>\n",
			   argv[0]);
	}
	vstream_fflush(VSTREAM_OUT);
	argv_free(cmd);
    }
    vstring_free(line_buffer);
    vstring_free(fold_buffer);
    exit(0);
}

#endif					/* TEST */
