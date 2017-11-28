/* Trace file support in GDB.

   Copyright (C) 1997-2017 Free Software Foundation, Inc.

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

#include "defs.h"
#include "tracefile.h"
#include "ctf.h"
#include "exec.h"
#include "regcache.h"

/* Helper macros.  */

#define TRACE_WRITE_R_BLOCK(writer, buf, size)	\
  writer->ops->frame_ops->write_r_block ((writer), (buf), (size))
#define TRACE_WRITE_M_BLOCK_HEADER(writer, addr, size)		  \
  writer->ops->frame_ops->write_m_block_header ((writer), (addr), \
						(size))
#define TRACE_WRITE_M_BLOCK_MEMORY(writer, buf, size)	  \
  writer->ops->frame_ops->write_m_block_memory ((writer), (buf), \
						(size))
#define TRACE_WRITE_V_BLOCK(writer, num, val)	\
  writer->ops->frame_ops->write_v_block ((writer), (num), (val))

/* Free trace file writer.  */

static void
trace_file_writer_xfree (void *arg)
{
  struct trace_file_writer *writer = (struct trace_file_writer *) arg;

  writer->ops->dtor (writer);
  xfree (writer);
}

/* Save tracepoint data to file named FILENAME through WRITER.  WRITER
   determines the trace file format.  If TARGET_DOES_SAVE is non-zero,
   the save is performed on the target, otherwise GDB obtains all trace
   data and saves it locally.  */

static void
trace_save (const char *filename, struct trace_file_writer *writer,
	    int target_does_save)
{
  struct trace_status *ts = current_trace_status ();
  struct uploaded_tp *uploaded_tps = NULL, *utp;
  struct uploaded_tsv *uploaded_tsvs = NULL, *utsv;

  ULONGEST offset = 0;
#define MAX_TRACE_UPLOAD 2000
  gdb_byte buf[MAX_TRACE_UPLOAD];
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());

  /* If the target is to save the data to a file on its own, then just
     send the command and be done with it.  */
  if (target_does_save)
    {
      if (!writer->ops->target_save (writer, filename))
	error (_("Target failed to save trace data to '%s'."),
	       filename);
      return;
    }

  /* Get the trace status first before opening the file, so if the
     target is losing, we can get out without touching files.  Since
     we're just calling this for side effects, we ignore the
     result.  */
  target_get_trace_status (ts);

  writer->ops->start (writer, filename);

  writer->ops->write_header (writer);

  /* Write descriptive info.  */

  /* Write out the size of a register block.  */
  writer->ops->write_regblock_type (writer, trace_regblock_size);

  /* Write out the target description info.  */
  writer->ops->write_tdesc (writer);

  /* Write out status of the tracing run (aka "tstatus" info).  */
  writer->ops->write_status (writer, ts);

  /* Note that we want to upload tracepoints and save those, rather
     than simply writing out the local ones, because the user may have
     changed tracepoints in GDB in preparation for a future tracing
     run, or maybe just mass-deleted all types of breakpoints as part
     of cleaning up.  So as not to contaminate the session, leave the
     data in its uploaded form, don't make into real tracepoints.  */

  /* Get trace state variables first, they may be checked when parsing
     uploaded commands.  */

  target_upload_trace_state_variables (&uploaded_tsvs);

  for (utsv = uploaded_tsvs; utsv; utsv = utsv->next)
    writer->ops->write_uploaded_tsv (writer, utsv);

  free_uploaded_tsvs (&uploaded_tsvs);

  target_upload_tracepoints (&uploaded_tps);

  for (utp = uploaded_tps; utp; utp = utp->next)
    target_get_tracepoint_status (NULL, utp);

  for (utp = uploaded_tps; utp; utp = utp->next)
    writer->ops->write_uploaded_tp (writer, utp);

  free_uploaded_tps (&uploaded_tps);

  /* Mark the end of the definition section.  */
  writer->ops->write_definition_end (writer);

  /* Get and write the trace data proper.  */
  while (1)
    {
      LONGEST gotten = 0;

      /* The writer supports writing the contents of trace buffer
	  directly to trace file.  Don't parse the contents of trace
	  buffer.  */
      if (writer->ops->write_trace_buffer != NULL)
	{
	  /* We ask for big blocks, in the hopes of efficiency, but
	     will take less if the target has packet size limitations
	     or some such.  */
	  gotten = target_get_raw_trace_data (buf, offset,
					      MAX_TRACE_UPLOAD);
	  if (gotten < 0)
	    error (_("Failure to get requested trace buffer data"));
	  /* No more data is forthcoming, we're done.  */
	  if (gotten == 0)
	    break;

	  writer->ops->write_trace_buffer (writer, buf, gotten);

	  offset += gotten;
	}
      else
	{
	  uint16_t tp_num;
	  uint32_t tf_size;
	  /* Parse the trace buffers according to how data are stored
	     in trace buffer in GDBserver.  */

	  gotten = target_get_raw_trace_data (buf, offset, 6);

	  if (gotten == 0)
	    break;

	  /* Read the first six bytes in, which is the tracepoint
	     number and trace frame size.  */
	  tp_num = (uint16_t)
	    extract_unsigned_integer (&buf[0], 2, byte_order);

	  tf_size = (uint32_t)
	    extract_unsigned_integer (&buf[2], 4, byte_order);

	  writer->ops->frame_ops->start (writer, tp_num);
	  gotten = 6;

	  if (tf_size > 0)
	    {
	      unsigned int block;

	      offset += 6;

	      for (block = 0; block < tf_size; )
		{
		  gdb_byte block_type;

		  /* We'll fetch one block each time, in order to
		     handle the extremely large 'M' block.  We first
		     fetch one byte to get the type of the block.  */
		  gotten = target_get_raw_trace_data (buf, offset, 1);
		  if (gotten < 1)
		    error (_("Failure to get requested trace buffer data"));

		  gotten = 1;
		  block += 1;
		  offset += 1;

		  block_type = buf[0];
		  switch (block_type)
		    {
		    case 'R':
		      gotten
			= target_get_raw_trace_data (buf, offset,
						     trace_regblock_size);
		      if (gotten < trace_regblock_size)
			error (_("Failure to get requested trace"
				 " buffer data"));

		      TRACE_WRITE_R_BLOCK (writer, buf,
					   trace_regblock_size);
		      break;
		    case 'M':
		      {
			unsigned short mlen;
			ULONGEST addr;
			LONGEST t;
			int j;

			t = target_get_raw_trace_data (buf,offset, 10);
			if (t < 10)
			  error (_("Failure to get requested trace"
				   " buffer data"));

			offset += 10;
			block += 10;

			gotten = 0;
			addr = (ULONGEST)
			  extract_unsigned_integer (buf, 8,
						    byte_order);
			mlen = (unsigned short)
			  extract_unsigned_integer (&buf[8], 2,
						    byte_order);

			TRACE_WRITE_M_BLOCK_HEADER (writer, addr,
						    mlen);

			/* The memory contents in 'M' block may be
			   very large.  Fetch the data from the target
			   and write them into file one by one.  */
			for (j = 0; j < mlen; )
			  {
			    unsigned int read_length;

			    if (mlen - j > MAX_TRACE_UPLOAD)
			      read_length = MAX_TRACE_UPLOAD;
			    else
			      read_length = mlen - j;

			    t = target_get_raw_trace_data (buf,
							   offset + j,
							   read_length);
			    if (t < read_length)
			      error (_("Failure to get requested"
				       " trace buffer data"));

			    TRACE_WRITE_M_BLOCK_MEMORY (writer, buf,
							read_length);

			    j += read_length;
			    gotten += read_length;
			  }

			break;
		      }
		    case 'V':
		      {
			int vnum;
			LONGEST val;

			gotten
			  = target_get_raw_trace_data (buf, offset,
						       12);
			if (gotten < 12)
			  error (_("Failure to get requested"
				   " trace buffer data"));

			vnum  = (int) extract_signed_integer (buf,
							      4,
							      byte_order);
			val
			  = extract_signed_integer (&buf[4], 8,
						    byte_order);

			TRACE_WRITE_V_BLOCK (writer, vnum, val);
		      }
		      break;
		    default:
		      error (_("Unknown block type '%c' (0x%x) in"
			       " trace frame"),
			     block_type, block_type);
		    }

		  block += gotten;
		  offset += gotten;
		}
	    }
	  else
	    offset += gotten;

	  writer->ops->frame_ops->end (writer);
	}
    }

  writer->ops->end (writer);
}

static void
tsave_command (char *args, int from_tty)
{
  int target_does_save = 0;
  char **argv;
  char *filename = NULL;
  struct cleanup *back_to;
  int generate_ctf = 0;
  struct trace_file_writer *writer = NULL;

  if (args == NULL)
    error_no_arg (_("file in which to save trace data"));

  argv = gdb_buildargv (args);
  back_to = make_cleanup_freeargv (argv);

  for (; *argv; ++argv)
    {
      if (strcmp (*argv, "-r") == 0)
	target_does_save = 1;
      else if (strcmp (*argv, "-ctf") == 0)
	generate_ctf = 1;
      else if (**argv == '-')
	error (_("unknown option `%s'"), *argv);
      else
	filename = *argv;
    }

  if (!filename)
    error_no_arg (_("file in which to save trace data"));

  if (generate_ctf)
    writer = ctf_trace_file_writer_new ();
  else
    writer = tfile_trace_file_writer_new ();

  make_cleanup (trace_file_writer_xfree, writer);

  trace_save (filename, writer, target_does_save);

  if (from_tty)
    printf_filtered (_("Trace data saved to %s '%s'.\n"),
		     generate_ctf ? "directory" : "file", filename);

  do_cleanups (back_to);
}

/* Save the trace data to file FILENAME of tfile format.  */

void
trace_save_tfile (const char *filename, int target_does_save)
{
  struct trace_file_writer *writer;
  struct cleanup *back_to;

  writer = tfile_trace_file_writer_new ();
  back_to = make_cleanup (trace_file_writer_xfree, writer);
  trace_save (filename, writer, target_does_save);
  do_cleanups (back_to);
}

/* Save the trace data to dir DIRNAME of ctf format.  */

void
trace_save_ctf (const char *dirname, int target_does_save)
{
  struct trace_file_writer *writer;
  struct cleanup *back_to;

  writer = ctf_trace_file_writer_new ();
  back_to = make_cleanup (trace_file_writer_xfree, writer);

  trace_save (dirname, writer, target_does_save);
  do_cleanups (back_to);
}

/* Fetch register data from tracefile, shared for both tfile and
   ctf.  */

void
tracefile_fetch_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  struct tracepoint *tp = get_tracepoint (get_tracepoint_number ());
  int regn;

  /* We get here if no register data has been found.  Mark registers
     as unavailable.  */
  for (regn = 0; regn < gdbarch_num_regs (gdbarch); regn++)
    regcache_raw_supply (regcache, regn, NULL);

  /* We can often usefully guess that the PC is going to be the same
     as the address of the tracepoint.  */
  if (tp == NULL || tp->base.loc == NULL)
    return;

  /* But don't try to guess if tracepoint is multi-location...  */
  if (tp->base.loc->next)
    {
      warning (_("Tracepoint %d has multiple "
		 "locations, cannot infer $pc"),
	       tp->base.number);
      return;
    }
  /* ... or does while-stepping.  */
  else if (tp->step_count > 0)
    {
      warning (_("Tracepoint %d does while-stepping, "
		 "cannot infer $pc"),
	       tp->base.number);
      return;
    }

  /* Guess what we can from the tracepoint location.  */
  gdbarch_guess_tracepoint_registers (gdbarch, regcache,
				      tp->base.loc->address);
}

/* This is the implementation of target_ops method to_has_all_memory.  */

static int
tracefile_has_all_memory (struct target_ops *ops)
{
  return 1;
}

/* This is the implementation of target_ops method to_has_memory.  */

static int
tracefile_has_memory (struct target_ops *ops)
{
  return 1;
}

/* This is the implementation of target_ops method to_has_stack.
   The target has a stack when GDB has already selected one trace
   frame.  */

static int
tracefile_has_stack (struct target_ops *ops)
{
  return get_traceframe_number () != -1;
}

/* This is the implementation of target_ops method to_has_registers.
   The target has registers when GDB has already selected one trace
   frame.  */

static int
tracefile_has_registers (struct target_ops *ops)
{
  return get_traceframe_number () != -1;
}

/* This is the implementation of target_ops method to_thread_alive.
   tracefile has one thread faked by GDB.  */

static int
tracefile_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  return 1;
}

/* This is the implementation of target_ops method to_get_trace_status.
   The trace status for a file is that tracing can never be run.  */

static int
tracefile_get_trace_status (struct target_ops *self, struct trace_status *ts)
{
  /* Other bits of trace status were collected as part of opening the
     trace files, so nothing to do here.  */

  return -1;
}

/* Initialize OPS for tracefile related targets.  */

void
init_tracefile_ops (struct target_ops *ops)
{
  ops->to_stratum = process_stratum;
  ops->to_get_trace_status = tracefile_get_trace_status;
  ops->to_has_all_memory = tracefile_has_all_memory;
  ops->to_has_memory = tracefile_has_memory;
  ops->to_has_stack = tracefile_has_stack;
  ops->to_has_registers = tracefile_has_registers;
  ops->to_thread_alive = tracefile_thread_alive;
  ops->to_magic = OPS_MAGIC;
}

extern initialize_file_ftype _initialize_tracefile;

void
_initialize_tracefile (void)
{
  add_com ("tsave", class_trace, tsave_command, _("\
Save the trace data to a file.\n\
Use the '-ctf' option to save the data to CTF format.\n\
Use the '-r' option to direct the target to save directly to the file,\n\
using its own filesystem."));
}
