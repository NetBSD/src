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

/* Declarations concerning the buffer data structure.  */

#if defined (SERVER_SUPPORT) || defined (CLIENT_SUPPORT)

# include "getpagesize.h"

/*
 * We must read data from a child process and send it across the
 * network.  We do not want to block on writing to the network, so we
 * store the data from the child process in memory.  A BUFFER
 * structure holds the status of one communication, and uses a linked
 * list of buffer_data structures to hold data.
 */

struct buffer;

typedef int (*type_buf_input) (void *, char *, size_t, size_t, size_t *);
typedef int (*type_buf_output) (void *, const char *, size_t, size_t *);
typedef int (*type_buf_flush) (void *);
typedef int (*type_buf_block) (void *, bool);
typedef int (*type_buf_get_fd) (void *);
typedef int (*type_buf_shutdown) (struct buffer *);
typedef void (*type_buf_memory_error) (struct buffer *);

struct buffer
{
    /* Data.  */
    struct buffer_data *data;

    /* Last buffer on data chain.  */
    struct buffer_data *last;

    /* Nonzero if the buffer is in nonblocking mode.  */
    bool nonblocking;

    /* Functions must be provided to transfer data in and out of the
       buffer.  Either the input or output field must be set, but not
       both.  */

    /* Read data into the buffer DATA.  There is room for up to SIZE
       bytes.  In blocking mode, wait until some input, at least NEED
       bytes, is available (NEED may be 0 but that is the same as NEED
       == 1).  In non-blocking mode return immediately no matter how
       much input is available; NEED is ignored. Return 0 on success,
       or -1 on end of file, or an errno code.  Set the number of
       bytes read in *GOT.
       
       If there are a nonzero number of bytes available, less than NEED,
       followed by end of file, just read those bytes and return 0.  */
    type_buf_input input;

    /* Write data.  This should write up to HAVE bytes from DATA.
       This should return 0 on success, or an errno code.  It should
       set the number of bytes written in *WROTE.  */
    type_buf_output output;

    /* Flush any data which may be buffered up after previous calls to
       OUTPUT.  This should return 0 on success, or an errno code.  */
    type_buf_flush flush;

    /* Change the blocking mode of the underlying communication
       stream.  If BLOCK is non-zero, it should be placed into
       blocking mode.  Otherwise, it should be placed into
       non-blocking mode.  This should return 0 on success, or an
       errno code.  */
    type_buf_block block;

    /* Return the file descriptor underlying this buffer, if any, or -1
     * otherwise.
     */
    type_buf_get_fd get_fd;

    /* Shut down the communication stream.  This does not mean that it
       should be closed.  It merely means that no more data will be
       read or written, and that any final processing that is
       appropriate should be done at this point.  This may be NULL.
       It should return 0 on success, or an errno code.  This entry
       point exists for the compression code.  */
    type_buf_shutdown shutdown;

    /* This field is passed to the INPUT, OUTPUT, and BLOCK functions.  */
    void *closure;

    /* Function to call if we can't allocate memory.  */
    type_buf_memory_error memory_error;
};

/* Data is stored in lists of these structures.  */

struct buffer_data
{
    /* Next buffer in linked list.  */
    struct buffer_data *next;

    /*
     * A pointer into the data area pointed to by the text field.  This
     * is where to find data that has not yet been written out.
     */
    char *bufp;

    /* The number of data bytes found at BUFP.  */
    size_t size;

    /*
     * Actual buffer.  This never changes after the structure is
     * allocated.  The buffer is BUFFER_DATA_SIZE bytes.
     */
    char *text;
};

/* The size we allocate for each buffer_data structure.  */
#define BUFFER_DATA_SIZE getpagesize ()

/* The type of a function passed as a memory error handler.  */
typedef void (*BUFMEMERRPROC) (struct buffer *);

struct buffer *buf_initialize (type_buf_input,
				type_buf_output,
				type_buf_flush,
				type_buf_block,
				type_buf_get_fd,
				type_buf_shutdown,
				type_buf_memory_error,
				void *);
void buf_free (struct buffer *);
struct buffer *buf_nonio_initialize (void (*) (struct buffer *));
struct buffer *compress_buffer_initialize (struct buffer *, int, int,
					   void (*) (struct buffer *));
struct buffer *packetizing_buffer_initialize
	(struct buffer *, int (*) (void *, const char *, char *, size_t),
	 int (*) (void *, const char *, char *, size_t, size_t *), void *,
	 void (*) (struct buffer *));
int buf_empty (struct buffer *);
int buf_empty_p (struct buffer *);
void buf_output (struct buffer *, const char *, size_t);
void buf_output0 (struct buffer *, const char *);
void buf_append_char (struct buffer *, int);
int buf_send_output (struct buffer *);
int buf_flush (struct buffer *, bool);
int set_nonblock (struct buffer *);
int set_block (struct buffer *);
int buf_send_counted (struct buffer *);
int buf_send_special_count (struct buffer *, int);
void buf_append_data (struct buffer *, struct buffer_data *,
		      struct buffer_data *);
void buf_append_buffer (struct buffer *, struct buffer *);
int buf_read_file (FILE *, long, struct buffer_data **, struct buffer_data **);
int buf_read_file_to_eof (FILE *, struct buffer_data **,
			  struct buffer_data **);
int buf_input_data (struct buffer *, size_t *);
int buf_read_line (struct buffer *, char **, size_t *);
int buf_read_short_line (struct buffer *buf, char **line, size_t *lenp,
                         size_t max);
int buf_read_data (struct buffer *, size_t, char **, size_t *);
void buf_copy_lines (struct buffer *, struct buffer *, int);
int buf_copy_counted (struct buffer *, struct buffer *, int *);
int buf_chain_length (struct buffer_data *);
int buf_length (struct buffer *);
int buf_get_fd (struct buffer *);
int buf_shutdown (struct buffer *);
#ifdef PROXY_SUPPORT
void buf_copy_data (struct buffer *buf, struct buffer_data *data,
                    struct buffer_data *last);
#endif /* PROXY_SUPPORT */
void buf_free_data (struct buffer *);

#ifdef SERVER_FLOWCONTROL
int buf_count_mem (struct buffer *);
#endif /* SERVER_FLOWCONTROL */

struct buffer *
fd_buffer_initialize (int fd, pid_t child_pid, cvsroot_t *root, bool input,
                      void (*memory) (struct buffer *));

/* EWOULDBLOCK is not defined by POSIX, but some BSD systems will
   return it, rather than EAGAIN, for nonblocking writes.  */
# ifdef EWOULDBLOCK
#   define blocking_error(err) ((err) == EWOULDBLOCK || (err) == EAGAIN)
# else
#   define blocking_error(err) ((err) == EAGAIN)
# endif
#endif /* defined (SERVER_SUPPORT) || defined (CLIENT_SUPPORT) */
