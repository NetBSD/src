/* Remote serial support interface definitions for GDB, the GNU Debugger.
   Copyright 1992, 1993, 1999, 2000 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef SERIAL_H
#define SERIAL_H

/* For most routines, if a failure is indicated, then errno should be
   examined.  */

/* Terminal state pointer.  This is specific to each type of
   interface. */

typedef void *serial_ttystate;
struct _serial_t;
typedef struct _serial_t *serial_t;

/* Try to open NAME.  Returns a new serial_t on success, NULL on
   failure. */

extern serial_t serial_open (const char *name);
#define SERIAL_OPEN(NAME) serial_open(NAME)

/* Open a new serial stream using a file handle.  */

extern serial_t serial_fdopen (const int fd);
#define SERIAL_FDOPEN(FD) serial_fdopen(FD)

/* Push out all buffers, close the device and destroy SERIAL_T. */

extern void serial_close (serial_t);
#define SERIAL_CLOSE(SERIAL_T) serial_close ((SERIAL_T))

/* Push out all buffers and destroy SERIAL_T without closing the
   device.  */

extern void serial_un_fdopen (serial_t scb);
#define SERIAL_UN_FDOPEN(SERIAL_T) serial_un_fdopen ((SERIAL_T))

/* Read one char from the serial device with TIMEOUT seconds to wait
   or -1 to wait forever.  Use timeout of 0 to effect a poll.
   Infinite waits are not permitted. Returns unsigned char if ok, else
   one of the following codes.  Note that all error return-codes are
   guaranteed to be < 0. */

enum serial_rc {
  SERIAL_ERROR = -1,	/* General error. */
  SERIAL_TIMEOUT = -2,	/* Timeout or data-not-ready during read.
			   Unfortunatly, through ui_loop_hook(), this
			   can also be a QUIT indication.  */
  SERIAL_EOF = -3	/* General end-of-file or remote target
			   connection closed, indication.  Includes
			   things like the line dropping dead. */
};

extern int serial_readchar (serial_t scb, int timeout);
#define SERIAL_READCHAR(SERIAL_T, TIMEOUT) serial_readchar ((SERIAL_T), (TIMEOUT))

/* Write LEN chars from STRING to the port SERIAL_T.  Returns 0 for
   success, non-zero for failure.  */

extern int serial_write (serial_t scb, const char *str, int len);
#define SERIAL_WRITE(SERIAL_T, STRING,LEN)  serial_write (SERIAL_T, STRING, LEN)

/* Write a printf style string onto the serial port. */

extern void serial_printf (serial_t desc, const char *,...) ATTR_FORMAT (printf, 2, 3);

/* Allow pending output to drain. */

extern int serial_drain_output (serial_t);
#define SERIAL_DRAIN_OUTPUT(SERIAL_T) serial_drain_output ((SERIAL_T))

/* Flush (discard) pending output.  Might also flush input (if this
   system can't flush only output).  */

extern int serial_flush_output (serial_t);
#define SERIAL_FLUSH_OUTPUT(SERIAL_T) serial_flush_output ((SERIAL_T))

/* Flush pending input.  Might also flush output (if this system can't
   flush only input).  */

extern int serial_flush_input (serial_t);
#define SERIAL_FLUSH_INPUT(SERIAL_T) serial_flush_input ((SERIAL_T))

/* Send a break between 0.25 and 0.5 seconds long.  */

extern int serial_send_break (serial_t scb);
#define SERIAL_SEND_BREAK(SERIAL_T) serial_send_break (SERIAL_T)

/* Turn the port into raw mode. */

extern void serial_raw (serial_t scb);
#define SERIAL_RAW(SERIAL_T) serial_raw ((SERIAL_T))

/* Return a pointer to a newly malloc'd ttystate containing the state
   of the tty.  */

extern serial_ttystate serial_get_tty_state (serial_t scb);
#define SERIAL_GET_TTY_STATE(SERIAL_T) serial_get_tty_state ((SERIAL_T))

/* Set the state of the tty to TTYSTATE.  The change is immediate.
   When changing to or from raw mode, input might be discarded.
   Returns 0 for success, negative value for error (in which case
   errno contains the error).  */

extern int serial_set_tty_state (serial_t scb, serial_ttystate ttystate);
#define SERIAL_SET_TTY_STATE(SERIAL_T, TTYSTATE) serial_set_tty_state ((SERIAL_T), (TTYSTATE))

/* printf_filtered a user-comprehensible description of ttystate on
   the specified STREAM. FIXME: At present this sends output to the
   default stream - GDB_STDOUT. */

extern void serial_print_tty_state (serial_t scb, serial_ttystate ttystate, struct ui_file *);
#define SERIAL_PRINT_TTY_STATE(SERIAL_T, TTYSTATE, STREAM) serial_print_tty_state ((SERIAL_T), (TTYSTATE), (STREAM))

/* Set the tty state to NEW_TTYSTATE, where OLD_TTYSTATE is the
   current state (generally obtained from a recent call to
   SERIAL_GET_TTY_STATE), but be careful not to discard any input.
   This means that we never switch in or out of raw mode, even if
   NEW_TTYSTATE specifies a switch.  */

extern int serial_noflush_set_tty_state (serial_t scb, serial_ttystate new_ttystate, serial_ttystate old_ttystate);
#define SERIAL_NOFLUSH_SET_TTY_STATE(SERIAL_T, NEW_TTYSTATE, OLD_TTYSTATE) \
serial_noflush_set_tty_state ((SERIAL_T), (NEW_TTYSTATE), (OLD_TTYSTATE))

/* Set the baudrate to the decimal value supplied.  Returns 0 for
   success, -1 for failure.  */

extern int serial_setbaudrate (serial_t scb, int rate);
#define SERIAL_SETBAUDRATE(SERIAL_T, RATE) serial_setbaudrate ((SERIAL_T), (RATE))

/* Set the number of stop bits to the value specified.  Returns 0 for
   success, -1 for failure.  */

#define SERIAL_1_STOPBITS 1
#define SERIAL_1_AND_A_HALF_STOPBITS 2	/* 1.5 bits, snicker... */
#define SERIAL_2_STOPBITS 3

extern int serial_setstopbits (serial_t scb, int num);
#define SERIAL_SETSTOPBITS(SERIAL_T, NUM) serial_setstopbits ((SERIAL_T), (NUM))

/* Asynchronous serial interface: */

/* Can the serial device support asynchronous mode? */

extern int serial_can_async_p (serial_t scb);
#define SERIAL_CAN_ASYNC_P(SERIAL_T) serial_can_async_p ((SERIAL_T))

/* Has the serial device been put in asynchronous mode? */

extern int serial_is_async_p (serial_t scb);
#define SERIAL_IS_ASYNC_P(SERIAL_T) serial_is_async_p ((SERIAL_T))

/* For ASYNC enabled devices, register a callback and enable
   asynchronous mode.  To disable asynchronous mode, register a NULL
   callback. */

typedef void (serial_event_ftype) (serial_t scb, void *context);
extern void serial_async (serial_t scb, serial_event_ftype *handler, void *context);
#define SERIAL_ASYNC(SERIAL_T, HANDLER, CONTEXT) serial_async ((SERIAL_T), (HANDLER), (CONTEXT)) 

/* Provide direct access to the underlying FD (if any) used to
   implement the serial device.  This interface is clearly
   deprecated. Will call internal_error() if the operation isn't
   applicable to the current serial device. */

extern int deprecated_serial_fd (serial_t scb);
#define DEPRECATED_SERIAL_FD(SERIAL_T) deprecated_serial_fd ((SERIAL_T))

/* Trace/debug mechanism.

   SERIAL_DEBUG() enables/disables internal debugging.
   SERIAL_DEBUG_P() indicates the current debug state. */

extern void serial_debug (serial_t scb, int debug_p);
#define SERIAL_DEBUG(SERIAL_T, DEBUG_P) serial_debug ((SERIAL_T), (DEBUG_P))

extern int serial_debug_p (serial_t scb);
#define SERIAL_DEBUG_P(SERIAL_T) serial_debug_p ((SERIAL_T))


/* Details of an instance of a serial object */

struct _serial_t
  {
    int fd;			/* File descriptor */
    struct serial_ops *ops;	/* Function vector */
    void *state;       		/* Local context info for open FD */
    serial_ttystate ttystate;	/* Not used (yet) */
    int bufcnt;			/* Amount of data remaining in receive
				   buffer.  -ve for sticky errors. */
    unsigned char *bufp;	/* Current byte */
    unsigned char buf[BUFSIZ];	/* Da buffer itself */
    int current_timeout;	/* (ser-unix.c termio{,s} only), last
				   value of VTIME */
    int timeout_remaining;	/* (ser-unix.c termio{,s} only), we
				   still need to wait for this many
				   more seconds.  */
    char *name;			/* The name of the device or host */
    struct _serial_t *next;	/* Pointer to the next serial_t */
    int refcnt;			/* Number of pointers to this block */
    int debug_p;		/* Trace this serial devices operation. */
    int async_state;		/* Async internal state. */
    void *async_context;	/* Async event thread's context */
    serial_event_ftype *async_handler;/* Async event handler */
  };

struct serial_ops
  {
    char *name;
    struct serial_ops *next;
    int (*open) (serial_t, const char *name);
    void (*close) (serial_t);
    int (*readchar) (serial_t, int timeout);
    int (*write) (serial_t, const char *str, int len);
    /* Discard pending output */
    int (*flush_output) (serial_t);
    /* Discard pending input */
    int (*flush_input) (serial_t);
    int (*send_break) (serial_t);
    void (*go_raw) (serial_t);
    serial_ttystate (*get_tty_state) (serial_t);
    int (*set_tty_state) (serial_t, serial_ttystate);
    void (*print_tty_state) (serial_t, serial_ttystate, struct ui_file *);
    int (*noflush_set_tty_state) (serial_t, serial_ttystate, serial_ttystate);
    int (*setbaudrate) (serial_t, int rate);
    int (*setstopbits) (serial_t, int num);
    /* Wait for output to drain */
    int (*drain_output) (serial_t);
    /* Change the serial device into/out of asynchronous mode, call
       the specified function when ever there is something
       interesting. */
    void (*async) (serial_t scb, int async_p);
  };

/* Add a new serial interface to the interface list */

extern void serial_add_interface (struct serial_ops * optable);

/* File in which to record the remote debugging session */

extern void serial_log_command (const char *);

#endif /* SERIAL_H */
