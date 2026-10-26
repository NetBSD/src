/* window.c, Atto Emacs, Hugh Barney, Public Domain, 2015 */

#include "header.h"

int win_cnt = 0;

window_t* new_window(void)
{
	window_t *wp = (window_t *)malloc(sizeof(window_t));
	
	assert(wp != NULL); /* call fatal instead XXX */
	wp->w_next = NULL;
	wp->w_bufp = NULL;
	wp->w_point = 0;
	wp->w_mark = NOMARK;
	wp->w_top = 0;	
	wp->w_rows = 0;	
	wp->w_update = FALSE;
	sprintf(wp->w_name, "W%d", ++win_cnt);
	return wp;
}

void one_window(window_t *wp)
{
	wp->w_top = 0;
	wp->w_rows = LINES - 2;
	wp->w_next = NULL;
}

void split_window(void)
{
	window_t *wp, *wp2;
	int ntru, ntrl;

	if (curwp->w_rows < 3) {
		msg("Cannot split a %d line window", curwp->w_rows);
		return;
	}
	
	wp = new_window();	
	associate_b2w(curwp->w_bufp,wp);
	b2w(wp); /* inherit buffer settings */
  
	ntru = (curwp->w_rows - 1) / 2; /* Upper size */
	ntrl = (curwp->w_rows - 1) - ntru; /* Lower size */

	/* Old is upper window */
	curwp->w_rows = ntru;
	wp->w_top = curwp->w_top + ntru + 1;
	wp->w_rows = ntrl;

	/* insert it in the list */
	wp2 = curwp->w_next;
	curwp->w_next = wp;
	wp->w_next = wp2;
	redraw(); /* mark the lot for update */
}

void next_window(void) {
	curwp->w_update = TRUE; /* make sure modeline gets updated */
	curwp = (curwp->w_next == NULL ? wheadp : curwp->w_next);
	curbp = curwp->w_bufp;
	
	if (curbp->b_cnt > 1)
		w2b(curwp); /* push win vars to buffer */
}

void delete_other_windows(void)
{
	if (wheadp->w_next == NULL) {
		msg("Only 1 window");
		return;
	}
	free_other_windows(curwp);
}
	
void free_other_windows(window_t *winp)
{
	window_t *wp, *next;

	for (wp = next = wheadp; next != NULL; wp = next) {
		next = wp->w_next; /* get next before a call to free() makes wp undefined */
		if (wp != winp) {
			disassociate_b(wp); /* this window no longer references its buffer */
			free(wp);
		}
	}
	
	wheadp = curwp = winp;
	one_window(winp);
}

void associate_b2w(buffer_t *bp, window_t *wp) {
	assert(bp != NULL);
	assert(wp != NULL);
	wp->w_bufp = bp;
	bp->b_cnt++;
}

void disassociate_b(window_t *wp) {
	assert(wp != NULL);
	assert(wp->w_bufp != NULL);
	wp->w_bufp->b_cnt--;
}
