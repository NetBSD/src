/* buffer.c, Atto Emacs, Hugh Barney, Public Domain, 2015 */

#include <assert.h>
#include <string.h>
#include "header.h"

void buffer_init(buffer_t *bp)
{
	bp->b_mark = NOMARK;
	bp->b_point = 0;
	bp->b_cpoint = 0;
	bp->b_page = 0;
	bp->b_epage = 0;
	bp->b_reframe = 0;
	bp->b_size = 0;
	bp->b_psize = 0;	
	bp->b_flags = 0;
	bp->b_cnt = 0;
	bp->b_buf = NULL;
	bp->b_ebuf = NULL;
	bp->b_gap = NULL;
	bp->b_egap = NULL;
	bp->b_next = NULL;
	bp->b_fname[0] = '\0';
}

/* Find a buffer by filename or create if requested */
buffer_t* find_buffer (const char *fname, int cflag)
{
	buffer_t *bp = NULL;
	buffer_t *sb = NULL;
	
	bp = bheadp;
	while (bp != NULL) {
		if (strcmp (fname, bp->b_fname) == 0 || strcmp(fname, bp->b_bname) == 0) {
			return (bp);
		}
		bp = bp->b_next;
	}

	if (cflag != FALSE) {
		if ((bp = (buffer_t *) malloc (sizeof (buffer_t))) == NULL)
			return (0);

		buffer_init(bp);
		assert(bp != NULL);
		
		/* find the place in the list to insert this buffer */
		if (bheadp == NULL) {
			bheadp = bp;
		} else if (strcmp (bheadp->b_fname, fname) > 0) {
			/* insert at the beginning */
			bp->b_next = bheadp;
			bheadp = bp;
		} else {
			for (sb = bheadp; sb->b_next != NULL; sb = sb->b_next)
				if (strcmp (sb->b_next->b_fname, fname) > 0)
					break;
			
			/* and insert it */
			bp->b_next = sb->b_next;
			sb->b_next = bp;
		}
	}
	return bp;
}

/* unlink from the list of buffers, free associated memory, assumes buffer has been saved if modified */
int delete_buffer (buffer_t *bp)
{
	buffer_t *sb = NULL;

	/* we must have switched to a different buffer first */
	assert(bp != curbp);
	
	/* if buffer is the head buffer */
	if (bp == bheadp) {
		bheadp = bp->b_next;
	} else {
		/* find place where the bp buffer is next */
		for (sb = bheadp; sb->b_next != bp && sb->b_next != NULL; sb = sb->b_next)
			;
		assert(sb->b_next == bp || sb->b_next == NULL);
		sb->b_next = bp->b_next;
	}

	/* now we can delete */
	free(bp->b_buf);
	free(bp);
	return TRUE;
}

void next_buffer(void)
{
	assert(curbp != NULL);
	assert(bheadp != NULL);
	disassociate_b(curwp);	
	curbp = (curbp->b_next != NULL ? curbp->b_next : bheadp);
	associate_b2w(curbp,curwp);
}

char* get_buffer_name(buffer_t *bp)
{
	return (strlen(bp->b_fname) > 0) ? bp->b_fname : bp->b_bname;
}

int count_buffers(void)
{
	buffer_t* bp;
	int i;

	for (i=0, bp=bheadp; bp != NULL; bp = bp->b_next)
		i++;

	return i;
}

int modified_buffers(void)
{
	buffer_t* bp;

	for (bp=bheadp; bp != NULL; bp = bp->b_next)
		if (bp->b_flags & B_MODIFIED)
			return TRUE;

	return FALSE;
}
