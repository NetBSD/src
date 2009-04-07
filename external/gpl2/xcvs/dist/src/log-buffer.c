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

#if defined CLIENT_SUPPORT || defined SERVER_SUPPORT

/* We want to be able to log data sent between us and the server.  We
   do it using log buffers.  Each log buffer has another buffer which
   handles the actual I/O, and a file to log information to.

   This structure is the closure field of a log buffer.  */

struct log_buffer
{
    /* The underlying buffer.  */
    struct buffer *buf;
    /* The file to log information to.  */
    FILE *log;

#ifdef PROXY_SUPPORT
    /* Whether errors writing to the log file should be fatal or not.  */
    bool fatal_errors;

    /* The name of the file backing this buffer so that it may be deleted on
     * buffer shutdown.
     */
    char *back_fn;

    /* Set once logging is permanently disabled for a buffer.  */
    bool disabled;

    /* The memory buffer (cache) backing this log.  */
    struct buffer *back_buf;

    /* The maximum number of bytes to store in memory before beginning logging
     * to a file.
     */
    size_t max;

    /* Once we start logging to a file we do not want to stop unless asked.  */
    bool tofile;
#endif /* PROXY_SUPPORT */
};



#ifdef PROXY_SUPPORT
/* Force the existance of lb->log.
 *
 * INPUTS
 *   lb			The log buffer.
 *
 * OUTPUTS
 *   lb->log		The new FILE *.
 *   lb->back_fn	The name of the new log, for later disposal.
 *
 * ASSUMPTIONS
 *   lb->log is NULL or, at least, does not require freeing.
 *   lb->back_fn is NULL or, at least, does not require freeing..
 *
 * RETURNS
 *   Nothing.
 *
 * ERRORS
 *   Errors creating the log file will output a message via error().  Whether
 *   the error is fatal or not is dependent on lb->fatal_errors.
 */
static inline void
log_buffer_force_file (struct log_buffer *lb)
{
    lb->log = cvs_temp_file (&lb->back_fn);
    if (!lb->log)
	error (lb->fatal_errors, errno, "failed to open log file.");
}
#endif /* PROXY_SUPPORT */



/* Create a log buffer.
 *
 * INPUTS
 *   buf		A pointer to the buffer structure to log input from.
 *   fp			A file name to log data to.  May be NULL.
#ifdef PROXY_SUPPORT
 *   fatal_errors	Whether errors writing to a log file should be
 *			considered fatal.
#else
 *   fatal_errors	unused
#endif
 *   input		Whether we will log data for an input or output
 *			buffer.
#ifdef PROXY_SUPPORT
 *   max		The maximum size of our memory cache.
#else
 *   max		unused
#endif
 *   memory		The function to call when memory allocation errors are
 *			encountered.
 *
 * RETURNS
 *   A pointer to a new buffer structure.
 */
static int log_buffer_input (void *, char *, size_t, size_t, size_t *);
static int log_buffer_output (void *, const char *, size_t, size_t *);
static int log_buffer_flush (void *);
static int log_buffer_block (void *, bool);
static int log_buffer_get_fd (void *);
static int log_buffer_shutdown (struct buffer *);
struct buffer *
log_buffer_initialize (struct buffer *buf, FILE *fp,
# ifdef PROXY_SUPPORT
		       bool fatal_errors,
		       size_t max,
# endif /* PROXY_SUPPORT */
                       bool input,
		       void (*memory) (struct buffer *))
{
    struct log_buffer *lb = xmalloc (sizeof *lb);
    struct buffer *retbuf;

    lb->buf = buf;
    lb->log = fp;
#ifdef PROXY_SUPPORT
    lb->back_fn = NULL;
    lb->fatal_errors = fatal_errors;
    lb->disabled = false;
    assert (size_in_bounds_p (max));
    lb->max = max;
    lb->tofile = false;
    lb->back_buf = buf_nonio_initialize (memory);
#endif /* PROXY_SUPPORT */
    retbuf = buf_initialize (input ? log_buffer_input : NULL,
			     input ? NULL : log_buffer_output,
			     input ? NULL : log_buffer_flush,
			     log_buffer_block, log_buffer_get_fd,
			     log_buffer_shutdown, memory, lb);

    if (!buf_empty_p (buf))
    {
	/* If our buffer already had data, copy it & log it if necessary.  This
	 * can happen, for instance, with a pserver, where we deliberately do
	 * not instantiate the log buffer until after authentication so that
	 * auth data does not get logged (the pserver data will not be logged
	 * in this case, but any data which was left unused in the buffer by
	 * the auth code will be logged and put in our new buffer).
	 */
	struct buffer_data *data;
#ifdef PROXY_SUPPORT
	size_t total = 0;
#endif /* PROXY_SUPPORT */

	for (data = buf->data; data != NULL; data = data->next)
	{
#ifdef PROXY_SUPPORT
	    if (!lb->tofile)
	    {
		total = xsum (data->size, total);
		if (total >= max)
		    lb->tofile = true;
	    }

	    if (lb->tofile)
	    {
		if (!lb->log) log_buffer_force_file (lb);
		if (lb->log)
		{
#endif /* PROXY_SUPPORT */
		    if (fwrite (data->bufp, 1, data->size, lb->log)
			!= (size_t) data->size)
			error (
#ifdef PROXY_SUPPORT
			       fatal_errors,
#else /* !PROXY_SUPPORT */
			       false,
#endif /* PROXY_SUPPORT */
			       errno, "writing to log file");
		    fflush (lb->log);
#ifdef PROXY_SUPPORT
		}
	    }
	    else
		/* Log to memory buffer.  */
		buf_copy_data (lb->back_buf, data, data);
#endif /* PROXY_SUPPORT */
	}
	buf_append_buffer (retbuf, buf);
    }
    return retbuf;
}



/* The input function for a log buffer.  */
static int
log_buffer_input (void *closure, char *data, size_t need, size_t size,
		  size_t *got)
{
    struct log_buffer *lb = closure;
    int status;

    assert (lb->buf->input);

    status = (*lb->buf->input) (lb->buf->closure, data, need, size, got);
    if (status != 0)
	return status;

    if (
#ifdef PROXY_SUPPORT
	!lb->disabled &&
#endif /* PROXY_SUPPORT */
	*got > 0)
    {
#ifdef PROXY_SUPPORT
	if (!lb->tofile
	    && xsum (*got, buf_count_mem (lb->back_buf)) >= lb->max)
	    lb->tofile = true;

	if (lb->tofile)
	{
	    if (!lb->log) log_buffer_force_file (lb);
	    if (lb->log)
	    {
#endif /* PROXY_SUPPORT */
		if (fwrite (data, 1, *got, lb->log) != *got)
		    error (
#ifdef PROXY_SUPPORT
			   lb->fatal_errors,
#else /* !PROXY_SUPPORT */
			   false,
#endif /* PROXY_SUPPORT */
			   errno, "writing to log file");
		fflush (lb->log);
#ifdef PROXY_SUPPORT
	    }
	}
	else
	    /* Log to memory buffer.  */
	    buf_output (lb->back_buf, data, *got);
#endif /* PROXY_SUPPORT */
    }

    return 0;
}



/* The output function for a log buffer.  */
static int
log_buffer_output (void *closure, const char *data, size_t have, size_t *wrote)
{
    struct log_buffer *lb = closure;
    int status;

    assert (lb->buf->output);

    status = (*lb->buf->output) (lb->buf->closure, data, have, wrote);
    if (status != 0)
	return status;

    if (
#ifdef PROXY_SUPPORT
	!lb->disabled &&
#endif /* PROXY_SUPPORT */
	*wrote > 0)
    {
#ifdef PROXY_SUPPORT
	if (!lb->tofile
	    && xsum (*wrote, buf_count_mem (lb->back_buf)) >= lb->max)
	    lb->tofile = true;

	if (lb->tofile)
	{
	    if (!lb->log) log_buffer_force_file (lb);
	    if (lb->log)
	    {
#endif /* PROXY_SUPPORT */
		if (fwrite (data, 1, *wrote, lb->log) != *wrote)
		    error (
#ifdef PROXY_SUPPORT
			   lb->fatal_errors,
#else /* !PROXY_SUPPORT */
			   false,
#endif /* PROXY_SUPPORT */
			   errno, "writing to log file");
		fflush (lb->log);
#ifdef PROXY_SUPPORT
	    }
	}
	else
	    /* Log to memory buffer.  */
	    buf_output (lb->back_buf, data, *wrote);
#endif /* PROXY_SUPPORT */
    }

    return 0;
}



/* The flush function for a log buffer.  */
static int
log_buffer_flush (void *closure)
{
    struct log_buffer *lb = closure;

    assert (lb->buf->flush);

    /* We don't really have to flush the log file here, but doing it
     * will let tail -f on the log file show what is sent to the
     * network as it is sent.
     */
    if (lb->log && (fflush (lb->log)))
        error (0, errno, "flushing log file");

    return (*lb->buf->flush) (lb->buf->closure);
}



/* The block function for a log buffer.  */
static int
log_buffer_block (void *closure, bool block)
{
    struct log_buffer *lb = closure;

    if (block)
	return set_block (lb->buf);
    else
	return set_nonblock (lb->buf);
}



#ifdef PROXY_SUPPORT
/* Disable logging without shutting down the next buffer in the chain.
 */
struct buffer *
log_buffer_rewind (struct buffer *buf)
{
    struct log_buffer *lb = buf->closure;
    struct buffer *retbuf;
    int fd;

    lb->disabled = true;

    if (lb->log)
    {
	FILE *tmp = lb->log;
	lb->log = NULL;

	/* flush & rewind the file.  */
	if (fflush (tmp) < 0)
	    error (0, errno, "flushing log file");
	rewind (tmp);

	/* Get a descriptor for the log and close the FILE *.  */
	fd = dup (fileno (tmp));
	if (fclose (tmp) < 0)
	    error (0, errno, "closing log file");
    }
    else
	fd = open (DEVNULL, O_RDONLY);

    /* Catch dup/open errors.  */
    if (fd < 0)
    {
	error (lb->fatal_errors, errno, "failed to rewind log buf.");
	return NULL;
    }

    /* Create a new fd buffer around the log.  */
    retbuf = fd_buffer_initialize (fd, 0, NULL, true, buf->memory_error);

    {
	struct buffer *tmp;
        /* Insert the data which wasn't written to a file.  */
	buf_append_buffer (retbuf, lb->back_buf);
	tmp = lb->back_buf;
	lb->back_buf = NULL;
	buf_free (tmp);
    }

    return retbuf;
}
#endif /* PROXY_SUPPORT */



/* Disable logging and close the log without shutting down the next buffer in
 * the chain.
 */
#ifndef PROXY_SUPPORT
static
#endif /* !PROXY_SUPPORT */
void
log_buffer_closelog (struct buffer *buf)
{
    struct log_buffer *lb = buf->closure;
    void *tmp;

#ifdef PROXY_SUPPORT
    lb->disabled = true;
#endif /* PROXY_SUPPORT */

    /* Close the log.  */
    if (lb->log)
    {
	tmp = lb->log;
	lb->log = NULL;
	if (fclose (tmp) < 0)
	    error (0, errno, "closing log file");
    }

#ifdef PROXY_SUPPORT
    /* Delete the log if we know its name.  */
    if (lb->back_fn)
    {
	tmp = lb->back_fn;
	lb->back_fn = NULL;
	if (CVS_UNLINK (tmp))
	    error (0, errno, "Failed to delete log file.");
	free (tmp);
    }

    if (lb->back_buf)
    {
	tmp = lb->back_buf;
	lb->back_buf = NULL;
	buf_free (tmp);
    }
#endif /* PROXY_SUPPORT */
}



/* Return the file descriptor underlying any child buffers.  */
static int
log_buffer_get_fd (void *closure)
{
    struct log_buffer *lb = closure;
    return buf_get_fd (lb->buf);
}



/* The shutdown function for a log buffer.  */
static int
log_buffer_shutdown (struct buffer *buf)
{
    struct log_buffer *lb = buf->closure;

    log_buffer_closelog (buf);
    return buf_shutdown (lb->buf);
}



void
setup_logfiles (char *var, struct buffer **to_server_p,
                struct buffer **from_server_p)
{
  char *log = getenv (var);

  /* Set up logfiles, if any.
   *
   * We do this _after_ authentication on purpose.  Wouldn't really like to
   * worry about logging passwords...
   */
  if (log)
    {
      int len = strlen (log);
      char *buf = xmalloc (len + 5);
      char *p;
      FILE *fp;

      strcpy (buf, log);
      p = buf + len;

      /* Open logfiles in binary mode so that they reflect
	 exactly what was transmitted and received (that is
	 more important than that they be maximally
	 convenient to view).  */
      /* Note that if we create several connections in a single CVS client
	 (currently used by update.c), then the last set of logfiles will
	 overwrite the others.  There is currently no way around this.  */
      strcpy (p, ".in");
      fp = fopen (buf, "wb");
      if (!fp)
	error (0, errno, "opening to-server logfile %s", buf);
      else
	*to_server_p = log_buffer_initialize (*to_server_p, fp,
# ifdef PROXY_SUPPORT
					      false,
					      0,
# endif /* PROXY_SUPPORT */
					      false, NULL);

      strcpy (p, ".out");
      fp = fopen (buf, "wb");
      if (!fp)
	error (0, errno, "opening from-server logfile %s", buf);
      else
	*from_server_p = log_buffer_initialize (*from_server_p, fp,
# ifdef PROXY_SUPPORT
						false,
						0,
# endif /* PROXY_SUPPORT */
                                                true, NULL);

      free (buf);
    }
}

#endif /* CLIENT_SUPPORT || SERVER_SUPPORT */
