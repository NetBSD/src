/* Trace file TFILE format support in GDB.

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
#include "readline/tilde.h"
#include "filestuff.h"
#include "rsp-low.h" /* bin2hex */
#include "regcache.h"
#include "inferior.h"
#include "gdbthread.h"
#include "exec.h" /* exec_bfd */
#include "completer.h"
#include "filenames.h"
#include "remote.h"
#include "xml-tdesc.h"
#include "target-descriptions.h"
#include "buffer.h"
#include <algorithm>

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

/* TFILE trace writer.  */

struct tfile_trace_file_writer
{
  struct trace_file_writer base;

  /* File pointer to tfile trace file.  */
  FILE *fp;
  /* Path name of the tfile trace file.  */
  char *pathname;
};

/* This is the implementation of trace_file_write_ops method
   target_save.  We just call the generic target
   target_save_trace_data to do target-side saving.  */

static int
tfile_target_save (struct trace_file_writer *self,
		   const char *filename)
{
  int err = target_save_trace_data (filename);

  return (err >= 0);
}

/* This is the implementation of trace_file_write_ops method
   dtor.  */

static void
tfile_dtor (struct trace_file_writer *self)
{
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;

  xfree (writer->pathname);

  if (writer->fp != NULL)
    fclose (writer->fp);
}

/* This is the implementation of trace_file_write_ops method
   start.  It creates the trace file FILENAME and registers some
   cleanups.  */

static void
tfile_start (struct trace_file_writer *self, const char *filename)
{
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;

  writer->pathname = tilde_expand (filename);
  writer->fp = gdb_fopen_cloexec (writer->pathname, "wb");
  if (writer->fp == NULL)
    error (_("Unable to open file '%s' for saving trace data (%s)"),
	   writer->pathname, safe_strerror (errno));
}

/* This is the implementation of trace_file_write_ops method
   write_header.  Write the TFILE header.  */

static void
tfile_write_header (struct trace_file_writer *self)
{
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;
  int written;

  /* Write a file header, with a high-bit-set char to indicate a
     binary file, plus a hint as what this file is, and a version
     number in case of future needs.  */
  written = fwrite ("\x7fTRACE0\n", 8, 1, writer->fp);
  if (written < 1)
    perror_with_name (writer->pathname);
}

/* This is the implementation of trace_file_write_ops method
   write_regblock_type.  Write the size of register block.  */

static void
tfile_write_regblock_type (struct trace_file_writer *self, int size)
{
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;

  fprintf (writer->fp, "R %x\n", size);
}

/* This is the implementation of trace_file_write_ops method
   write_status.  */

static void
tfile_write_status (struct trace_file_writer *self,
		    struct trace_status *ts)
{
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;

  fprintf (writer->fp, "status %c;%s",
	   (ts->running ? '1' : '0'), stop_reason_names[ts->stop_reason]);
  if (ts->stop_reason == tracepoint_error
      || ts->stop_reason == trace_stop_command)
    {
      char *buf = (char *) alloca (strlen (ts->stop_desc) * 2 + 1);

      bin2hex ((gdb_byte *) ts->stop_desc, buf, strlen (ts->stop_desc));
      fprintf (writer->fp, ":%s", buf);
    }
  fprintf (writer->fp, ":%x", ts->stopping_tracepoint);
  if (ts->traceframe_count >= 0)
    fprintf (writer->fp, ";tframes:%x", ts->traceframe_count);
  if (ts->traceframes_created >= 0)
    fprintf (writer->fp, ";tcreated:%x", ts->traceframes_created);
  if (ts->buffer_free >= 0)
    fprintf (writer->fp, ";tfree:%x", ts->buffer_free);
  if (ts->buffer_size >= 0)
    fprintf (writer->fp, ";tsize:%x", ts->buffer_size);
  if (ts->disconnected_tracing)
    fprintf (writer->fp, ";disconn:%x", ts->disconnected_tracing);
  if (ts->circular_buffer)
    fprintf (writer->fp, ";circular:%x", ts->circular_buffer);
  if (ts->start_time)
    {
      fprintf (writer->fp, ";starttime:%s",
      phex_nz (ts->start_time, sizeof (ts->start_time)));
    }
  if (ts->stop_time)
    {
      fprintf (writer->fp, ";stoptime:%s",
      phex_nz (ts->stop_time, sizeof (ts->stop_time)));
    }
  if (ts->notes != NULL)
    {
      char *buf = (char *) alloca (strlen (ts->notes) * 2 + 1);

      bin2hex ((gdb_byte *) ts->notes, buf, strlen (ts->notes));
      fprintf (writer->fp, ";notes:%s", buf);
    }
  if (ts->user_name != NULL)
    {
      char *buf = (char *) alloca (strlen (ts->user_name) * 2 + 1);

      bin2hex ((gdb_byte *) ts->user_name, buf, strlen (ts->user_name));
      fprintf (writer->fp, ";username:%s", buf);
    }
  fprintf (writer->fp, "\n");
}

/* This is the implementation of trace_file_write_ops method
   write_uploaded_tsv.  */

static void
tfile_write_uploaded_tsv (struct trace_file_writer *self,
			  struct uploaded_tsv *utsv)
{
  char *buf = NULL;
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;

  if (utsv->name)
    {
      buf = (char *) xmalloc (strlen (utsv->name) * 2 + 1);
      bin2hex ((gdb_byte *) (utsv->name), buf, strlen (utsv->name));
    }

  fprintf (writer->fp, "tsv %x:%s:%x:%s\n",
	   utsv->number, phex_nz (utsv->initial_value, 8),
	   utsv->builtin, buf != NULL ? buf : "");

  if (utsv->name)
    xfree (buf);
}

#define MAX_TRACE_UPLOAD 2000

/* This is the implementation of trace_file_write_ops method
   write_uploaded_tp.  */

static void
tfile_write_uploaded_tp (struct trace_file_writer *self,
			 struct uploaded_tp *utp)
{
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;
  int a;
  char *act;
  char buf[MAX_TRACE_UPLOAD];

  fprintf (writer->fp, "tp T%x:%s:%c:%x:%x",
	   utp->number, phex_nz (utp->addr, sizeof (utp->addr)),
	   (utp->enabled ? 'E' : 'D'), utp->step, utp->pass);
  if (utp->type == bp_fast_tracepoint)
    fprintf (writer->fp, ":F%x", utp->orig_size);
  if (utp->cond)
    fprintf (writer->fp,
	     ":X%x,%s", (unsigned int) strlen (utp->cond) / 2,
	     utp->cond);
  fprintf (writer->fp, "\n");
  for (a = 0; VEC_iterate (char_ptr, utp->actions, a, act); ++a)
    fprintf (writer->fp, "tp A%x:%s:%s\n",
	     utp->number, phex_nz (utp->addr, sizeof (utp->addr)), act);
  for (a = 0; VEC_iterate (char_ptr, utp->step_actions, a, act); ++a)
    fprintf (writer->fp, "tp S%x:%s:%s\n",
	     utp->number, phex_nz (utp->addr, sizeof (utp->addr)), act);
  if (utp->at_string)
    {
      encode_source_string (utp->number, utp->addr,
			    "at", utp->at_string, buf, MAX_TRACE_UPLOAD);
      fprintf (writer->fp, "tp Z%s\n", buf);
    }
  if (utp->cond_string)
    {
      encode_source_string (utp->number, utp->addr,
			    "cond", utp->cond_string,
			    buf, MAX_TRACE_UPLOAD);
      fprintf (writer->fp, "tp Z%s\n", buf);
    }
  for (a = 0; VEC_iterate (char_ptr, utp->cmd_strings, a, act); ++a)
    {
      encode_source_string (utp->number, utp->addr, "cmd", act,
			    buf, MAX_TRACE_UPLOAD);
      fprintf (writer->fp, "tp Z%s\n", buf);
    }
  fprintf (writer->fp, "tp V%x:%s:%x:%s\n",
	   utp->number, phex_nz (utp->addr, sizeof (utp->addr)),
	   utp->hit_count,
	   phex_nz (utp->traceframe_usage,
		    sizeof (utp->traceframe_usage)));
}

/* This is the implementation of trace_file_write_ops method
   write_tdesc.  */

static void
tfile_write_tdesc (struct trace_file_writer *self)
{
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;
  char *tdesc = target_fetch_description_xml (&current_target);
  char *ptr = tdesc;
  char *next;

  if (tdesc == NULL)
    return;

  /* Write tdesc line by line, prefixing each line with "tdesc ".  */
  while (ptr != NULL)
    {
      next = strchr (ptr, '\n');
      if (next != NULL)
	{
	  fprintf (writer->fp, "tdesc %.*s\n", (int) (next - ptr), ptr);
	  /* Skip the \n.  */
	  next++;
	}
      else if (*ptr != '\0')
	{
	  /* Last line, doesn't have a newline.  */
	  fprintf (writer->fp, "tdesc %s\n", ptr);
	}
      ptr = next;
    }

  xfree (tdesc);
}

/* This is the implementation of trace_file_write_ops method
   write_definition_end.  */

static void
tfile_write_definition_end (struct trace_file_writer *self)
{
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;

  fprintf (writer->fp, "\n");
}

/* This is the implementation of trace_file_write_ops method
   write_raw_data.  */

static void
tfile_write_raw_data (struct trace_file_writer *self, gdb_byte *buf,
		      LONGEST len)
{
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;

  if (fwrite (buf, len, 1, writer->fp) < 1)
    perror_with_name (writer->pathname);
}

/* This is the implementation of trace_file_write_ops method
   end.  */

static void
tfile_end (struct trace_file_writer *self)
{
  struct tfile_trace_file_writer *writer
    = (struct tfile_trace_file_writer *) self;
  uint32_t gotten = 0;

  /* Mark the end of trace data.  */
  if (fwrite (&gotten, 4, 1, writer->fp) < 1)
    perror_with_name (writer->pathname);
}

/* Operations to write trace buffers into TFILE format.  */

static const struct trace_file_write_ops tfile_write_ops =
{
  tfile_dtor,
  tfile_target_save,
  tfile_start,
  tfile_write_header,
  tfile_write_regblock_type,
  tfile_write_status,
  tfile_write_uploaded_tsv,
  tfile_write_uploaded_tp,
  tfile_write_tdesc,
  tfile_write_definition_end,
  tfile_write_raw_data,
  NULL,
  tfile_end,
};

/* Return a trace writer for TFILE format.  */

struct trace_file_writer *
tfile_trace_file_writer_new (void)
{
  struct tfile_trace_file_writer *writer
    = XNEW (struct tfile_trace_file_writer);

  writer->base.ops = &tfile_write_ops;
  writer->fp = NULL;
  writer->pathname = NULL;

  return (struct trace_file_writer *) writer;
}

/* target tfile command */

static struct target_ops tfile_ops;

/* Fill in tfile_ops with its defined operations and properties.  */

#define TRACE_HEADER_SIZE 8

#define TFILE_PID (1)

static char *trace_filename;
static int trace_fd = -1;
static off_t trace_frames_offset;
static off_t cur_offset;
static int cur_data_size;
int trace_regblock_size;
static struct buffer trace_tdesc;

static void tfile_append_tdesc_line (const char *line);
static void tfile_interp_line (char *line,
			       struct uploaded_tp **utpp,
			       struct uploaded_tsv **utsvp);

/* Read SIZE bytes into READBUF from the trace frame, starting at
   TRACE_FD's current position.  Note that this call `read'
   underneath, hence it advances the file's seek position.  Throws an
   error if the `read' syscall fails, or less than SIZE bytes are
   read.  */

static void
tfile_read (gdb_byte *readbuf, int size)
{
  int gotten;

  gotten = read (trace_fd, readbuf, size);
  if (gotten < 0)
    perror_with_name (trace_filename);
  else if (gotten < size)
    error (_("Premature end of file while reading trace file"));
}

static void
tfile_open (const char *arg, int from_tty)
{
  char *temp;
  struct cleanup *old_chain;
  int flags;
  int scratch_chan;
  char header[TRACE_HEADER_SIZE];
  char linebuf[1000]; /* Should be max remote packet size or so.  */
  gdb_byte byte;
  int bytes, i;
  struct trace_status *ts;
  struct uploaded_tp *uploaded_tps = NULL;
  struct uploaded_tsv *uploaded_tsvs = NULL;
  char *filename;

  target_preopen (from_tty);
  if (!arg)
    error (_("No trace file specified."));

  filename = tilde_expand (arg);
  if (!IS_ABSOLUTE_PATH(filename))
    {
      temp = concat (current_directory, "/", filename, (char *) NULL);
      xfree (filename);
      filename = temp;
    }

  old_chain = make_cleanup (xfree, filename);

  flags = O_BINARY | O_LARGEFILE;
  flags |= O_RDONLY;
  scratch_chan = gdb_open_cloexec (filename, flags, 0);
  if (scratch_chan < 0)
    perror_with_name (filename);

  /* Looks semi-reasonable.  Toss the old trace file and work on the new.  */

  discard_cleanups (old_chain);	/* Don't free filename any more.  */
  unpush_target (&tfile_ops);

  trace_filename = xstrdup (filename);
  trace_fd = scratch_chan;

  /* Make sure this is clear.  */
  buffer_free (&trace_tdesc);

  bytes = 0;
  /* Read the file header and test for validity.  */
  tfile_read ((gdb_byte *) &header, TRACE_HEADER_SIZE);

  bytes += TRACE_HEADER_SIZE;
  if (!(header[0] == 0x7f
	&& (startswith (header + 1, "TRACE0\n"))))
    error (_("File is not a valid trace file."));

  push_target (&tfile_ops);

  trace_regblock_size = 0;
  ts = current_trace_status ();
  /* We know we're working with a file.  Record its name.  */
  ts->filename = trace_filename;
  /* Set defaults in case there is no status line.  */
  ts->running_known = 0;
  ts->stop_reason = trace_stop_reason_unknown;
  ts->traceframe_count = -1;
  ts->buffer_free = 0;
  ts->disconnected_tracing = 0;
  ts->circular_buffer = 0;

  TRY
    {
      /* Read through a section of newline-terminated lines that
	 define things like tracepoints.  */
      i = 0;
      while (1)
	{
	  tfile_read (&byte, 1);

	  ++bytes;
	  if (byte == '\n')
	    {
	      /* Empty line marks end of the definition section.  */
	      if (i == 0)
		break;
	      linebuf[i] = '\0';
	      i = 0;
	      tfile_interp_line (linebuf, &uploaded_tps, &uploaded_tsvs);
	    }
	  else
	    linebuf[i++] = byte;
	  if (i >= 1000)
	    error (_("Excessively long lines in trace file"));
	}

      /* By now, tdesc lines have been read from tfile - let's parse them.  */
      target_find_description ();

      /* Record the starting offset of the binary trace data.  */
      trace_frames_offset = bytes;

      /* If we don't have a blocksize, we can't interpret the
	 traceframes.  */
      if (trace_regblock_size == 0)
	error (_("No register block size recorded in trace file"));
    }
  CATCH (ex, RETURN_MASK_ALL)
    {
      /* Remove the partially set up target.  */
      unpush_target (&tfile_ops);
      throw_exception (ex);
    }
  END_CATCH

  inferior_appeared (current_inferior (), TFILE_PID);
  inferior_ptid = pid_to_ptid (TFILE_PID);
  add_thread_silent (inferior_ptid);

  if (ts->traceframe_count <= 0)
    warning (_("No traceframes present in this file."));

  /* Add the file's tracepoints and variables into the current mix.  */

  /* Get trace state variables first, they may be checked when parsing
     uploaded commands.  */
  merge_uploaded_trace_state_variables (&uploaded_tsvs);

  merge_uploaded_tracepoints (&uploaded_tps);

  post_create_inferior (&tfile_ops, from_tty);
}

/* Interpret the given line from the definitions part of the trace
   file.  */

static void
tfile_interp_line (char *line, struct uploaded_tp **utpp,
		   struct uploaded_tsv **utsvp)
{
  char *p = line;

  if (startswith (p, "R "))
    {
      p += strlen ("R ");
      trace_regblock_size = strtol (p, &p, 16);
    }
  else if (startswith (p, "status "))
    {
      p += strlen ("status ");
      parse_trace_status (p, current_trace_status ());
    }
  else if (startswith (p, "tp "))
    {
      p += strlen ("tp ");
      parse_tracepoint_definition (p, utpp);
    }
  else if (startswith (p, "tsv "))
    {
      p += strlen ("tsv ");
      parse_tsv_definition (p, utsvp);
    }
  else if (startswith (p, "tdesc "))
    {
      p += strlen ("tdesc ");
      tfile_append_tdesc_line (p);
    }
  else
    warning (_("Ignoring trace file definition \"%s\""), line);
}

/* Close the trace file and generally clean up.  */

static void
tfile_close (struct target_ops *self)
{
  int pid;

  if (trace_fd < 0)
    return;

  pid = ptid_get_pid (inferior_ptid);
  inferior_ptid = null_ptid;	/* Avoid confusion from thread stuff.  */
  exit_inferior_silent (pid);

  close (trace_fd);
  trace_fd = -1;
  xfree (trace_filename);
  trace_filename = NULL;
  buffer_free (&trace_tdesc);

  trace_reset_local_state ();
}

static void
tfile_files_info (struct target_ops *t)
{
  printf_filtered ("\t`%s'\n", trace_filename);
}

static void
tfile_get_tracepoint_status (struct target_ops *self,
			     struct breakpoint *tp, struct uploaded_tp *utp)
{
  /* Other bits of trace status were collected as part of opening the
     trace files, so nothing to do here.  */
}

/* Given the position of a traceframe in the file, figure out what
   address the frame was collected at.  This would normally be the
   value of a collected PC register, but if not available, we
   improvise.  */

static CORE_ADDR
tfile_get_traceframe_address (off_t tframe_offset)
{
  CORE_ADDR addr = 0;
  short tpnum;
  struct tracepoint *tp;
  off_t saved_offset = cur_offset;

  /* FIXME dig pc out of collected registers.  */

  /* Fall back to using tracepoint address.  */
  lseek (trace_fd, tframe_offset, SEEK_SET);
  tfile_read ((gdb_byte *) &tpnum, 2);
  tpnum = (short) extract_signed_integer ((gdb_byte *) &tpnum, 2,
					  gdbarch_byte_order
					      (target_gdbarch ()));

  tp = get_tracepoint_by_number_on_target (tpnum);
  /* FIXME this is a poor heuristic if multiple locations.  */
  if (tp && tp->base.loc)
    addr = tp->base.loc->address;

  /* Restore our seek position.  */
  cur_offset = saved_offset;
  lseek (trace_fd, cur_offset, SEEK_SET);
  return addr;
}

/* Given a type of search and some parameters, scan the collection of
   traceframes in the file looking for a match.  When found, return
   both the traceframe and tracepoint number, otherwise -1 for
   each.  */

static int
tfile_trace_find (struct target_ops *self, enum trace_find_type type, int num,
		  CORE_ADDR addr1, CORE_ADDR addr2, int *tpp)
{
  short tpnum;
  int tfnum = 0, found = 0;
  unsigned int data_size;
  struct tracepoint *tp;
  off_t offset, tframe_offset;
  CORE_ADDR tfaddr;

  if (num == -1)
    {
      if (tpp)
        *tpp = -1;
      return -1;
    }

  lseek (trace_fd, trace_frames_offset, SEEK_SET);
  offset = trace_frames_offset;
  while (1)
    {
      tframe_offset = offset;
      tfile_read ((gdb_byte *) &tpnum, 2);
      tpnum = (short) extract_signed_integer ((gdb_byte *) &tpnum, 2,
					      gdbarch_byte_order
						  (target_gdbarch ()));
      offset += 2;
      if (tpnum == 0)
	break;
      tfile_read ((gdb_byte *) &data_size, 4);
      data_size = (unsigned int) extract_unsigned_integer
                                     ((gdb_byte *) &data_size, 4,
				      gdbarch_byte_order (target_gdbarch ()));
      offset += 4;

      if (type == tfind_number)
	{
	  /* Looking for a specific trace frame.  */
	  if (tfnum == num)
	    found = 1;
	}
      else
	{
	  /* Start from the _next_ trace frame.  */
	  if (tfnum > get_traceframe_number ())
	    {
	      switch (type)
		{
		case tfind_pc:
		  tfaddr = tfile_get_traceframe_address (tframe_offset);
		  if (tfaddr == addr1)
		    found = 1;
		  break;
		case tfind_tp:
		  tp = get_tracepoint (num);
		  if (tp && tpnum == tp->number_on_target)
		    found = 1;
		  break;
		case tfind_range:
		  tfaddr = tfile_get_traceframe_address (tframe_offset);
		  if (addr1 <= tfaddr && tfaddr <= addr2)
		    found = 1;
		  break;
		case tfind_outside:
		  tfaddr = tfile_get_traceframe_address (tframe_offset);
		  if (!(addr1 <= tfaddr && tfaddr <= addr2))
		    found = 1;
		  break;
		default:
		  internal_error (__FILE__, __LINE__, _("unknown tfind type"));
		}
	    }
	}

      if (found)
	{
	  if (tpp)
	    *tpp = tpnum;
	  cur_offset = offset;
	  cur_data_size = data_size;

	  return tfnum;
	}
      /* Skip past the traceframe's data.  */
      lseek (trace_fd, data_size, SEEK_CUR);
      offset += data_size;
      /* Update our own count of traceframes.  */
      ++tfnum;
    }
  /* Did not find what we were looking for.  */
  if (tpp)
    *tpp = -1;
  return -1;
}

/* Prototype of the callback passed to tframe_walk_blocks.  */
typedef int (*walk_blocks_callback_func) (char blocktype, void *data);

/* Callback for traceframe_walk_blocks, used to find a given block
   type in a traceframe.  */

static int
match_blocktype (char blocktype, void *data)
{
  char *wantedp = (char *) data;

  if (*wantedp == blocktype)
    return 1;

  return 0;
}

/* Walk over all traceframe block starting at POS offset from
   CUR_OFFSET, and call CALLBACK for each block found, passing in DATA
   unmodified.  If CALLBACK returns true, this returns the position in
   the traceframe where the block is found, relative to the start of
   the traceframe (cur_offset).  Returns -1 if no callback call
   returned true, indicating that all blocks have been walked.  */

static int
traceframe_walk_blocks (walk_blocks_callback_func callback,
			int pos, void *data)
{
  /* Iterate through a traceframe's blocks, looking for a block of the
     requested type.  */

  lseek (trace_fd, cur_offset + pos, SEEK_SET);
  while (pos < cur_data_size)
    {
      unsigned short mlen;
      char block_type;

      tfile_read ((gdb_byte *) &block_type, 1);

      ++pos;

      if ((*callback) (block_type, data))
	return pos;

      switch (block_type)
	{
	case 'R':
	  lseek (trace_fd, cur_offset + pos + trace_regblock_size, SEEK_SET);
	  pos += trace_regblock_size;
	  break;
	case 'M':
	  lseek (trace_fd, cur_offset + pos + 8, SEEK_SET);
	  tfile_read ((gdb_byte *) &mlen, 2);
          mlen = (unsigned short)
                extract_unsigned_integer ((gdb_byte *) &mlen, 2,
                                          gdbarch_byte_order
                                              (target_gdbarch ()));
	  lseek (trace_fd, mlen, SEEK_CUR);
	  pos += (8 + 2 + mlen);
	  break;
	case 'V':
	  lseek (trace_fd, cur_offset + pos + 4 + 8, SEEK_SET);
	  pos += (4 + 8);
	  break;
	default:
	  error (_("Unknown block type '%c' (0x%x) in trace frame"),
		 block_type, block_type);
	  break;
	}
    }

  return -1;
}

/* Convenience wrapper around traceframe_walk_blocks.  Looks for the
   position offset of a block of type TYPE_WANTED in the current trace
   frame, starting at POS.  Returns -1 if no such block was found.  */

static int
traceframe_find_block_type (char type_wanted, int pos)
{
  return traceframe_walk_blocks (match_blocktype, pos, &type_wanted);
}

/* Look for a block of saved registers in the traceframe, and get the
   requested register from it.  */

static void
tfile_fetch_registers (struct target_ops *ops,
		       struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int offset, regn, regsize, dummy;

  /* An uninitialized reg size says we're not going to be
     successful at getting register blocks.  */
  if (!trace_regblock_size)
    return;

  if (traceframe_find_block_type ('R', 0) >= 0)
    {
      gdb_byte *regs = (gdb_byte *) alloca (trace_regblock_size);

      tfile_read (regs, trace_regblock_size);

      for (regn = 0; regn < gdbarch_num_regs (gdbarch); regn++)
	{
	  if (!remote_register_number_and_offset (get_regcache_arch (regcache),
						  regn, &dummy, &offset))
	    continue;

	  regsize = register_size (gdbarch, regn);
	  /* Make sure we stay within block bounds.  */
	  if (offset + regsize > trace_regblock_size)
	    break;
	  if (regcache_register_status (regcache, regn) == REG_UNKNOWN)
	    {
	      if (regno == regn)
		{
		  regcache_raw_supply (regcache, regno, regs + offset);
		  break;
		}
	      else if (regno == -1)
		{
		  regcache_raw_supply (regcache, regn, regs + offset);
		}
	    }
	}
    }
  else
    tracefile_fetch_registers (regcache, regno);
}

static enum target_xfer_status
tfile_xfer_partial_features (struct target_ops *ops, const char *annex,
			     gdb_byte *readbuf, const gdb_byte *writebuf,
			     ULONGEST offset, ULONGEST len,
			     ULONGEST *xfered_len)
{
  if (strcmp (annex, "target.xml"))
    return TARGET_XFER_E_IO;

  if (readbuf == NULL)
    error (_("tfile_xfer_partial: tdesc is read-only"));

  if (trace_tdesc.used_size == 0)
    return TARGET_XFER_E_IO;

  if (offset >= trace_tdesc.used_size)
    return TARGET_XFER_EOF;

  if (len > trace_tdesc.used_size - offset)
    len = trace_tdesc.used_size - offset;

  memcpy (readbuf, trace_tdesc.buffer + offset, len);
  *xfered_len = len;

  return TARGET_XFER_OK;
}

static enum target_xfer_status
tfile_xfer_partial (struct target_ops *ops, enum target_object object,
		    const char *annex, gdb_byte *readbuf,
		    const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		    ULONGEST *xfered_len)
{
  /* We're only doing regular memory and tdesc for now.  */
  if (object == TARGET_OBJECT_AVAILABLE_FEATURES)
    return tfile_xfer_partial_features (ops, annex, readbuf, writebuf,
					offset, len, xfered_len);
  if (object != TARGET_OBJECT_MEMORY)
    return TARGET_XFER_E_IO;

  if (readbuf == NULL)
    error (_("tfile_xfer_partial: trace file is read-only"));

  if (get_traceframe_number () != -1)
    {
      int pos = 0;
      enum target_xfer_status res;
      /* Records the lowest available address of all blocks that
	 intersects the requested range.  */
      ULONGEST low_addr_available = 0;

      /* Iterate through the traceframe's blocks, looking for
	 memory.  */
      while ((pos = traceframe_find_block_type ('M', pos)) >= 0)
	{
	  ULONGEST maddr, amt;
	  unsigned short mlen;
	  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());

	  tfile_read ((gdb_byte *) &maddr, 8);
	  maddr = extract_unsigned_integer ((gdb_byte *) &maddr, 8,
					    byte_order);
	  tfile_read ((gdb_byte *) &mlen, 2);
	  mlen = (unsigned short)
	    extract_unsigned_integer ((gdb_byte *) &mlen, 2, byte_order);

	  /* If the block includes the first part of the desired
	     range, return as much it has; GDB will re-request the
	     remainder, which might be in a different block of this
	     trace frame.  */
	  if (maddr <= offset && offset < (maddr + mlen))
	    {
	      amt = (maddr + mlen) - offset;
	      if (amt > len)
		amt = len;

	      if (maddr != offset)
	        lseek (trace_fd, offset - maddr, SEEK_CUR);
	      tfile_read (readbuf, amt);
	      *xfered_len = amt;
	      return TARGET_XFER_OK;
	    }

	  if (offset < maddr && maddr < (offset + len))
	    if (low_addr_available == 0 || low_addr_available > maddr)
	      low_addr_available = maddr;

	  /* Skip over this block.  */
	  pos += (8 + 2 + mlen);
	}

      /* Requested memory is unavailable in the context of traceframes,
	 and this address falls within a read-only section, fallback
	 to reading from executable, up to LOW_ADDR_AVAILABLE.  */
      if (offset < low_addr_available)
	len = std::min (len, low_addr_available - offset);
      res = exec_read_partial_read_only (readbuf, offset, len, xfered_len);

      if (res == TARGET_XFER_OK)
	return TARGET_XFER_OK;
      else
	{
	  /* No use trying further, we know some memory starting
	     at MEMADDR isn't available.  */
	  *xfered_len = len;
	  return TARGET_XFER_UNAVAILABLE;
	}
    }
  else
    {
      /* Fallback to reading from read-only sections.  */
      return section_table_read_available_memory (readbuf, offset, len,
						  xfered_len);
    }
}

/* Iterate through the blocks of a trace frame, looking for a 'V'
   block with a matching tsv number.  */

static int
tfile_get_trace_state_variable_value (struct target_ops *self,
				      int tsvnum, LONGEST *val)
{
  int pos;
  int found = 0;

  /* Iterate over blocks in current frame and find the last 'V'
     block in which tsv number is TSVNUM.  In one trace frame, there
     may be multiple 'V' blocks created for a given trace variable,
     and the last matched 'V' block contains the updated value.  */
  pos = 0;
  while ((pos = traceframe_find_block_type ('V', pos)) >= 0)
    {
      int vnum;

      tfile_read ((gdb_byte *) &vnum, 4);
      vnum = (int) extract_signed_integer ((gdb_byte *) &vnum, 4,
					   gdbarch_byte_order
					   (target_gdbarch ()));
      if (tsvnum == vnum)
	{
	  tfile_read ((gdb_byte *) val, 8);
	  *val = extract_signed_integer ((gdb_byte *) val, 8,
					 gdbarch_byte_order
					 (target_gdbarch ()));
	  found = 1;
	}
      pos += (4 + 8);
    }

  return found;
}

/* Callback for traceframe_walk_blocks.  Builds a traceframe_info
   object for the tfile target's current traceframe.  */

static int
build_traceframe_info (char blocktype, void *data)
{
  struct traceframe_info *info = (struct traceframe_info *) data;

  switch (blocktype)
    {
    case 'M':
      {
	struct mem_range *r;
	ULONGEST maddr;
	unsigned short mlen;

	tfile_read ((gdb_byte *) &maddr, 8);
	maddr = extract_unsigned_integer ((gdb_byte *) &maddr, 8,
					  gdbarch_byte_order
					  (target_gdbarch ()));
	tfile_read ((gdb_byte *) &mlen, 2);
	mlen = (unsigned short)
		extract_unsigned_integer ((gdb_byte *) &mlen,
					  2, gdbarch_byte_order
					  (target_gdbarch ()));

	r = VEC_safe_push (mem_range_s, info->memory, NULL);

	r->start = maddr;
	r->length = mlen;
	break;
      }
    case 'V':
      {
	int vnum;

	tfile_read ((gdb_byte *) &vnum, 4);
	VEC_safe_push (int, info->tvars, vnum);
      }
    case 'R':
    case 'S':
      {
	break;
      }
    default:
      warning (_("Unhandled trace block type (%d) '%c ' "
		 "while building trace frame info."),
	       blocktype, blocktype);
      break;
    }

  return 0;
}

static struct traceframe_info *
tfile_traceframe_info (struct target_ops *self)
{
  struct traceframe_info *info = XCNEW (struct traceframe_info);

  traceframe_walk_blocks (build_traceframe_info, 0, info);
  return info;
}

/* Handles tdesc lines from tfile by appending the payload to
   a global trace_tdesc variable.  */

static void
tfile_append_tdesc_line (const char *line)
{
  buffer_grow_str (&trace_tdesc, line);
  buffer_grow_str (&trace_tdesc, "\n");
}

static void
init_tfile_ops (void)
{
  init_tracefile_ops (&tfile_ops);

  tfile_ops.to_shortname = "tfile";
  tfile_ops.to_longname = "Local trace dump file";
  tfile_ops.to_doc
    = "Use a trace file as a target.  Specify the filename of the trace file.";
  tfile_ops.to_open = tfile_open;
  tfile_ops.to_close = tfile_close;
  tfile_ops.to_fetch_registers = tfile_fetch_registers;
  tfile_ops.to_xfer_partial = tfile_xfer_partial;
  tfile_ops.to_files_info = tfile_files_info;
  tfile_ops.to_get_tracepoint_status = tfile_get_tracepoint_status;
  tfile_ops.to_trace_find = tfile_trace_find;
  tfile_ops.to_get_trace_state_variable_value
    = tfile_get_trace_state_variable_value;
  tfile_ops.to_traceframe_info = tfile_traceframe_info;
}

extern initialize_file_ftype _initialize_tracefile_tfile;

void
_initialize_tracefile_tfile (void)
{
  init_tfile_ops ();

  add_target_with_completer (&tfile_ops, filename_completer);
}
