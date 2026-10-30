/* replace.c, Atto Emacs, Hugh Barney, Public Domain, 2016 */

#include <string.h>
#include "header.h"

/*search for a string and replace it with another string */
void query_replace(void)
{
	point_t o_point = curbp->b_point;
	point_t l_point = -1;
	point_t found;
	char question[STRBUF_L];
	int slen, rlen, diff;   /* length of search and replace strings */
	int numsub = 0;   /* number of substitutions */
	int ask = TRUE;
	int c;

	searchtext[0] = '\0';
	replace[0] = '\0';
	
	if (!getinput("Query replace: ", searchtext, sizeof(searchtext), F_CLEAR))
		return;

	if (!getinput("With: ", replace, sizeof(replace), F_CLEAR))
		return;

	slen = strlen(searchtext);
	rlen = strlen(replace);
	
	/* build query replace question string */
	snprintf(question, sizeof(question), "Replace '%s' with '%s' ? ", searchtext, replace);

	/* scan through the file, from point */
	numsub = 0;
	while(TRUE) {
		found = search_forward(curbp, curbp->b_point, searchtext);

		/* if not found set the point to the last point of replacement, or where we started */
		if (found == -1) {
			curbp->b_point = (l_point == -1 ? o_point : l_point);
			break;
		}

		curbp->b_point = found;
		/* search_forward places point at end of search, move to start of search */
		curbp->b_point -= slen;

		if (ask == TRUE) {
			msg("%s", question);
			clrtoeol();
			
		qprompt:
			display(curwp, TRUE);
			c = getch();

			switch (c) {
			case 'y': /* yes, substitute */
				break;
			
			case 'n': /* no, find next */
				curbp->b_point = found; /* set to end of search string */
				continue;
			
			case '!': /* yes/stop asking, do the lot */
				ask = FALSE;
				break;
			
			case 0x1B: /* esc */
				flushinp(); /* discard any escape sequence without writing in buffer */
			case 'q': /* controlled exit */
				return;

			default: /* help me */
				msg("(y)es, (n)o, (!)do the rest, (q)uit");
				goto qprompt;
			}
		}
		
		diff = rlen - slen;
		if (diff != 0) {
			movegap(curbp, found);
			/* check if we need to grow */
			if (diff > 0 && curbp->b_egap - curbp->b_gap < diff &&
			    !growgap(curbp, diff))
				return;
			curbp->b_gap += diff;
		}

		/* now just overwrite the chars at point in the buffer */
		l_point = curbp->b_point;
		memcpy(ptr(curbp, curbp->b_point), replace, rlen * sizeof (char_t));
		curbp->b_flags |= B_MODIFIED;
		curbp->b_point = found + diff; /* end of replcement */
		numsub++;
	}

	msg("%d substitutions", numsub);
}
