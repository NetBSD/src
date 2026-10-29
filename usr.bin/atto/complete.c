/* complete.c, Atto Emacs, Hugh Barney, Public Domain, 2016 */

#include "header.h"

/* basic filename completion, based on code in uemacs/PK */
int getfilename(const char *prompt, char *buf, int nbuf)
{
	static char temp_file[STRBUF_S] = TEMPFILE;
	int cpos = 0;	/* current character position in string */
	int k = 0, c, fd, didtry, iswild = 0;

	char sys_command[TEMPBUF];
	FILE *fp = NULL;
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
			if (fp != NULL) fclose(fp);
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
			/* scan backwards for a wild card and set */
			iswild=0;
			while (cpos > 0) {
				cpos--;
				if (buf[cpos] == '*' || buf[cpos] == '?')
					iswild = 1;
			}

			/* first time retrieval */
			if (! didtry) {
				if (fp != NULL) fclose(fp);
				strlcpy(temp_file, TEMPFILE, sizeof(temp_file));
				if (-1 == (fd = mkstemp(temp_file)))
					fatal("%s: Failed to create temp file\n");
				strlcpy(sys_command, "echo ", sizeof(sys_command));
				strlcat(sys_command, buf, sizeof(sys_command));
				if (!iswild) strlcat(sys_command, "*", sizeof(sys_command));
				strlcat(sys_command, " >", sizeof(sys_command));
				strlcat(sys_command, temp_file, sizeof(sys_command));
				strlcat(sys_command, " 2>&1", sizeof(sys_command));
				(void) system(sys_command); /* stop compiler unused result warning */
				fp = fdopen(fd, "r");
				unlink(temp_file);
			}

			/* copy next filename into buf */
			while ((c = getc(fp)) != EOF && c != '\n' && c != ' ')
				if (cpos < nbuf - 1 && c != '*')
					buf[cpos++] = c;

			buf[cpos] = '\0';
			if (c != ' ') rewind(fp);
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
