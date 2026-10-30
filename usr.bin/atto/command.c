/* command.c, Atto Emacs, Public Domain, Hugh Barney, 2016, Derived from: Anthony's Editor January 93 */

#include <sys/param.h> // for MIN()
#include "header.h"

void quit(void) { done = 1; }
void up(void) { curbp->b_point = lncolumn(curbp, upup(curbp, curbp->b_point),curbp->b_col); }
void down(void) { curbp->b_point = lncolumn(curbp, dndn(curbp, curbp->b_point),curbp->b_col); }
void lnbegin(void) { curbp->b_point = segstart(curbp, lnstart(curbp,curbp->b_point), curbp->b_point); }
void version(void) { msg(VERSION); }
void top(void) { curbp->b_point = 0; }
void bottom(void) { curbp->b_point = pos(curbp, curbp->b_ebuf); if (curbp->b_epage < pos(curbp, curbp->b_ebuf)) curbp->b_reframe = 1;}
void block(void) { curbp->b_mark = curbp->b_point; }
void copy(void) { copy_cut(FALSE); }
void cut(void) { copy_cut(TRUE); }
void resize_terminal(void) { one_window(curwp); }

void quit_ask(void)
{
	if (modified_buffers() > 0) {
		mvaddstr(MSGLINE, 0, "Modified buffers exist; really exit (y/n) ?");
		clrtoeol();
		if (!yesno(FALSE))
			return;
	}
	quit();
}

/* flag = default answer, FALSE=n, TRUE=y */
int yesno(int flag)
{
	int ch;

	addstr(flag ? " y\b" : " n\b");
	refresh();
	ch = getch();
	if (ch == '\r' || ch == '\n')
		return (flag);
	return (tolower(ch) == 'y');
}

void redraw(void)
{
	window_t *wp;
	
	clear();
	for (wp=wheadp; wp != NULL; wp = wp->w_next)
		wp->w_update = TRUE;
	update_display();
}

void left(void)
{
	int n = prev_utf8_char_size();
	while (0 < curbp->b_point && n-- > 0)
		--curbp->b_point;
}

void right(void)
{
	int n = utf8_size(*ptr(curbp,curbp->b_point));
	while ((curbp->b_point < pos(curbp, curbp->b_ebuf)) && n-- > 0)
		++curbp->b_point;
}

/* work out number of bytes based on first byte */
int utf8_size(char_t c)
{
	if (c >= 192 && c < 224) return 2;
	if (c >= 224 && c < 240) return 3;
	if (c >= 240 && c < 248) return 4;
	return 1; /* if in doubt it is 1 */
}

int prev_utf8_char_size(void)
{
	int n;
	for (n=2;n<5;n++)
		if (-1 < curbp->b_point - n && (utf8_size(*(ptr(curbp, curbp->b_point - n))) == n))
			return n;
	return 1;
}

void lnend(void)
{
        if (curbp->b_point == pos(curbp, curbp->b_ebuf)) return; /* do nothing if EOF */
	curbp->b_point = dndn(curbp, curbp->b_point);
	point_t p = curbp->b_point;
	left();
	curbp->b_point = (*ptr(curbp, curbp->b_point) == '\n') ? curbp->b_point : p;
}

void wleft(void)
{
	char_t *p;
	while (!isspace(*(p = ptr(curbp, curbp->b_point))) && curbp->b_buf < p)
		--curbp->b_point;
	while (isspace(*(p = ptr(curbp, curbp->b_point))) && curbp->b_buf < p)
		--curbp->b_point;
}

void pgdown(void)
{
	curbp->b_page = curbp->b_point = upup(curbp, curbp->b_epage);
	while (0 < curbp->b_row--)
		down();
	curbp->b_epage = pos(curbp, curbp->b_ebuf);
}

void pgup(void)
{
	int i = curwp->w_rows;
	while (0 < --i) {
		curbp->b_page = upup(curbp, curbp->b_page);
		up();
	}
}

void wright(void)
{
	char_t *p;
	while (!isspace(*(p = ptr(curbp, curbp->b_point))) && p < curbp->b_ebuf)
		++curbp->b_point;
	while (isspace(*(p = ptr(curbp, curbp->b_point))) && p < curbp->b_ebuf)
		++curbp->b_point;
}

void insert(void)
{
	assert(curbp->b_gap <= curbp->b_egap);
	if (curbp->b_gap == curbp->b_egap && !growgap(curbp, CHUNK))
		return;
	curbp->b_point = movegap(curbp, curbp->b_point);

	/* overwrite if mid line, not EOL or EOF, CR will insert as normal */
	if ((curbp->b_flags & B_OVERWRITE) && *input != '\r' && *(ptr(curbp, curbp->b_point)) != '\n' && curbp->b_point < pos(curbp,curbp->b_ebuf) ) {
		*(ptr(curbp, curbp->b_point)) = *input;
		if (curbp->b_point < pos(curbp, curbp->b_ebuf))
			++curbp->b_point;
	} else {
		*curbp->b_gap++ = *input == '\r' ? '\n' : *input;
		curbp->b_point = pos(curbp, curbp->b_egap);
		// force reframe if scrolled off bottom of screen and at EOF
		if (curbp->b_point == pos(curbp, curbp->b_ebuf) && curbp->b_point >= curbp->b_epage) curbp->b_reframe = 1;
	}
	curbp->b_flags |= B_MODIFIED;
}

void backsp(void)
{
	curbp->b_point = movegap(curbp, curbp->b_point);
	if (curbp->b_buf < curbp->b_gap) {
		curbp->b_gap -= prev_utf8_char_size();
		curbp->b_flags |= B_MODIFIED;
	}
	curbp->b_point = pos(curbp, curbp->b_egap);
}

void delete(void)
{
	curbp->b_point = movegap(curbp, curbp->b_point);
	if (curbp->b_egap < curbp->b_ebuf) {
		curbp->b_egap += MIN(utf8_size(*curbp->b_egap),
		    curbp->b_ebuf - curbp->b_egap);
		curbp->b_point = pos(curbp, curbp->b_egap);
		curbp->b_flags |= B_MODIFIED;
	}
}

void gotoline(void)
{
	long lline;
	int line;
	char *ep;
	point_t p;

	if (getinput("Goto line: ", temp, STRBUF_S, F_CLEAR)) {
		errno = 0;
		lline = strtol(temp, &ep, 10);
		if (ep == temp || *ep != '\0' || errno != 0 || lline < INT_MIN || INT_MAX < lline) {
			msg("Line %s, not valid", temp);
			return;
		}
		line = (int) lline;
		p = line_to_point(line);
		if (p != -1) {
			curbp->b_point = p;
			msg("Line %d", line);
		} else {
			msg("Line %d, not found", line);
		}
	}
}

void insertfile(void)
{
	if (getfilename("Insert file: ", temp, NAME_MAX))
		(void)insert_file(temp, TRUE);
}

void readfile(void)
{
	buffer_t *bp;
	
	temp[0] = '\0';
	int result = getfilename("Find file: ", (char*)temp, NAME_MAX);
	/* int result = getinput("Find file: ", (char*)temp, NAME_MAX, F_CLEAR); */

	if (result) {
		bp = find_buffer(temp, TRUE);
		disassociate_b(curwp); /* we are leaving the old buffer for a new one */
		curbp = bp;
		associate_b2w(curbp, curwp);

		/* load the file if not already loaded */
		if (bp != NULL && bp->b_fname[0] == '\0') {
			if (!load_file(temp)) {
				msg("New file %s", temp);
			}
			strlcpy(curbp->b_fname, temp, NAME_MAX);
		}
	}
}

void savebuffer(void)
{
	if (curbp->b_fname[0] != '\0') {
		save(curbp->b_fname);
		return;
	} else {
		writefile();
	}
	refresh();
}

void writefile(void)
{
	strlcpy(temp, curbp->b_fname, NAME_MAX);
	if (getinput("Write file: ", temp, NAME_MAX, F_NONE))
		if (save(temp) == TRUE)
			strlcpy(curbp->b_fname, temp, NAME_MAX);
}

void killbuffer(void)
{
	buffer_t *kill_bp = curbp;
	int bcount = count_buffers();

	/* do nothing if only buffer left is the scratch buffer */
	if (bcount == 1 && 0 == strcmp(get_buffer_name(curbp), "*scratch*"))
		return;
	
	if (curbp->b_flags & B_MODIFIED) {
		mvaddstr(MSGLINE, 0, "Discard changes (y/n) ?");
		clrtoeol();
		if (!yesno(FALSE))
			return;
	}

	if (bcount == 1) {
		/* create a scratch buffer */
		create_scratch();
	}

	next_buffer();
	assert(kill_bp != curbp);
	delete_buffer(kill_bp);
}

void iblock(void)
{
	block();
	msg("Mark set");
}

void toggle_overwrite_mode(void) {
	if (curbp->b_flags & B_OVERWRITE)
		curbp->b_flags &= ~B_OVERWRITE;
	else
		curbp->b_flags |= B_OVERWRITE;
}

void killtoeol(void)
{
        if (curbp->b_point == pos(curbp, curbp->b_ebuf))
		return; /* do nothing if at end of file */
	if (*(ptr(curbp, curbp->b_point)) == 0xa) {
		delete(); /* delete CR if at start of empty line */
	} else {
		curbp->b_mark = curbp->b_point;
		lnend();
		if (curbp->b_mark != curbp->b_point) copy_cut(TRUE);
	}
}

void copy_cut(int cut)
{
	char_t *p;
	/* if no mark or point == marker, nothing doing */
	if (curbp->b_mark == NOMARK || curbp->b_point == curbp->b_mark)
		return;
	if (scrap != NULL) {
		free(scrap);
		scrap = NULL;
	}
	if (curbp->b_point < curbp->b_mark) {
		/* point above marker: move gap under point, region = marker - point */
		(void) movegap(curbp, curbp->b_point);
		p = ptr(curbp, curbp->b_point);
		nscrap = curbp->b_mark - curbp->b_point;
	} else {
		/* if point below marker: move gap under marker, region = point - marker */
		(void) movegap(curbp, curbp->b_mark);
		p = ptr(curbp, curbp->b_mark);
		nscrap = curbp->b_point - curbp->b_mark;
	}
	if ((scrap = (char_t*) malloc(nscrap)) == NULL) {
		msg("No more memory available.");
	} else {
		(void) memcpy(scrap, p, nscrap * sizeof (char_t));
		if (cut) {
			curbp->b_egap += nscrap; /* if cut expand gap down */
			curbp->b_point = pos(curbp, curbp->b_egap); /* set point to after region */
			curbp->b_flags |= B_MODIFIED;
			msg("%ld bytes cut.", nscrap);
		} else {
			msg("%ld bytes copied.", nscrap);
		}
		curbp->b_mark = NOMARK;  /* unmark */
	}
}

void paste(void)
{
	if(curbp->b_flags & B_OVERWRITE)
		return;
	if (nscrap <= 0) {
		msg("Scrap is empty.  Nothing to paste.");
	} else if (nscrap < curbp->b_egap - curbp->b_gap || growgap(curbp, nscrap)) {
		curbp->b_point = movegap(curbp, curbp->b_point);
		memcpy(curbp->b_gap, scrap, nscrap * sizeof (char_t));
		curbp->b_gap += nscrap;
		curbp->b_point = pos(curbp, curbp->b_egap);
		curbp->b_flags |= B_MODIFIED;
	}
}

void showpos(void)
{
	int current, lastln;
	point_t end_p = pos(curbp, curbp->b_ebuf);
    
	get_line_stats(&current, &lastln);

	if (curbp->b_point == end_p) {
		msg("[EOB] Line = %d/%d  Point = %ld/%td", current, lastln,
			curbp->b_point, ((curbp->b_ebuf - curbp->b_buf) - (curbp->b_egap - curbp->b_gap)));
	} else {
		msg("Char = %s 0x%x  Line = %d/%d  Point = %ld/%td", unctrl(*(ptr(curbp, curbp->b_point))), *(ptr(curbp, curbp->b_point)), 
			current, lastln,
			curbp->b_point, ((curbp->b_ebuf - curbp->b_buf) - (curbp->b_egap - curbp->b_gap)));
	}
}
