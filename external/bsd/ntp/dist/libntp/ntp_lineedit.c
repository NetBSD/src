/*	$NetBSD: ntp_lineedit.c,v 1.1.1.1 2009/12/13 16:55:02 kardel Exp $	*/

/*
 * ntp_lineedit.c - generic interface to various line editing libs
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(HAVE_READLINE_HISTORY)
# include <readline/readline.h>
# include <readline/history.h>
#else 
# if defined(HAVE_HISTEDIT_H)
#  include <histedit.h>
# endif
#endif

#include "ntp.h"
#include "ntp_stdlib.h"
#include "ntp_lineedit.h"

#define MAXEDITLINE	512

/*
 * external references
 */

extern char *	progname;

/*
 * globals, private prototypes
 */

static int	ntp_readline_initted;
static char *	lineedit_prompt;


#if !defined(HAVE_READLINE_HISTORY) && defined(HAVE_HISTEDIT_H)

	static EditLine *	ntp_el;
	static History *	ntp_hist;
	static HistEvent	hev;

	char *	ntp_prompt_callback(EditLine *);

#endif /* !HAVE_READLINE_HISTORY_H && HAVE_HISTEDIT_H */


/*
 * ntp_readline_init - setup, set or reset prompt string
 */
int
ntp_readline_init(
	const char *	prompt
	)
{
	int	success;

	success = 1;

	if (prompt) {
		if (lineedit_prompt) 
			free(lineedit_prompt);
		lineedit_prompt = estrdup(prompt);
	}

#if !defined(HAVE_READLINE_HISTORY) && defined(HAVE_HISTEDIT_H)

	if (NULL == ntp_el) {

		ntp_el = el_init(progname, stdin, stdout, stderr);
		if (ntp_el) {

			el_set(ntp_el, EL_PROMPT, ntp_prompt_callback);
			el_set(ntp_el, EL_EDITOR, "emacs");

			ntp_hist = history_init();

			if (NULL == ntp_hist) {

				fprintf(stderr, "history_init(): %s\n",
						strerror(errno));
				fflush(stderr);

				el_end(ntp_el);
				ntp_el = NULL;

				success = 0;

			} else {
				memset(&hev, 0, sizeof hev);

				history(ntp_hist, &hev,	H_SETSIZE, 128);

				el_set(ntp_el, EL_HIST, history, ntp_hist);

				/* use any .editrc */
				el_source(ntp_el, NULL);
			}
		} else
			success = 0;
	}

#endif	/* !HAVE_READLINE_HISTORY && HAVE_HISTEDIT_H */

	ntp_readline_initted = success;

	return success;
}


/*
 * ntp_readline_uninit - release resources
 */
void
ntp_readline_uninit(
	void
	)
{
#if !defined(HAVE_READLINE_HISTORY) && defined(HAVE_HISTEDIT_H)

	if (ntp_el) {
		el_end(ntp_el);
		ntp_el = NULL;

		history_end(ntp_hist);
		ntp_hist = NULL;
	}

#endif /* !HAVE_READLINE_HISTORY && HAVE_HISTEDIT_H */

	if (lineedit_prompt) {
		free(lineedit_prompt);
		lineedit_prompt = NULL;
	}

	ntp_readline_initted = 0;
}


/*
 * ntp_readline - read a line with the line editor available
 *
 * The string returned must be released with free()
 */

char *
ntp_readline(
	int *	pcount
	)
{
#if !defined(HAVE_READLINE_HISTORY) && !defined(HAVE_HISTEDIT_H)
	char line_buf[MAXEDITLINE];
#endif
#if !defined(HAVE_READLINE_HISTORY) && defined(HAVE_HISTEDIT_H)
	const char *	cline;
#endif
	char *		line;

	if (!ntp_readline_initted)
		return NULL;

	*pcount = 0;

#if defined(HAVE_READLINE_HISTORY)

	line = readline(lineedit_prompt ? lineedit_prompt : "");
	if (NULL != line) {
		if (*line) {
			add_history(line);
			*pcount = strlen(line);
		} else {
			free(line);
			line = NULL;
		}
	}

#endif	/* HAVE_READLINE_HISTORY */

#if !defined(HAVE_READLINE_HISTORY) && defined(HAVE_HISTEDIT_H)

	cline = el_gets(ntp_el, pcount);

	if (NULL != cline && *cline) {
		history(ntp_hist, &hev, H_ENTER, cline);
		*pcount = strlen(cline);
		line = estrdup(cline);
	} else
		line = NULL;

#endif	/* !HAVE_READLINE_HISTORY && HAVE_HISTEDIT_H */

#if !defined(HAVE_READLINE_HISTORY) && !defined(HAVE_HISTEDIT_H)
					/* stone hammers */
	if (lineedit_prompt) {
# ifdef VMS
			/*
			 * work around problem mixing
			 * stdout & stderr
			 */
			fputs("", stdout);
# endif	/* VMS */

		fputs(lineedit_prompt, stderr);
		fflush(stderr);
	}

	line = fgets(line_buf, sizeof(line_buf), stdin);
	if (NULL != line && *line) {
		*pcount = strlen(line);
		line = estrdup(line);
	} else
		line = NULL;

#endif	/* !HAVE_READLINE_HISTORY && !HAVE_HISTEDIT_H */


	if (!line)			/* EOF */
		fputs("\n", stderr);

	return line;
}


#if !defined(HAVE_READLINE_HISTORY) && defined(HAVE_HISTEDIT_H)
/*
 * ntp_prompt_callback - return prompt string to el_gets()
 */
char *
ntp_prompt_callback(
	EditLine *el
	)
{
	UNUSED_ARG(el);

	return lineedit_prompt;
}
#endif /* !HAVE_READLINE_HISTORY_H && HAVE_HISTEDIT_H */

