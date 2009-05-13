/*
 * Copyright (C) 1996-2005 The Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* Code for the buffer data structure.  */

#include "cvs.h"
#include "buffer.h"
#include "pagealign_alloc.h"

#if defined (SERVER_SUPPORT) || defined (CLIENT_SUPPORT)

# include <sys/socket.h>

/* OS/2 doesn't have EIO.  FIXME: this whole notion of turning
   a different error into EIO strikes me as pretty dubious.  */
# if !defined( EIO )
#   define EIO EBADPOS
# endif

/* Local functions.  */
static void buf_default_memory_error (struct buffer *);
static struct buffer_data *get_buffer_data (void);



/* Initialize a buffer structure.  */
struct buffer *
buf_initialize (type_buf_input input,
                type_buf_output output,
                type_buf_flush flush,
                type_buf_block block,
                type_buf_get_fd get_fd,
                type_buf_shutdown shutdown,
                type_buf_memory_error memory_error,
                void *closure)
{
    struct buffer *buf;

    buf = xmalloc (sizeof (struct buffer));
    buf->data = NULL;
    buf->last = NULL;
    buf->nonblocking = false;
    buf->input = input;
    buf->output = output;
    buf->flush = flush;
    buf->block = block;
    buf->get_fd = get_fd;
    buf->shutdown = shutdown;
    buf->memory_error = memory_error ? memory_error : buf_default_memory_error;
    buf->closure = closure;
    return buf;
}



/* Free a buffer structure.  */
void
buf_free (struct buffer *buf)
{
    if (buf->closure != NULL)
    {
	free (buf->closure);
	buf->closure = NULL;
    }
    buf_free_data (buf);
    free (buf);
}



/* Initialize a buffer structure which is not to be used for I/O.  */
struct buffer *
buf_nonio_initialize( void (*memory) (struct buffer *) )
{
    return buf_initialize (NULL, NULL, NULL, NULL, NULL, NULL, memory, NULL);
}



/* Default memory error handler.  */
static void
buf_default_memory_error (struct buffer *buf)
{
    error (1, 0, "out of memory");
}



/* Allocate more buffer_data structures.  */
/* Get a new buffer_data structure.  */
static struct buffer_data *
get_buffer_data (void)
{
    struct buffer_data *ret;

    ret = xmalloc (sizeof (struct buffer_data));
    ret->text = pagealign_xalloc (BUFFER_DATA_SIZE);

    return ret;
}



/* See whether a buffer and its file descriptor is empty.  */
int
buf_empty (buf)
    struct buffer *buf;
{
	/* Try and read any data on the file descriptor first.
	 * We already know the descriptor is non-blocking.
	 */
	buf_input_data (buf, NULL);
	return buf_empty_p (buf);
}



/* See whether a buffer is empty.  */
int
buf_empty_p (struct buffer *buf)
{
    struct buffer_data *data;

    for (data = buf->data; data != NULL; data = data->next)
	if (data->size > 0)
	    return 0;
    return 1;
}



# if defined (SERVER_FLOWCONTROL) || defined (PROXY_SUPPORT)
/*
 * Count how much data is stored in the buffer..
 * Note that each buffer is a xmalloc'ed chunk BUFFER_DATA_SIZE.
 */
int
buf_count_mem (struct buffer *buf)
{
    struct buffer_data *data;
    int mem = 0;

    for (data = buf->data; data != NULL; data = data->next)
	mem += BUFFER_DATA_SIZE;

    return mem;
}
# endif /* SERVER_FLOWCONTROL || PROXY_SUPPORT */



/* Add data DATA of length LEN to BUF.  */
void
buf_output (struct buffer *buf, const char *data, size_t len)
{
    if (buf->data != NULL
	&& (((buf->last->text + BUFFER_DATA_SIZE)
	     - (buf->last->bufp + buf->last->size))
	    >= len))
    {
	memcpy (buf->last->bufp + buf->last->size, data, len);
	buf->last->size += len;
	return;
    }

    while (1)
    {
	struct buffer_data *newdata;

	newdata = get_buffer_data ();
	if (newdata == NULL)
	{
	    (*buf->memory_error) (buf);
	    return;
	}

	if (buf->data == NULL)
	    buf->data = newdata;
	else
	    buf->last->next = newdata;
	newdata->next = NULL;
	buf->last = newdata;

	newdata->bufp = newdata->text;

	if (len <= BUFFER_DATA_SIZE)
	{
	    newdata->size = len;
	    memcpy (newdata->text, data, len);
	    return;
	}

	newdata->size = BUFFER_DATA_SIZE;
	memcpy (newdata->text, data, BUFFER_DATA_SIZE);

	data += BUFFER_DATA_SIZE;
	len -= BUFFER_DATA_SIZE;
    }

    /*NOTREACHED*/
}



/* Add a '\0' terminated string to BUF.  */
void
buf_output0 (struct buffer *buf, const char *string)
{
    buf_output (buf, string, strlen (string));
}



/* Add a single character to BUF.  */
void
buf_append_char (struct buffer *buf, int ch)
{
    if (buf->data != NULL
	&& (buf->last->text + BUFFER_DATA_SIZE
	    != buf->last->bufp + buf->last->size))
    {
	*(buf->last->bufp + buf->last->size) = ch;
	++buf->last->size;
    }
    else
    {
	char b;

	b = ch;
	buf_output (buf, &b, 1);
    }
}



/* Free struct buffer_data's from the list starting with FIRST and ending at
 * LAST, inclusive.
 */
static inline void
buf_free_datas (struct buffer_data *first, struct buffer_data *last)
{
    struct buffer_data *b, *n, *p;
    b = first;
    do
    {
	p = b;
	n = b->next;
	pagealign_free (b->text);
	free (b);
	b = n;
    } while (p != last);
}



/*
 * Send all the output we've been saving up.  Returns 0 for success or
 * errno code.  If the buffer has been set to be nonblocking, this
 * will just write until the write would block.
 */
int
buf_send_output (struct buffer *buf)
{
    assert (buf->output != NULL);

    while (buf->data != NULL)
    {
	struct buffer_data *data;

	data = buf->data;

	if (data->size > 0)
	{
	    int status;
	    size_t nbytes;

	    status = (*buf->output) (buf->closure, data->bufp, data->size,
				     &nbytes);
	    if (status != 0)
	    {
		/* Some sort of error.  Discard the data, and return.  */
		buf_free_data (buf);
	        return status;
	    }

	    if (nbytes != data->size)
	    {
		/* Not all the data was written out.  This is only
                   permitted in nonblocking mode.  Adjust the buffer,
                   and return.  */

		assert (buf->nonblocking);

		data->size -= nbytes;
		data->bufp += nbytes;

		return 0;
	    }
	}

	buf->data = data->next;
	buf_free_datas (data, data);
    }

    buf->last = NULL;

    return 0;
}



/*
 * Flush any data queued up in the buffer.  If BLOCK is nonzero, then
 * if the buffer is in nonblocking mode, put it into blocking mode for
 * the duration of the flush.  This returns 0 on success, or an error
 * code.
 */
int
buf_flush (struct buffer *buf, bool block)
{
    int nonblocking;
    int status;

    assert (buf->flush != NULL);

    nonblocking = buf->nonblocking;
    if (nonblocking && block)
    {
        status = set_block (buf);
	if (status != 0)
	    return status;
    }

    status = buf_send_output (buf);
    if (status == 0)
        status = (*buf->flush) (buf->closure);

    if (nonblocking && block)
    {
        int blockstat;

        blockstat = set_nonblock (buf);
	if (status == 0)
	    status = blockstat;
    }

    return status;
}



/*
 * Set buffer BUF to nonblocking I/O.  Returns 0 for success or errno
 * code.
 */
int
set_nonblock (struct buffer *buf)
{
    int status;

    if (buf->nonblocking)
	return 0;
    assert (buf->block != NULL);
    status = (*buf->block) (buf->closure, 0);
    if (status != 0)
	return status;
    buf->nonblocking = true;
    return 0;
}



/*
 * Set buffer BUF to blocking I/O.  Returns 0 for success or errno
 * code.
 */
int
set_block (struct buffer *buf)
{
    int status;

    if (! buf->nonblocking)
	return 0;
    assert (buf->block != NULL);
    status = (*buf->block) (buf->closure, 1);
    if (status != 0)
	return status;
    buf->nonblocking = false;
    return 0;
}



/*
 * Send a character count and some output.  Returns errno code or 0 for
 * success.
 *
 * Sending the count in binary is OK since this is only used on a pipe
 * within the same system.
 */
int
buf_send_counted (struct buffer *buf)
{
    int size;
    struct buffer_data *data;

    size = 0;
    for (data = buf->data; data != NULL; data = data->next)
	size += data->size;

    data = get_buffer_data ();
    if (data == NULL)
    {
	(*buf->memory_error) (buf);
	return ENOMEM;
    }

    data->next = buf->data;
    buf->data = data;
    if (buf->last == NULL)
	buf->last = data;

    data->bufp = data->text;
    data->size = sizeof (int);

    *((int *) data->text) = size;

    return buf_send_output (buf);
}



/*
 * Send a special count.  COUNT should be negative.  It will be
 * handled specially by buf_copy_counted.  This function returns 0 or
 * an errno code.
 *
 * Sending the count in binary is OK since this is only used on a pipe
 * within the same system.
 */
int
buf_send_special_count (struct buffer *buf, int count)
{
    struct buffer_data *data;

    data = get_buffer_data ();
    if (data == NULL)
    {
	(*buf->memory_error) (buf);
	return ENOMEM;
    }

    data->next = buf->data;
    buf->data = data;
    if (buf->last == NULL)
	buf->last = data;

    data->bufp = data->text;
    data->size = sizeof (int);

    *((int *) data->text) = count;

    return buf_send_output (buf);
}



/* Append a list of buffer_data structures to an buffer.  */
void
buf_append_data (struct buffer *buf, struct buffer_data *data,
                 struct buffer_data *last)
{
    if (data != NULL)
    {
	if (buf->data == NULL)
	    buf->data = data;
	else
	    buf->last->next = data;
	buf->last = last;
    }
}



# ifdef PROXY_SUPPORT
/* Copy data structures and append them to a buffer.
 *
 * ERRORS
 *   Failure to allocate memory here is fatal.
 */
void
buf_copy_data (struct buffer *buf, struct buffer_data *data,
               struct buffer_data *last)
{
    struct buffer_data *first, *new, *cur, *prev;

    assert (buf);
    assert (data);

    prev = first = NULL;
    cur = data;
    while (1)
    {
	new = get_buffer_data ();
	if (!new) error (1, errno, "Failed to allocate buffer data.");

	if (!first) first = new;
	memcpy (new->text, cur->bufp, cur->size);
	new->bufp = new->text;
	new->size = cur->size;
	new->next = NULL;
	if (prev) prev->next = new;
	if (cur == last) break;
	prev = new;
	cur = cur->next;
    }

    buf_append_data (buf, first, new);
}
# endif /* PROXY_SUPPORT */



/* Dispose of any remaining data in the buffer.  */
void
buf_free_data (struct buffer *buffer)
{
    if (buf_empty_p (buffer)) return;
    buf_free_datas (buffer->data, buffer->last);
    buffer->data = buffer->last = NULL;
}



/* Append the data in one buffer to another.  This removes the data
 * from the source buffer.
 */
void
buf_append_buffer (struct buffer *to, struct buffer *from)
{
    struct buffer_data *n;

    /* Copy the data pointer to the new buf.  */
    buf_append_data (to, from->data, from->last);

    n = from->data;
    while (n)
    {
	if (n == from->last) break;
	n = n->next;
    }

    /* Remove from the original location.  */
    from->data = NULL;
    from->last = NULL;
}



/*
 * Copy the contents of file F into buffer_data structures.  We can't
 * copy directly into an buffer, because we want to handle failure and
 * success differently.  Returns 0 on success, or -2 if out of
 * memory, or a status code on error.  Since the caller happens to
 * know the size of the file, it is passed in as SIZE.  On success,
 * this function sets *RETP and *LASTP, which may be passed to
 * buf_append_data.
 */
int
buf_read_file (FILE *f, long int size, struct buffer_data **retp,
               struct buffer_data **lastp)
{
    int status;

    *retp = NULL;
    *lastp = NULL;

    while (size > 0)
    {
	struct buffer_data *data;
	int get;

	data = get_buffer_data ();
	if (data == NULL)
	{
	    status = -2;
	    goto error_return;
	}

	if (*retp == NULL)
	    *retp = data;
	else
	    (*lastp)->next = data;
	data->next = NULL;
	*lastp = data;

	data->bufp = data->text;
	data->size = 0;

	if (size > BUFFER_DATA_SIZE)
	    get = BUFFER_DATA_SIZE;
	else
	    get = size;

	errno = EIO;
	if (fread (data->text, get, 1, f) != 1)
	{
	    status = errno;
	    goto error_return;
	}

	data->size += get;
	size -= get;
    }

    return 0;

  error_return:
    if (*retp != NULL)
	buf_free_datas (*retp, (*lastp)->next);
    return status;
}



/*
 * Copy the contents of file F into buffer_data structures.  We can't
 * copy directly into an buffer, because we want to handle failure and
 * success differently.  Returns 0 on success, or -2 if out of
 * memory, or a status code on error.  On success, this function sets
 * *RETP and *LASTP, which may be passed to buf_append_data.
 */
int
buf_read_file_to_eof (FILE *f, struct buffer_data **retp,
                      struct buffer_data **lastp)
{
    int status;

    *retp = NULL;
    *lastp = NULL;

    while (!feof (f))
    {
	struct buffer_data *data;
	int get, nread;

	data = get_buffer_data ();
	if (data == NULL)
	{
	    status = -2;
	    goto error_return;
	}

	if (*retp == NULL)
	    *retp = data;
	else
	    (*lastp)->next = data;
	data->next = NULL;
	*lastp = data;

	data->bufp = data->text;
	data->size = 0;

	get = BUFFER_DATA_SIZE;

	errno = EIO;
	nread = fread (data->text, 1, get, f);
	if (nread == 0 && !feof (f))
	{
	    status = errno;
	    goto error_return;
	}

	data->size = nread;
    }

    return 0;

  error_return:
    if (*retp != NULL)
	buf_free_datas (*retp, (*lastp)->next);
    return status;
}



/* Return the number of bytes in a chain of buffer_data structures.  */
int
buf_chain_length (struct buffer_data *buf)
{
    int size = 0;
    while (buf)
    {
	size += buf->size;
	buf = buf->next;
    }
    return size;
}



/* Return the number of bytes in a buffer.  */
int
buf_length (struct buffer *buf)
{
    return buf_chain_length (buf->data);
}



/*
 * Read an arbitrary amount of data into an input buffer.  The buffer
 * will be in nonblocking mode, and we just grab what we can.  Return
 * 0 on success, or -1 on end of file, or -2 if out of memory, or an
 * error code.  If COUNTP is not NULL, *COUNTP is set to the number of
 * bytes read.
 */
int
buf_input_data (struct buffer *buf, size_t *countp)
{
    assert (buf->input != NULL);

    if (countp != NULL)
	*countp = 0;

    while (1)
    {
	int status;
	size_t get, nbytes;

	if (buf->data == NULL
	    || (buf->last->bufp + buf->last->size
		== buf->last->text + BUFFER_DATA_SIZE))
	{
	    struct buffer_data *data;

	    data = get_buffer_data ();
	    if (data == NULL)
	    {
		(*buf->memory_error) (buf);
		return -2;
	    }

	    if (buf->data == NULL)
		buf->data = data;
	    else
		buf->last->next = data;
	    data->next = NULL;
	    buf->last = data;

	    data->bufp = data->text;
	    data->size = 0;
	}

	get = ((buf->last->text + BUFFER_DATA_SIZE)
	       - (buf->last->bufp + buf->last->size));

	status = (*buf->input) (buf->closure,
				buf->last->bufp + buf->last->size,
				0, get, &nbytes);
	if (status != 0)
	    return status;

	buf->last->size += nbytes;
	if (countp != NULL)
	    *countp += nbytes;

	if (nbytes < get)
	{
	    /* If we did not fill the buffer, then presumably we read
               all the available data.  */
	    return 0;
	}
    }

    /*NOTREACHED*/
}



/*
 * Read a line (characters up to a \012) from an input buffer.  (We
 * use \012 rather than \n for the benefit of non Unix clients for
 * which \n means something else).  This returns 0 on success, or -1
 * on end of file, or -2 if out of memory, or an error code.  If it
 * succeeds, it sets *LINE to an allocated buffer holding the contents
 * of the line.  The trailing \012 is not included in the buffer.  If
 * LENP is not NULL, then *LENP is set to the number of bytes read;
 * strlen may not work, because there may be embedded null bytes.
 */
int
buf_read_line (struct buffer *buf, char **line, size_t *lenp)
{
    return buf_read_short_line (buf, line, lenp, SIZE_MAX);
}



/* Like buf_read_line, but return -2 if no newline is found in MAX characters.
 */
int
buf_read_short_line (struct buffer *buf, char **line, size_t *lenp,
                     size_t max)
{
    assert (buf->input != NULL);

    *line = NULL;

    while (1)
    {
	size_t len, finallen, predicted_len;
	struct buffer_data *data;
	char *nl;

	/* See if there is a newline in BUF.  */
	len = 0;
	for (data = buf->data; data != NULL; data = data->next)
	{
	    nl = memchr (data->bufp, '\012', data->size);
	    if (nl != NULL)
	    {
	        finallen = nl - data->bufp;
		if (xsum (len, finallen) >= max) return -2;
	        len += finallen;
		break;
	    }
	    else if (xsum (len, data->size) >= max) return -2;
	    len += data->size;
	}

	/* If we found a newline, copy the line into a memory buffer,
           and remove it from BUF.  */
	if (data != NULL)
	{
	    char *p;
	    struct buffer_data *nldata;

	    p = xmalloc (len + 1);
	    if (p == NULL)
		return -2;
	    *line = p;

	    nldata = data;
	    data = buf->data;
	    while (data != nldata)
	    {
		struct buffer_data *next;

		memcpy (p, data->bufp, data->size);
		p += data->size;
		next = data->next;
		buf_free_datas (data, data);
		data = next;
	    }

	    memcpy (p, data->bufp, finallen);
	    p[finallen] = '\0';

	    data->size -= finallen + 1;
	    data->bufp = nl + 1;
	    buf->data = data;

	    if (lenp != NULL)
	        *lenp = len;

	    return 0;
	}

	/* Read more data until we get a newline or MAX characters.  */
	predicted_len = 0;
	while (1)
	{
	    int status;
	    size_t size, nbytes;
	    char *mem;

	    if (buf->data == NULL
		|| (buf->last->bufp + buf->last->size
		    == buf->last->text + BUFFER_DATA_SIZE))
	    {
		data = get_buffer_data ();
		if (data == NULL)
		{
		    (*buf->memory_error) (buf);
		    return -2;
		}

		if (buf->data == NULL)
		    buf->data = data;
		else
		    buf->last->next = data;
		data->next = NULL;
		buf->last = data;

		data->bufp = data->text;
		data->size = 0;
	    }

	    mem = buf->last->bufp + buf->last->size;
	    size = (buf->last->text + BUFFER_DATA_SIZE) - mem;

	    /* We need to read at least 1 byte.  We can handle up to
               SIZE bytes.  This will only be efficient if the
               underlying communication stream does its own buffering,
               or is clever about getting more than 1 byte at a time.  */
	    status = (*buf->input) (buf->closure, mem, 1, size, &nbytes);
	    if (status != 0)
		return status;

	    predicted_len += nbytes;
	    buf->last->size += nbytes;

	    /* Optimize slightly to avoid an unnecessary call to memchr.  */
	    if (nbytes == 1)
	    {
		if (*mem == '\012')
		    break;
	    }
	    else
	    {
		if (memchr (mem, '\012', nbytes) != NULL)
		    break;
	    }
	    if (xsum (len, predicted_len) >= max) return -2;
	}
    }
}



/*
 * Extract data from the input buffer BUF.  This will read up to WANT
 * bytes from the buffer.  It will set *RETDATA to point at the bytes,
 * and set *GOT to the number of bytes to be found there.  Any buffer
 * call which uses BUF may change the contents of the buffer at *DATA,
 * so the data should be fully processed before any further calls are
 * made.  This returns 0 on success, or -1 on end of file, or -2 if
 * out of memory, or an error code.
 */
int
buf_read_data (struct buffer *buf, size_t want, char **retdata, size_t *got)
{
    assert (buf->input != NULL);

    while (buf->data != NULL && buf->data->size == 0)
    {
	struct buffer_data *next;

	next = buf->data->next;
	buf_free_datas (buf->data, buf->data);
	buf->data = next;
	if (next == NULL)
	    buf->last = NULL;
    }

    if (buf->data == NULL)
    {
	struct buffer_data *data;
	int status;
	size_t get, nbytes;

	data = get_buffer_data ();
	if (data == NULL)
	{
	    (*buf->memory_error) (buf);
	    return -2;
	}

	buf->data = data;
	buf->last = data;
	data->next = NULL;
	data->bufp = data->text;
	data->size = 0;

	if (want < BUFFER_DATA_SIZE)
	    get = want;
	else
	    get = BUFFER_DATA_SIZE;
	status = (*buf->input) (buf->closure, data->bufp, get,
				BUFFER_DATA_SIZE, &nbytes);
	if (status != 0)
	    return status;

	data->size = nbytes;
    }

    *retdata = buf->data->bufp;
    if (want < buf->data->size)
    {
        *got = want;
	buf->data->size -= want;
	buf->data->bufp += want;
    }
    else
    {
        *got = buf->data->size;
	buf->data->size = 0;
    }

    return 0;
}



/*
 * Copy lines from an input buffer to an output buffer.
 * This copies all complete lines (characters up to a
 * newline) from INBUF to OUTBUF.  Each line in OUTBUF is preceded by the
 * character COMMAND and a space.
 */
void
buf_copy_lines (struct buffer *outbuf, struct buffer *inbuf, int command)
{
    while (1)
    {
	struct buffer_data *data;
	struct buffer_data *nldata;
	char *nl;
	int len;

	/* See if there is a newline in INBUF.  */
	nldata = NULL;
	nl = NULL;
	for (data = inbuf->data; data != NULL; data = data->next)
	{
	    nl = memchr (data->bufp, '\n', data->size);
	    if (nl != NULL)
	    {
		nldata = data;
		break;
	    }
	}

	if (nldata == NULL)
	{
	    /* There are no more lines in INBUF.  */
	    return;
	}

	/* Put in the command.  */
	buf_append_char (outbuf, command);
	buf_append_char (outbuf, ' ');

	if (inbuf->data != nldata)
	{
	    /*
	     * Simply move over all the buffers up to the one containing
	     * the newline.
	     */
	    for (data = inbuf->data; data->next != nldata; data = data->next);
	    data->next = NULL;
	    buf_append_data (outbuf, inbuf->data, data);
	    inbuf->data = nldata;
	}

	/*
	 * If the newline is at the very end of the buffer, just move
	 * the buffer onto OUTBUF.  Otherwise we must copy the data.
	 */
	len = nl + 1 - nldata->bufp;
	if (len == nldata->size)
	{
	    inbuf->data = nldata->next;
	    if (inbuf->data == NULL)
		inbuf->last = NULL;

	    nldata->next = NULL;
	    buf_append_data (outbuf, nldata, nldata);
	}
	else
	{
	    buf_output (outbuf, nldata->bufp, len);
	    nldata->bufp += len;
	    nldata->size -= len;
	}
    }
}



/*
 * Copy counted data from one buffer to another.  The count is an
 * integer, host size, host byte order (it is only used across a
 * pipe).  If there is enough data, it should be moved over.  If there
 * is not enough data, it should remain on the original buffer.  A
 * negative count is a special case.  if one is seen, *SPECIAL is set
 * to the (negative) count value and no additional data is gathered
 * from the buffer; normally *SPECIAL is set to 0.  This function
 * returns the number of bytes it needs to see in order to actually
 * copy something over.
 */
int
buf_copy_counted (struct buffer *outbuf, struct buffer *inbuf, int *special)
{
    *special = 0;

    while (1)
    {
	struct buffer_data *data;
	int need;
	union
	{
	    char intbuf[sizeof (int)];
	    int i;
	} u;
	char *intp;
	int count;
	struct buffer_data *start;
	int startoff;
	struct buffer_data *stop;
	int stopwant;

	/* See if we have enough bytes to figure out the count.  */
	need = sizeof (int);
	intp = u.intbuf;
	for (data = inbuf->data; data != NULL; data = data->next)
	{
	    if (data->size >= need)
	    {
		memcpy (intp, data->bufp, need);
		break;
	    }
	    memcpy (intp, data->bufp, data->size);
	    intp += data->size;
	    need -= data->size;
	}
	if (data == NULL)
	{
	    /* We don't have enough bytes to form an integer.  */
	    return need;
	}

	count = u.i;
	start = data;
	startoff = need;

	if (count < 0)
	{
	    /* A negative COUNT is a special case meaning that we
               don't need any further information.  */
	    stop = start;
	    stopwant = 0;
	}
	else
	{
	    /*
	     * We have an integer in COUNT.  We have gotten all the
	     * data from INBUF in all buffers before START, and we
	     * have gotten STARTOFF bytes from START.  See if we have
	     * enough bytes remaining in INBUF.
	     */
	    need = count - (start->size - startoff);
	    if (need <= 0)
	    {
		stop = start;
		stopwant = count;
	    }
	    else
	    {
		for (data = start->next; data != NULL; data = data->next)
		{
		    if (need <= data->size)
			break;
		    need -= data->size;
		}
		if (data == NULL)
		{
		    /* We don't have enough bytes.  */
		    return need;
		}
		stop = data;
		stopwant = need;
	    }
	}

	/*
	 * We have enough bytes.  Free any buffers in INBUF before
	 * START, and remove STARTOFF bytes from START, so that we can
	 * forget about STARTOFF.
	 */
	start->bufp += startoff;
	start->size -= startoff;

	if (start->size == 0)
	    start = start->next;

	if (stop->size == stopwant)
	{
	    stop = stop->next;
	    stopwant = 0;
	}

	while (inbuf->data != start)
	{
	    data = inbuf->data;
	    inbuf->data = data->next;
	    buf_free_datas (data, data);
	}

	/* If COUNT is negative, set *SPECIAL and get out now.  */
	if (count < 0)
	{
	    *special = count;
	    return 0;
	}

	/*
	 * We want to copy over the bytes from START through STOP.  We
	 * only want STOPWANT bytes from STOP.
	 */

	if (start != stop)
	{
	    /* Attach the buffers from START through STOP to OUTBUF.  */
	    for (data = start; data->next != stop; data = data->next);
	    inbuf->data = stop;
	    data->next = NULL;
	    buf_append_data (outbuf, start, data);
	}

	if (stopwant > 0)
	{
	    buf_output (outbuf, stop->bufp, stopwant);
	    stop->bufp += stopwant;
	    stop->size -= stopwant;
	}
    }

    /*NOTREACHED*/
}



int
buf_get_fd (struct buffer *buf)
{
    if (buf->get_fd)
	return (*buf->get_fd) (buf->closure);
    return -1;
}



/* Shut down a buffer.  This returns 0 on success, or an errno code.  */
int
buf_shutdown (struct buffer *buf)
{
    if (buf->shutdown) return (*buf->shutdown) (buf);
    return 0;
}



/* Certain types of communication input and output data in packets,
   where each packet is translated in some fashion.  The packetizing
   buffer type supports that, given a buffer which handles lower level
   I/O and a routine to translate the data in a packet.

   This code uses two bytes for the size of a packet, so packets are
   restricted to 65536 bytes in total.

   The translation functions should just translate; they may not
   significantly increase or decrease the amount of data.  The actual
   size of the initial data is part of the translated data.  The
   output translation routine may add up to PACKET_SLOP additional
   bytes, and the input translation routine should shrink the data
   correspondingly.  */

# define PACKET_SLOP (100)

/* This structure is the closure field of a packetizing buffer.  */

struct packetizing_buffer
{
    /* The underlying buffer.  */
    struct buffer *buf;
    /* The input translation function.  Exactly one of inpfn and outfn
       will be NULL.  The input translation function should
       untranslate the data in INPUT, storing the result in OUTPUT.
       SIZE is the amount of data in INPUT, and is also the size of
       OUTPUT.  This should return 0 on success, or an errno code.  */
    int (*inpfn) (void *fnclosure, const char *input, char *output,
			size_t size);
    /* The output translation function.  This should translate the
       data in INPUT, storing the result in OUTPUT.  The first two
       bytes in INPUT will be the size of the data, and so will SIZE.
       This should set *TRANSLATED to the amount of translated data in
       OUTPUT.  OUTPUT is large enough to hold SIZE + PACKET_SLOP
       bytes.  This should return 0 on success, or an errno code.  */
    int (*outfn) (void *fnclosure, const char *input, char *output,
			size_t size, size_t *translated);
    /* A closure for the translation function.  */
    void *fnclosure;
    /* For an input buffer, we may have to buffer up data here.  */
    /* This is non-zero if the buffered data has been translated.
       Otherwise, the buffered data has not been translated, and starts
       with the two byte packet size.  */
    bool translated;
    /* The amount of buffered data.  */
    size_t holdsize;
    /* The buffer allocated to hold the data.  */
    char *holdbuf;
    /* The size of holdbuf.  */
    size_t holdbufsize;
    /* If translated is set, we need another data pointer to track
       where we are in holdbuf.  If translated is clear, then this
       pointer is not used.  */
    char *holddata;
};



static int packetizing_buffer_input (void *, char *, size_t, size_t, size_t *);
static int packetizing_buffer_output (void *, const char *, size_t, size_t *);
static int packetizing_buffer_flush (void *);
static int packetizing_buffer_block (void *, bool);
static int packetizing_buffer_get_fd (void *);
static int packetizing_buffer_shutdown (struct buffer *);



/* Create a packetizing buffer.  */
struct buffer *
packetizing_buffer_initialize (struct buffer *buf,
                               int (*inpfn) (void *, const char *, char *,
                                             size_t),
                               int (*outfn) (void *, const char *, char *,
                                             size_t, size_t *),
                               void *fnclosure,
                               void (*memory) (struct buffer *))
{
    struct packetizing_buffer *pb;

    pb = xmalloc (sizeof *pb);
    memset (pb, 0, sizeof *pb);

    pb->buf = buf;
    pb->inpfn = inpfn;
    pb->outfn = outfn;
    pb->fnclosure = fnclosure;

    if (inpfn != NULL)
    {
	/* Add PACKET_SLOP to handle larger translated packets, and
           add 2 for the count.  This buffer is increased if
           necessary.  */
	pb->holdbufsize = BUFFER_DATA_SIZE + PACKET_SLOP + 2;
	pb->holdbuf = xmalloc (pb->holdbufsize);
    }

    return buf_initialize (inpfn != NULL ? packetizing_buffer_input : NULL,
			   inpfn != NULL ? NULL : packetizing_buffer_output,
			   inpfn != NULL ? NULL : packetizing_buffer_flush,
			   packetizing_buffer_block,
			   packetizing_buffer_get_fd,
			   packetizing_buffer_shutdown,
			   memory,
			   pb);
}



/* Input data from a packetizing buffer.  */
static int
packetizing_buffer_input (void *closure, char *data, size_t need, size_t size,
                          size_t *got)
{
    struct packetizing_buffer *pb = closure;

    *got = 0;

    if (pb->holdsize > 0 && pb->translated)
    {
	size_t copy;

	copy = pb->holdsize;

	if (copy > size)
	{
	    memcpy (data, pb->holddata, size);
	    pb->holdsize -= size;
	    pb->holddata += size;
	    *got = size;
	    return 0;
	}

	memcpy (data, pb->holddata, copy);
	pb->holdsize = 0;
	pb->translated = false;

	data += copy;
	need -= copy;
	size -= copy;
	*got = copy;
    }

    while (need > 0 || *got == 0)
    {
	int status;
	size_t get, nread, count, tcount;
	char *bytes;
	static char *stackoutbuf = NULL;
	char *inbuf, *outbuf;

	if (!stackoutbuf)
	    stackoutbuf = xmalloc (BUFFER_DATA_SIZE + PACKET_SLOP);

	/* If we don't already have the two byte count, get it.  */
	if (pb->holdsize < 2)
	{
	    get = 2 - pb->holdsize;
	    status = buf_read_data (pb->buf, get, &bytes, &nread);
	    if (status != 0)
	    {
		/* buf_read_data can return -2, but a buffer input
                   function is only supposed to return -1, 0, or an
                   error code.  */
		if (status == -2)
		    status = ENOMEM;
		return status;
	    }

	    if (nread == 0)
	    {
		/* The buffer is in nonblocking mode, and we didn't
                   manage to read anything.  */
		return 0;
	    }

	    if (get == 1)
		pb->holdbuf[1] = bytes[0];
	    else
	    {
		pb->holdbuf[0] = bytes[0];
		if (nread < 2)
		{
		    /* We only got one byte, but we needed two.  Stash
                       the byte we got, and try again.  */
		    pb->holdsize = 1;
		    continue;
		}
		pb->holdbuf[1] = bytes[1];
	    }
	    pb->holdsize = 2;
	}

	/* Read the packet.  */

	count = (((pb->holdbuf[0] & 0xff) << 8)
		 + (pb->holdbuf[1] & 0xff));

	if (count + 2 > pb->holdbufsize)
	{
	    char *n;

	    /* We didn't allocate enough space in the initialize
               function.  */

	    n = xrealloc (pb->holdbuf, count + 2);
	    if (n == NULL)
	    {
		(*pb->buf->memory_error) (pb->buf);
		return ENOMEM;
	    }
	    pb->holdbuf = n;
	    pb->holdbufsize = count + 2;
	}

	get = count - (pb->holdsize - 2);

	status = buf_read_data (pb->buf, get, &bytes, &nread);
	if (status != 0)
	{
	    /* buf_read_data can return -2, but a buffer input
               function is only supposed to return -1, 0, or an error
               code.  */
	    if (status == -2)
		status = ENOMEM;
	    return status;
	}

	if (nread == 0)
	{
	    /* We did not get any data.  Presumably the buffer is in
               nonblocking mode.  */
	    return 0;
	}

	if (nread < get)
	{
	    /* We did not get all the data we need to fill the packet.
               buf_read_data does not promise to return all the bytes
               requested, so we must try again.  */
	    memcpy (pb->holdbuf + pb->holdsize, bytes, nread);
	    pb->holdsize += nread;
	    continue;
	}

	/* We have a complete untranslated packet of COUNT bytes.  */

	if (pb->holdsize == 2)
	{
	    /* We just read the entire packet (the 2 bytes in
               PB->HOLDBUF are the size).  Save a memcpy by
               translating directly from BYTES.  */
	    inbuf = bytes;
	}
	else
	{
	    /* We already had a partial packet in PB->HOLDBUF.  We
               need to copy the new data over to make the input
               contiguous.  */
	    memcpy (pb->holdbuf + pb->holdsize, bytes, nread);
	    inbuf = pb->holdbuf + 2;
	}

	if (count <= BUFFER_DATA_SIZE + PACKET_SLOP)
	    outbuf = stackoutbuf;
	else
	{
	    outbuf = xmalloc (count);
	    if (outbuf == NULL)
	    {
		(*pb->buf->memory_error) (pb->buf);
		return ENOMEM;
	    }
	}

	status = (*pb->inpfn) (pb->fnclosure, inbuf, outbuf, count);
	if (status != 0)
	    return status;

	/* The first two bytes in the translated buffer are the real
           length of the translated data.  */
	tcount = ((outbuf[0] & 0xff) << 8) + (outbuf[1] & 0xff);

	if (tcount > count)
	    error (1, 0, "Input translation failure");

	if (tcount > size)
	{
	    /* We have more data than the caller has provided space
               for.  We need to save some of it for the next call.  */

	    memcpy (data, outbuf + 2, size);
	    *got += size;

	    pb->holdsize = tcount - size;
	    memcpy (pb->holdbuf, outbuf + 2 + size, tcount - size);
	    pb->holddata = pb->holdbuf;
	    pb->translated = true;

	    if (outbuf != stackoutbuf)
		free (outbuf);

	    return 0;
	}

	memcpy (data, outbuf + 2, tcount);

	if (outbuf != stackoutbuf)
	    free (outbuf);

	pb->holdsize = 0;

	data += tcount;
	need -= tcount;
	size -= tcount;
	*got += tcount;
    }

    return 0;
}



/* Output data to a packetizing buffer.  */
static int
packetizing_buffer_output (void *closure, const char *data, size_t have,
                           size_t *wrote)
{
    struct packetizing_buffer *pb = closure;
    static char *inbuf = NULL;  /* These two buffers are static so that they
				 * depend on the size of BUFFER_DATA_SIZE yet
				 * still only be allocated once per run.
				 */
    static char *stack_outbuf = NULL;
    struct buffer_data *outdata = NULL; /* Initialize to silence -Wall.  Dumb.
					 */
    char *outbuf;
    size_t size, translated;
    int status;

    /* It would be easy to xmalloc a buffer, but I don't think this
       case can ever arise.  */
    assert (have <= BUFFER_DATA_SIZE);

    if (!inbuf)
    {
	inbuf = xmalloc (BUFFER_DATA_SIZE + 2);
        stack_outbuf = xmalloc (BUFFER_DATA_SIZE + PACKET_SLOP + 4);
    }

    inbuf[0] = (have >> 8) & 0xff;
    inbuf[1] = have & 0xff;
    memcpy (inbuf + 2, data, have);

    size = have + 2;

    /* The output function is permitted to add up to PACKET_SLOP
       bytes, and we need 2 bytes for the size of the translated data.
       If we can guarantee that the result will fit in a buffer_data,
       we translate directly into one to avoid a memcpy in buf_output.  */
    if (size + PACKET_SLOP + 2 > BUFFER_DATA_SIZE)
	outbuf = stack_outbuf;
    else
    {
	outdata = get_buffer_data ();
	if (outdata == NULL)
	{
	    (*pb->buf->memory_error) (pb->buf);
	    return ENOMEM;
	}

	outdata->next = NULL;
	outdata->bufp = outdata->text;

	outbuf = outdata->text;
    }

    status = (*pb->outfn) (pb->fnclosure, inbuf, outbuf + 2, size,
			   &translated);
    if (status != 0)
	return status;

    /* The output function is permitted to add up to PACKET_SLOP
       bytes.  */
    assert (translated <= size + PACKET_SLOP);

    outbuf[0] = (translated >> 8) & 0xff;
    outbuf[1] = translated & 0xff;

    if (outbuf == stack_outbuf)
	buf_output (pb->buf, outbuf, translated + 2);
    else
    {
	outdata->size = translated + 2;
	buf_append_data (pb->buf, outdata, outdata);
    }

    *wrote = have;

    /* We will only be here because buf_send_output was called on the
       packetizing buffer.  That means that we should now call
       buf_send_output on the underlying buffer.  */
    return buf_send_output (pb->buf);
}



/* Flush data to a packetizing buffer.  */
static int
packetizing_buffer_flush (void *closure)
{
    struct packetizing_buffer *pb = closure;

    /* Flush the underlying buffer.  Note that if the original call to
       buf_flush passed 1 for the BLOCK argument, then the buffer will
       already have been set into blocking mode, so we should always
       pass 0 here.  */
    return buf_flush (pb->buf, 0);
}



/* The block routine for a packetizing buffer.  */
static int
packetizing_buffer_block (void *closure, bool block)
{
    struct packetizing_buffer *pb = closure;

    if (block)
	return set_block (pb->buf);
    else
	return set_nonblock (pb->buf);
}



/* Return the file descriptor underlying any child buffers.  */
static int
packetizing_buffer_get_fd (void *closure)
{
    struct packetizing_buffer *cb = closure;
    return buf_get_fd (cb->buf);
}



/* Shut down a packetizing buffer.  */
static int
packetizing_buffer_shutdown (struct buffer *buf)
{
    struct packetizing_buffer *pb = buf->closure;

    return buf_shutdown (pb->buf);
}



/* All server communication goes through buffer structures.  Most of
   the buffers are built on top of a file descriptor.  This structure
   is used as the closure field in a buffer.  */

struct fd_buffer
{
    /* The file descriptor.  */
    int fd;
    /* Nonzero if the file descriptor is in blocking mode.  */
    int blocking;
    /* The child process id when fd is a pipe.  */
    pid_t child_pid;
    /* The connection info, when fd is a pipe to a server.  */
    cvsroot_t *root;
};

static int fd_buffer_input (void *, char *, size_t, size_t, size_t *);
static int fd_buffer_output (void *, const char *, size_t, size_t *);
static int fd_buffer_flush (void *);
static int fd_buffer_block (void *, bool);
static int fd_buffer_get_fd (void *);
static int fd_buffer_shutdown (struct buffer *);

/* Initialize a buffer built on a file descriptor.  FD is the file
   descriptor.  INPUT is nonzero if this is for input, zero if this is
   for output.  MEMORY is the function to call when a memory error
   occurs.  */

struct buffer *
fd_buffer_initialize (int fd, pid_t child_pid, cvsroot_t *root, bool input,
                      void (*memory) (struct buffer *))
{
    struct fd_buffer *n;

    n = xmalloc (sizeof *n);
    n->fd = fd;
    n->child_pid = child_pid;
    n->root = root;
    fd_buffer_block (n, true);
    return buf_initialize (input ? fd_buffer_input : NULL,
			   input ? NULL : fd_buffer_output,
			   input ? NULL : fd_buffer_flush,
			   fd_buffer_block, fd_buffer_get_fd,
			   fd_buffer_shutdown,
			   memory,
			   n);
}



/* The buffer input function for a buffer built on a file descriptor.
 *
 * In non-blocking mode, this function will read as many bytes as it can in a
 * single try, up to SIZE bytes, and return.
 *
 * In blocking mode with NEED > 0, this function will read as many bytes as it
 * can but will not return until it has read at least NEED bytes.
 *
 * In blocking mode with NEED == 0, this function will block until it can read
 * either at least one byte or EOF, then read as many bytes as are available
 * and return.  At the very least, compress_buffer_shutdown depends on this
 * behavior to read EOF and can loop indefinitely without it.
 *
 * ASSUMPTIONS
 *   NEED <= SIZE.
 *
 * INPUTS
 *   closure	Our FD_BUFFER struct.
 *   data	The start of our input buffer.
 *   need	How many bytes our caller needs.
 *   size	How many bytes are available in DATA.
 *   got	Where to store the number of bytes read.
 *
 * OUTPUTS
 *   data	Filled with bytes read.
 *   *got	Number of bytes actually read into DATA.
 *
 * RETURNS
 *   errno	On error.
 *   -1		On EOF.
 *   0		Otherwise.
 *
 * ERRORS
 *   This function can return an error if fd_buffer_block(), or the system
 *   read() or select() calls do.
 */
static int
fd_buffer_input (void *closure, char *data, size_t need, size_t size,
		 size_t *got)
{
    struct fd_buffer *fb = closure;
    int nbytes;

    assert (need <= size);

    *got = 0;

    if (fb->blocking)
    {
	int status;
	fd_set readfds;

	/* Set non-block.  */
        status = fd_buffer_block (fb, false);
	if (status != 0) return status;

	FD_ZERO (&readfds);
	FD_SET (fb->fd, &readfds);
	do
	{
	    int numfds;

	    do {
		/* This used to select on exceptions too, but as far
		   as I know there was never any reason to do that and
		   SCO doesn't let you select on exceptions on pipes.  */
		numfds = fd_select (fb->fd + 1, &readfds, NULL, NULL, NULL);
		if (numfds < 0 && errno != EINTR)
		{
		    status = errno;
		    goto block_done;
		}
	    } while (numfds < 0);

	    nbytes = read (fb->fd, data + *got, size - *got);

	    if (nbytes == 0)
	    {
		/* End of file.  This assumes that we are using POSIX or BSD
		   style nonblocking I/O.  On System V we will get a zero
		   return if there is no data, even when not at EOF.  */
		if (*got)
		{
		    /* We already read some data, so return no error, counting
		     * on the fact that we will read EOF again next time.
		     */
		    status = 0;
		    break;
		}
		else
		{
		    /* Return EOF.  */
		    status = -1;
		    break;
		}
	    }

	    if (nbytes < 0)
	    {
		/* Some error occurred.  */
		if (!blocking_error (errno))
		{
		    status = errno;
		    break;
		}
		/* else Everything's fine, we just didn't get any data.  */
	    }

	    *got += nbytes;
	} while (*got < need);

block_done:
	if (status == 0 || status == -1)
	{
	    int newstatus;

	    /* OK or EOF - Reset block.  */
	    newstatus = fd_buffer_block (fb, true);
	    if (newstatus) status = newstatus;
	}
	return status;
    }

    /* The above will always return.  Handle non-blocking read.  */
    nbytes = read (fb->fd, data, size);

    if (nbytes > 0)
    {
	*got = nbytes;
	return 0;
    }

    if (nbytes == 0)
	/* End of file.  This assumes that we are using POSIX or BSD
	   style nonblocking I/O.  On System V we will get a zero
	   return if there is no data, even when not at EOF.  */
	return -1;

    /* Some error occurred.  */
    if (blocking_error (errno))
	/* Everything's fine, we just didn't get any data.  */
	return 0;

    return errno;
}



/* The buffer output function for a buffer built on a file descriptor.  */

static int
fd_buffer_output (void *closure, const char *data, size_t have, size_t *wrote)
{
    struct fd_buffer *fd = closure;

    *wrote = 0;

    while (have > 0)
    {
	int nbytes;

	nbytes = write (fd->fd, data, have);

	if (nbytes <= 0)
	{
	    if (! fd->blocking
		&& (nbytes == 0 || blocking_error (errno)))
	    {
		/* A nonblocking write failed to write any data.  Just
		   return.  */
		return 0;
	    }

	    /* Some sort of error occurred.  */

	    if (nbytes == 0)
		return EIO;

	    return errno;
	}

	*wrote += nbytes;
	data += nbytes;
	have -= nbytes;
    }

    return 0;
}



/* The buffer flush function for a buffer built on a file descriptor.  */
static int
fd_buffer_flush (void *closure)
{
    /* We don't need to do anything here.  Our fd doesn't have its own buffer
     * and syncing won't do anything but slow us down.
     *
     * struct fd_buffer *fb = closure;
     *
     * if (fsync (fb->fd) < 0 && errno != EROFS && errno != EINVAL)
     *     return errno;
     */
    return 0;
}



static struct stat devnull;
static int devnull_set = -1;

/* The buffer block function for a buffer built on a file descriptor.  */
static int
fd_buffer_block (void *closure, bool block)
{
    struct fd_buffer *fb = closure;
# if defined (F_GETFL) && defined (O_NONBLOCK) && defined (F_SETFL)
    int flags;

    flags = fcntl (fb->fd, F_GETFL, 0);
    if (flags < 0)
	return errno;

    if (block)
	flags &= ~O_NONBLOCK;
    else
	flags |= O_NONBLOCK;

    if (fcntl (fb->fd, F_SETFL, flags) < 0)
    {
	/*
	 * BSD returns ENODEV when we try to set block/nonblock on /dev/null.
	 * BSDI returns ENOTTY when we try to set block/nonblock on /dev/null.
	 */
	struct stat sb;
	int save_errno = errno;
	bool isdevnull = false;

	if (devnull_set == -1)
	    devnull_set = stat ("/dev/null", &devnull);

	if (devnull_set >= 0)
	    /* Equivalent to /dev/null ? */
	    isdevnull = (fstat (fb->fd, &sb) >= 0
			 && sb.st_dev == devnull.st_dev
			 && sb.st_ino == devnull.st_ino
			 && sb.st_mode == devnull.st_mode
			 && sb.st_uid == devnull.st_uid
			 && sb.st_gid == devnull.st_gid
			 && sb.st_size == devnull.st_size
			 && sb.st_blocks == devnull.st_blocks
			 && sb.st_blksize == devnull.st_blksize);
	if (isdevnull)
	    errno = 0;
	else
	{
	    errno = save_errno;
	    return errno;
	}
    }
# endif /* F_GETFL && O_NONBLOCK && F_SETFL */

    fb->blocking = block;

    return 0;
}



static int
fd_buffer_get_fd (void *closure)
{
    struct fd_buffer *fb = closure;
    return fb->fd;
}



/* The buffer shutdown function for a buffer built on a file descriptor.
 *
 * This function disposes of memory allocated for this buffer.
 */
static int
fd_buffer_shutdown (struct buffer *buf)
{
    struct fd_buffer *fb = buf->closure;
    struct stat s;
    bool closefd, statted;

    /* Must be an open pipe, socket, or file.  What could go wrong? */
    if (fstat (fb->fd, &s) == -1) statted = false;
    else statted = true;
    /* Don't bother to try closing the FD if we couldn't stat it.  This
     * probably won't work.
     *
     * (buf_shutdown() on some of the server/child communication pipes is
     * getting EBADF on both the fstat and the close.  I'm not sure why -
     * perhaps they were alredy closed somehow?
     */
    closefd = statted;

    /* Flush the buffer if possible.  */
    if (buf->flush)
    {
	buf_flush (buf, 1);
	buf->flush = NULL;
    }

    if (buf->input)
    {
	/* There used to be a check here for unread data in the buffer of
	 * the pipe, but it was deemed unnecessary and possibly dangerous.  In
	 * some sense it could be second-guessing the caller who requested it
	 * closed, as well.
	 */

/* FIXME:
 *
 * This mess of #ifdefs is hard to read.  There must be some relation between
 * the macros being checked which at least deserves comments - if
 * SHUTDOWN_SERVER, NO_SOCKET_TO_FD, & START_RSH_WITH_POPEN_RW were completely
 * independant, then the next few lines could easily refuse to compile.
 *
 * The note below about START_RSH_WITH_POPEN_RW never being set when
 * SHUTDOWN_SERVER is defined means that this code would always break on
 * systems with SHUTDOWN_SERVER defined and thus the comment must now be
 * incorrect or the code was broken since the comment was written.
 */
# ifdef SHUTDOWN_SERVER
	if (fb->root && fb->root->method != server_method)
# endif
# ifndef NO_SOCKET_TO_FD
	{
	    /* shutdown() sockets */
	    if (statted && S_ISSOCK (s.st_mode))
		shutdown (fb->fd, 0);
	}
# endif /* NO_SOCKET_TO_FD */
# ifdef START_RSH_WITH_POPEN_RW
/* Can't be set with SHUTDOWN_SERVER defined */
	/* FIXME: This is now certainly broken since pclose is defined by ANSI
	 * C to accept a FILE * argument.  The switch will need to happen at a
	 * higher abstraction level to switch between initializing stdio & fd
	 * buffers on systems that need this (or maybe an fd buffer that keeps
	 * track of the FILE * could be used - I think flushing the stream
	 * before beginning exclusive access via the FD is OK.
	 */
	else if (fb->root && pclose (fb->fd) == EOF)
	{
	    error (1, errno, "closing connection to %s",
		   fb->root->hostname);
	    closefd = false;
	}
# endif /* START_RSH_WITH_POPEN_RW */

	buf->input = NULL;
    }
    else if (buf->output)
    {
# ifdef SHUTDOWN_SERVER
	/* FIXME:  Should have a SHUTDOWN_SERVER_INPUT &
	 * SHUTDOWN_SERVER_OUTPUT
	 */
	if (fb->root && fb->root->method == server_method)
	    SHUTDOWN_SERVER (fb->fd);
	else
# endif
# ifndef NO_SOCKET_TO_FD
	/* shutdown() sockets */
	if (statted && S_ISSOCK (s.st_mode))
	    shutdown (fb->fd, 1);
# else
	{
	/* I'm not sure I like this empty block, but the alternative
	 * is another nested NO_SOCKET_TO_FD switch as above.
	 */
	}
# endif /* NO_SOCKET_TO_FD */

	buf->output = NULL;
    }

    if (statted && closefd && close (fb->fd) == -1)
    {
	if (server_active)
	{
            /* Syslog this? */
	}
# ifdef CLIENT_SUPPORT
	else if (fb->root)
            error (1, errno, "closing down connection to %s",
                   fb->root->hostname);
	    /* EXITS */
# endif /* CLIENT_SUPPORT */

	error (0, errno, "closing down buffer");
    }

    /* If we were talking to a process, make sure it exited */
    if (fb->child_pid)
    {
	int w;

	do
	    w = waitpid (fb->child_pid, NULL, 0);
	while (w == -1 && errno == EINTR);
	if (w == -1)
	    error (1, errno, "waiting for process %d", fb->child_pid);
    }

    free (buf->closure);
    buf->closure = NULL;

    return 0;
}
#endif /* defined (SERVER_SUPPORT) || defined (CLIENT_SUPPORT) */
