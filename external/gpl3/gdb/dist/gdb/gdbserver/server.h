/* Common definitions for remote server for GDB.
   Copyright (C) 1993-2013 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef SERVER_H
#define SERVER_H

#include "config.h"
#include "build-gnulib-gdbserver/config.h"

#ifdef __MINGW32CE__
#include "wincecompat.h"
#endif

#include "libiberty.h"
#include "ansidecl.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <setjmp.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
/* On some systems such as MinGW, alloca is declared in malloc.h
   (there is no alloca.h).  */
#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#if !HAVE_DECL_STRERROR
#ifndef strerror
extern char *strerror (int);	/* X3.159-1989  4.11.6.2 */
#endif
#endif

#if !HAVE_DECL_PERROR
#ifndef perror
extern void perror (const char *);
#endif
#endif

#if !HAVE_DECL_VASPRINTF
extern int vasprintf(char **strp, const char *fmt, va_list ap);
#endif
#if !HAVE_DECL_VSNPRINTF
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

/* Define underscore macro, if not available, to be able to use it inside
   code shared with gdb in common directory.  */
#ifndef _
#define _(String) (String)
#endif

#ifdef IN_PROCESS_AGENT
#  define PROG "ipa"
#else
#  define PROG "gdbserver"
#endif

/* A type used for binary buffers.  */
typedef unsigned char gdb_byte;

#include "ptid.h"
#include "buffer.h"
#include "xml-utils.h"
#include "gdb_locale.h"

/* FIXME: This should probably be autoconf'd for.  It's an integer type at
   least the size of a (void *).  */
typedef long long CORE_ADDR;

typedef long long LONGEST;
typedef unsigned long long ULONGEST;

/* Generic information for tracking a list of ``inferiors'' - threads,
   processes, etc.  */
struct inferior_list
{
  struct inferior_list_entry *head;
  struct inferior_list_entry *tail;
};
struct inferior_list_entry
{
  ptid_t id;
  struct inferior_list_entry *next;
};

struct thread_info;
struct process_info;
struct regcache;

#include "regcache.h"
#include "gdb/signals.h"
#include "gdb_signals.h"
#include "target.h"
#include "mem-break.h"
#include "gdbthread.h"

struct dll_info
{
  struct inferior_list_entry entry;
  char *name;
  CORE_ADDR base_addr;
};

struct sym_cache;
struct breakpoint;
struct raw_breakpoint;
struct fast_tracepoint_jump;
struct process_info_private;

struct process_info
{
  struct inferior_list_entry head;

  /* Nonzero if this child process was attached rather than
     spawned.  */
  int attached;

  /* True if GDB asked us to detach from this process, but we remained
     attached anyway.  */
  int gdb_detached;

  /* The symbol cache.  */
  struct sym_cache *symbol_cache;

  /* The list of memory breakpoints.  */
  struct breakpoint *breakpoints;

  /* The list of raw memory breakpoints.  */
  struct raw_breakpoint *raw_breakpoints;

  /* The list of installed fast tracepoints.  */
  struct fast_tracepoint_jump *fast_tracepoint_jumps;

  /* Private target data.  */
  struct process_info_private *private;
};

/* Return a pointer to the process that corresponds to the current
   thread (current_inferior).  It is an error to call this if there is
   no current thread selected.  */

struct process_info *current_process (void);
struct process_info *get_thread_process (struct thread_info *);

/* Target-specific functions */

void initialize_low ();

/* From inferiors.c.  */

extern struct inferior_list all_processes;
extern struct inferior_list all_dlls;
extern int dlls_changed;
extern void clear_dlls (void);

void add_inferior_to_list (struct inferior_list *list,
			   struct inferior_list_entry *new_inferior);
void for_each_inferior (struct inferior_list *list,
			void (*action) (struct inferior_list_entry *));

extern struct thread_info *current_inferior;
void remove_inferior (struct inferior_list *list,
		      struct inferior_list_entry *entry);

struct process_info *add_process (int pid, int attached);
void remove_process (struct process_info *process);
struct process_info *find_process_pid (int pid);
int have_started_inferiors_p (void);
int have_attached_inferiors_p (void);

ptid_t thread_id_to_gdb_id (ptid_t);
ptid_t thread_to_gdb_id (struct thread_info *);
ptid_t gdb_id_to_thread_id (ptid_t);

void clear_inferiors (void);
struct inferior_list_entry *find_inferior
     (struct inferior_list *,
      int (*func) (struct inferior_list_entry *,
		   void *),
      void *arg);
struct inferior_list_entry *find_inferior_id (struct inferior_list *list,
					      ptid_t id);
void *inferior_target_data (struct thread_info *);
void set_inferior_target_data (struct thread_info *, void *);
void *inferior_regcache_data (struct thread_info *);
void set_inferior_regcache_data (struct thread_info *, void *);

void loaded_dll (const char *name, CORE_ADDR base_addr);
void unloaded_dll (const char *name, CORE_ADDR base_addr);

/* Public variables in server.c */

extern ptid_t cont_thread;
extern ptid_t general_thread;

extern int server_waiting;
extern int debug_threads;
extern int debug_hw_points;
extern int pass_signals[];
extern int program_signals[];
extern int program_signals_p;

extern jmp_buf toplevel;

extern int disable_packet_vCont;
extern int disable_packet_Tthread;
extern int disable_packet_qC;
extern int disable_packet_qfThreadInfo;

extern int run_once;
extern int multi_process;
extern int non_stop;

extern int disable_randomization;

#if USE_WIN32API
#include <winsock2.h>
typedef SOCKET gdb_fildes_t;
#else
typedef int gdb_fildes_t;
#endif

/* Functions from event-loop.c.  */
typedef void *gdb_client_data;
typedef int (handler_func) (int, gdb_client_data);
typedef int (callback_handler_func) (gdb_client_data);

extern void delete_file_handler (gdb_fildes_t fd);
extern void add_file_handler (gdb_fildes_t fd, handler_func *proc,
			      gdb_client_data client_data);
extern int append_callback_event (callback_handler_func *proc,
				   gdb_client_data client_data);
extern void delete_callback_event (int id);

extern void start_event_loop (void);
extern void initialize_event_loop (void);

/* Functions from server.c.  */
extern int handle_serial_event (int err, gdb_client_data client_data);
extern int handle_target_event (int err, gdb_client_data client_data);

/* Functions from hostio.c.  */
extern int handle_vFile (char *, int, int *);

/* Functions from hostio-errno.c.  */
extern void hostio_last_error_from_errno (char *own_buf);

/* From remote-utils.c */

extern int remote_debug;
extern gdb_fildes_t remote_desc;
extern gdb_fildes_t listen_desc;
extern int noack_mode;
extern int transport_is_reliable;

int gdb_connected (void);

#define STDIO_CONNECTION_NAME "stdio"
int remote_connection_is_stdio (void);

ptid_t read_ptid (char *buf, char **obuf);
char *write_ptid (char *buf, ptid_t ptid);

int putpkt (char *buf);
int putpkt_binary (char *buf, int len);
int putpkt_notif (char *buf);
int getpkt (char *buf);
void remote_prepare (char *name);
void remote_open (char *name);
void remote_close (void);
void write_ok (char *buf);
void write_enn (char *buf);
void initialize_async_io (void);
void enable_async_io (void);
void disable_async_io (void);
void check_remote_input_interrupt_request (void);
void convert_ascii_to_int (const char *from, unsigned char *to, int n);
void convert_int_to_ascii (const unsigned char *from, char *to, int n);
void new_thread_notify (int id);
void dead_thread_notify (int id);
void prepare_resume_reply (char *buf, ptid_t ptid,
			   struct target_waitstatus *status);

const char *decode_address_to_semicolon (CORE_ADDR *addrp, const char *start);
void decode_address (CORE_ADDR *addrp, const char *start, int len);
void decode_m_packet (char *from, CORE_ADDR * mem_addr_ptr,
		      unsigned int *len_ptr);
void decode_M_packet (char *from, CORE_ADDR * mem_addr_ptr,
		      unsigned int *len_ptr, unsigned char **to_p);
int decode_X_packet (char *from, int packet_len, CORE_ADDR * mem_addr_ptr,
		     unsigned int *len_ptr, unsigned char **to_p);
int decode_xfer_write (char *buf, int packet_len,
		       CORE_ADDR *offset, unsigned int *len,
		       unsigned char *data);
int decode_search_memory_packet (const char *buf, int packet_len,
				 CORE_ADDR *start_addrp,
				 CORE_ADDR *search_space_lenp,
				 gdb_byte *pattern,
				 unsigned int *pattern_lenp);

int unhexify (char *bin, const char *hex, int count);
int hexify (char *hex, const char *bin, int count);
int remote_escape_output (const gdb_byte *buffer, int len,
			  gdb_byte *out_buf, int *out_len,
			  int out_maxlen);
char *unpack_varlen_hex (char *buff,  ULONGEST *result);

void clear_symbol_cache (struct sym_cache **symcache_p);
int look_up_one_symbol (const char *name, CORE_ADDR *addrp, int may_ask_gdb);

int relocate_instruction (CORE_ADDR *to, CORE_ADDR oldloc);

void monitor_output (const char *msg);

/* Functions from utils.c */
#include "common-utils.h"

void perror_with_name (const char *string);
void error (const char *string,...) ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 2);
void fatal (const char *string,...) ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 2);
void warning (const char *string,...) ATTRIBUTE_PRINTF (1, 2);
char *paddress (CORE_ADDR addr);
char *pulongest (ULONGEST u);
char *plongest (LONGEST l);
char *phex_nz (ULONGEST l, int sizeof_l);
char *pfildes (gdb_fildes_t fd);

#include "gdb_assert.h"

/* Maximum number of bytes to read/write at once.  The value here
   is chosen to fill up a packet (the headers account for the 32).  */
#define MAXBUFBYTES(N) (((N)-32)/2)

/* Buffer sizes for transferring memory, registers, etc.   Set to a constant
   value to accomodate multiple register formats.  This value must be at least
   as large as the largest register set supported by gdbserver.  */
#define PBUFSIZ 16384

/* Functions from tracepoint.c */

/* Size for a small buffer to report problems from the in-process
   agent back to GDBserver.  */
#define IPA_BUFSIZ 100

void initialize_tracepoint (void);

extern int tracing;
extern int disconnected_tracing;

void tracepoint_look_up_symbols (void);

void stop_tracing (void);

int handle_tracepoint_general_set (char *own_buf);
int handle_tracepoint_query (char *own_buf);

int tracepoint_finished_step (struct thread_info *tinfo, CORE_ADDR stop_pc);
int tracepoint_was_hit (struct thread_info *tinfo, CORE_ADDR stop_pc);

void release_while_stepping_state_list (struct thread_info *tinfo);

extern int current_traceframe;

int in_readonly_region (CORE_ADDR addr, ULONGEST length);
int traceframe_read_mem (int tfnum, CORE_ADDR addr,
			 unsigned char *buf, ULONGEST length,
			 ULONGEST *nbytes);
int fetch_traceframe_registers (int tfnum,
				struct regcache *regcache,
				int regnum);

int traceframe_read_sdata (int tfnum, ULONGEST offset,
			   unsigned char *buf, ULONGEST length,
			   ULONGEST *nbytes);

int traceframe_read_info (int tfnum, struct buffer *buffer);

/* If a thread is determined to be collecting a fast tracepoint, this
   structure holds the collect status.  */

struct fast_tpoint_collect_status
{
  /* The tracepoint that is presently being collected.  */
  int tpoint_num;
  CORE_ADDR tpoint_addr;

  /* The address range in the jump pad of where the original
     instruction the tracepoint jump was inserted was relocated
     to.  */
  CORE_ADDR adjusted_insn_addr;
  CORE_ADDR adjusted_insn_addr_end;
};

int fast_tracepoint_collecting (CORE_ADDR thread_area,
				CORE_ADDR stop_pc,
				struct fast_tpoint_collect_status *status);
void force_unlock_trace_buffer (void);

int handle_tracepoint_bkpts (struct thread_info *tinfo, CORE_ADDR stop_pc);

#ifdef IN_PROCESS_AGENT
void initialize_low_tracepoint (void);
void supply_fast_tracepoint_registers (struct regcache *regcache,
				       const unsigned char *regs);
void supply_static_tracepoint_registers (struct regcache *regcache,
					 const unsigned char *regs,
					 CORE_ADDR pc);
void set_trampoline_buffer_space (CORE_ADDR begin, CORE_ADDR end,
				  char *errmsg);
#else
void stop_tracing (void);

int claim_trampoline_space (ULONGEST used, CORE_ADDR *trampoline);
int have_fast_tracepoint_trampoline_buffer (char *msgbuf);
void gdb_agent_about_to_close (int pid);
#endif

struct traceframe;
struct eval_agent_expr_context;

/* Do memory copies for bytecodes.  */
/* Do the recording of memory blocks for actions and bytecodes.  */

int agent_mem_read (struct eval_agent_expr_context *ctx,
		    unsigned char *to, CORE_ADDR from,
		    ULONGEST len);

LONGEST agent_get_trace_state_variable_value (int num);
void agent_set_trace_state_variable_value (int num, LONGEST val);

/* Record the value of a trace state variable.  */

int agent_tsv_read (struct eval_agent_expr_context *ctx, int n);
int agent_mem_read_string (struct eval_agent_expr_context *ctx,
			   unsigned char *to,
			   CORE_ADDR from,
			   ULONGEST len);

/* Bytecode compilation function vector.  */

struct emit_ops
{
  void (*emit_prologue) (void);
  void (*emit_epilogue) (void);
  void (*emit_add) (void);
  void (*emit_sub) (void);
  void (*emit_mul) (void);
  void (*emit_lsh) (void);
  void (*emit_rsh_signed) (void);
  void (*emit_rsh_unsigned) (void);
  void (*emit_ext) (int arg);
  void (*emit_log_not) (void);
  void (*emit_bit_and) (void);
  void (*emit_bit_or) (void);
  void (*emit_bit_xor) (void);
  void (*emit_bit_not) (void);
  void (*emit_equal) (void);
  void (*emit_less_signed) (void);
  void (*emit_less_unsigned) (void);
  void (*emit_ref) (int size);
  void (*emit_if_goto) (int *offset_p, int *size_p);
  void (*emit_goto) (int *offset_p, int *size_p);
  void (*write_goto_address) (CORE_ADDR from, CORE_ADDR to, int size);
  void (*emit_const) (LONGEST num);
  void (*emit_call) (CORE_ADDR fn);
  void (*emit_reg) (int reg);
  void (*emit_pop) (void);
  void (*emit_stack_flush) (void);
  void (*emit_zero_ext) (int arg);
  void (*emit_swap) (void);
  void (*emit_stack_adjust) (int n);

  /* Emit code for a generic function that takes one fixed integer
     argument and returns a 64-bit int (for instance, tsv getter).  */
  void (*emit_int_call_1) (CORE_ADDR fn, int arg1);

  /* Emit code for a generic function that takes one fixed integer
     argument and a 64-bit int from the top of the stack, and returns
     nothing (for instance, tsv setter).  */
  void (*emit_void_call_2) (CORE_ADDR fn, int arg1);

  /* Emit code specialized for common combinations of compare followed
     by a goto.  */
  void (*emit_eq_goto) (int *offset_p, int *size_p);
  void (*emit_ne_goto) (int *offset_p, int *size_p);
  void (*emit_lt_goto) (int *offset_p, int *size_p);
  void (*emit_le_goto) (int *offset_p, int *size_p);
  void (*emit_gt_goto) (int *offset_p, int *size_p);
  void (*emit_ge_goto) (int *offset_p, int *size_p);
};

/* Returns the address of the get_raw_reg function in the IPA.  */
CORE_ADDR get_raw_reg_func_addr (void);
/* Returns the address of the get_trace_state_variable_value
   function in the IPA.  */
CORE_ADDR get_get_tsv_func_addr (void);
/* Returns the address of the set_trace_state_variable_value
   function in the IPA.  */
CORE_ADDR get_set_tsv_func_addr (void);

extern CORE_ADDR current_insn_ptr;
extern int emit_error;

/* Version information, from version.c.  */
extern const char version[];
extern const char host_name[];

#endif /* SERVER_H */
