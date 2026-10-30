/* complete.c, Atto Emacs, Hugh Barney, Public Domain, 2016 */

#include <glob.h>
#include "header.h"

/* basic filename completion, based on code in uemacs/PK */
int getfilename(const char *prompt, char *buf, int nbuf)
{
	glob_t gl;
	char pattern[TEMPBUF];
	size_t idx = 0;
	int cpos = 0;	/* current character position in string */
	int k = 0, didtry, iswild = 0, have_glob = 0;

	buf[0] ='\0';

	for (;;) {
		didtry = (k == 0x09);	/* Was last command tab-completion? */
		display_prompt_and_response(prompt, buf);
		k = getch(); /* get a character from the user */

		switch(k) {
		case 0x0a: /* cr, lf */
		case 0x0d:
			if (cpos > 0 && NULL != strchr(buf, '~')) goto do_tab;
			/* FALLTHRU */
		case 0x07: /* ctrl-g, abort */
			if (have_glob)
				globfree(&gl);
			return (k != 0x07 && cpos > 0);

		case 0x7f: /* del, erase */
		case 0x08: /* backspace */
			if (cpos == 0) continue;
			buf[--cpos] = '\0';
			break;

		case  0x15: /* C-u kill */
			cpos = 0;
			buf[0] = '\0';
			break;

do_tab: 
		case 0x09: /* TAB, complete file name */
			/* first time retrieval */
			if (! didtry) {
				if (have_glob)
					globfree(&gl);
				have_glob = 0;
				idx = 0;
				iswild = strpbrk(buf, "*?") != NULL;
				strlcpy(pattern, buf, sizeof(pattern));
				if (!iswild)
					strlcat(pattern, "*", sizeof(pattern));
				if (glob(pattern, GLOB_TILDE, NULL, &gl) == 0)
					have_glob = 1;
				else
					globfree(&gl);
			}

			if (have_glob && gl.gl_pathc > 0) {
				/* copy next match into buf */
				strlcpy(buf, gl.gl_pathv[idx], nbuf);
				/* cycling through all matches */
				idx = (idx + 1) % gl.gl_pathc;
			}
			cpos = strlen(buf);
			didtry = 1;
			break;

		default:
			if (cpos < nbuf - 1) {
				  buf[cpos++] = k;
				  buf[cpos] = '\0';
			}
			break;
		}
	}
}
