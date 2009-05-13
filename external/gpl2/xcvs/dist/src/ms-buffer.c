/* CVS client logging buffer.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#include <config.h>

#include <stdio.h>

#include "cvs.h"
#include "buffer.h"
#include "ms-buffer.h"

#if defined (SERVER_SUPPORT) || defined (CLIENT_SUPPORT)
#ifdef PROXY_SUPPORT

/* This structure is the closure field of a multi-source buffer.  */
struct ms_buffer
{
    /* Our buffer struct.  */
    struct buffer *buf;

    /* The underlying buffers.  */
    struct buffer *cur;
    List *bufs;

    /* Whether we are in blocking mode or not.  */
    bool block;
};



/* The block function for a multi-source buffer.  */
static int
ms_buffer_block (void *closure, bool block)
{
    struct ms_buffer *mb = closure;

    mb->block = block;
    if (block)
	return set_block (mb->cur);
    else
	return set_nonblock (mb->cur);
}



/* The input function for a log buffer.  */
static int
ms_buffer_input (void *closure, char *data, size_t need, size_t size,
		 size_t *got)
{
    struct ms_buffer *mb = closure;
    int status;

    assert (mb->cur->input);
    status = (*mb->cur->input) (mb->cur->closure, data, need, size, got);
    if (status == -1)
    {
	Node *p;
	/* EOF.  Set up the next buffer in line but return success and no
	 * data since our caller may have selected on the target to find
	 * ready data before calling us.
	 *
	 * If there are no more buffers, return EOF.
	 */
	if (list_isempty (mb->bufs)) return -1;
	buf_shutdown (mb->cur);
	buf_free (mb->cur);
	p = mb->bufs->list->next;
	mb->cur = p->data;
	p->delproc = NULL;
	p->data = NULL;
	delnode (p);
	if (!buf_empty_p (mb->cur)) buf_append_buffer (mb->buf, mb->cur);
	ms_buffer_block (closure, mb->block);
	*got = 0;
	status = 0;
    }

    return status;
}



/* Return the file descriptor underlying any child buffers.  */
static int
ms_buffer_get_fd (void *closure)
{
    struct ms_buffer *mb = closure;
    return buf_get_fd (mb->cur);
}



/* The shutdown function for a multi-source buffer.  */
static int
ms_buffer_shutdown (struct buffer *buf)
{
    struct ms_buffer *mb = buf->closure;
    Node *p;
    int err = 0;

    assert (mb->cur);
    err += buf_shutdown (mb->cur);
    buf_free (mb->cur);
    for (p = mb->bufs->list->next; p != mb->bufs->list; p = p->next)
    {
	assert (p);
	err += buf_shutdown (p->data);
    }

    dellist (&mb->bufs);
    return err;
}



static void
delbuflist (Node *p)
{
    if (p->data)
	buf_free (p->data);
}



/* Create a multi-source buffer.  This could easily be generalized to support
 * any number of source buffers, but for now only two are necessary.
 */
struct buffer *
ms_buffer_initialize (void (*memory) (struct buffer *),
		      struct buffer *buf, struct buffer *buf2/*, ...*/)
{
    struct ms_buffer *mb = xmalloc (sizeof *mb);
    struct buffer *retbuf;
    Node *p;

    mb->block = false;
    mb->cur = buf;
    set_nonblock (buf);
    mb->bufs = getlist ();
    p = getnode ();
    p->data = buf2;
    p->delproc = delbuflist;
    addnode (mb->bufs, p);
    retbuf = buf_initialize (ms_buffer_input, NULL, NULL,
			     ms_buffer_block, ms_buffer_get_fd,
			     ms_buffer_shutdown, memory, mb);
    if (!buf_empty_p (buf)) buf_append_buffer (retbuf, buf);
    mb->buf = retbuf;

    return retbuf;
}
#endif /* PROXY_SUPPORT */
#endif /* defined (SERVER_SUPPORT) || defined (CLIENT_SUPPORT) */
