/* This program does two things; it generates valid trace files, and
   it can also be traced so as to test trace file creation from
   GDB.  */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

char spbuf[200];

char trbuf[1000];
char *trptr;
char *tfsizeptr;

int testglob = 31415;

int testglob2 = 271828;

const int constglob = 10000;

int
start_trace_file (char *filename)
{
  int fd;

  fd = open (filename, O_WRONLY|O_CREAT|O_APPEND,
	     S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

  if (fd < 0)
    return fd;

  /* Write a file header, with a high-bit-set char to indicate a
     binary file, plus a hint as what this file is, and a version
     number in case of future needs.  */
  write (fd, "\x7fTRACE0\n", 8);

  return fd;
}

void
finish_trace_file (int fd)
{
  close (fd);
}


void
add_memory_block (char *addr, int size)
{
  short short_x;
  unsigned long long ll_x;

  *((char *) trptr) = 'M';
  trptr += 1;
  ll_x = (unsigned long) addr;
  memcpy (trptr, &ll_x, sizeof (unsigned long long));
  trptr += sizeof (unsigned long long);
  short_x = size;
  memcpy (trptr, &short_x, 2);
  trptr += 2;
  memcpy (trptr, addr, size);
  trptr += size;
}

void
write_basic_trace_file (void)
{
  int fd, int_x;
  short short_x;

  fd = start_trace_file ("basic.tf");

  /* The next part of the file consists of newline-separated lines
     defining status, tracepoints, etc.  The section is terminated by
     an empty line.  */

  /* Dump the size of the R (register) blocks in traceframes.  */
  snprintf (spbuf, sizeof spbuf, "R %x\n", 500 /* FIXME get from arch */);
  write (fd, spbuf, strlen (spbuf));

  /* Dump trace status, in the general form of the qTstatus reply.  */
  snprintf (spbuf, sizeof spbuf, "status 0;tstop:0;tframes:1;tcreated:1;tfree:100;tsize:1000\n");
  write (fd, spbuf, strlen (spbuf));

  /* Dump tracepoint definitions, in syntax similar to that used
     for reconnection uploads.  */
  /* FIXME need a portable way to print function address in hex */
  snprintf (spbuf, sizeof spbuf, "tp T1:%lx:E:0:0\n",
	    (long) &write_basic_trace_file);
  write (fd, spbuf, strlen (spbuf));
  /* (Note that we would only need actions defined if we wanted to
     test tdump.) */

  /* Empty line marks the end of the definition section.  */
  write (fd, "\n", 1);

  /* Make up a simulated trace buffer.  */
  /* (Encapsulate better if we're going to do lots of this; note that
     buffer endianness is the target program's enddianness.) */
  trptr = trbuf;
  short_x = 1;
  memcpy (trptr, &short_x, 2);
  trptr += 2;
  tfsizeptr = trptr;
  trptr += 4;
  add_memory_block (&testglob, sizeof (testglob));
  /* Divide a variable between two separate memory blocks.  */
  add_memory_block (&testglob2, 1);
  add_memory_block (((char*) &testglob2) + 1, sizeof (testglob2) - 1);
  /* Go back and patch in the frame size.  */
  int_x = trptr - tfsizeptr - sizeof (int);
  memcpy (tfsizeptr, &int_x, 4);

  /* Write end of tracebuffer marker.  */
  memset (trptr, 0, 6);
  trptr += 6;

  write (fd, trbuf, trptr - trbuf);

  finish_trace_file (fd);
}

/* Convert number NIB to a hex digit.  */

static int
tohex (int nib)
{
  if (nib < 10)
    return '0' + nib;
  else
    return 'a' + nib - 10;
}

int
bin2hex (const char *bin, char *hex, int count)
{
  int i;

  for (i = 0; i < count; i++)
    {
      *hex++ = tohex ((*bin >> 4) & 0xf);
      *hex++ = tohex (*bin++ & 0xf);
    }
  *hex = 0;
  return i;
}

void
write_error_trace_file (void)
{
  int fd;
  const char made_up[] = "made-up error";
  int len = sizeof (made_up) - 1;
  char *hex = alloca (len * 2 + 1);

  fd = start_trace_file ("error.tf");

  /* The next part of the file consists of newline-separated lines
     defining status, tracepoints, etc.  The section is terminated by
     an empty line.  */

  /* Dump the size of the R (register) blocks in traceframes.  */
  snprintf (spbuf, sizeof spbuf, "R %x\n", 500 /* FIXME get from arch */);
  write (fd, spbuf, strlen (spbuf));

  bin2hex (made_up, hex, len);

  /* Dump trace status, in the general form of the qTstatus reply.  */
  snprintf (spbuf, sizeof spbuf,
	    "status 0;"
	    "terror:%s:1;"
	    "tframes:0;tcreated:0;tfree:100;tsize:1000\n",
	    hex);
  write (fd, spbuf, strlen (spbuf));

  /* Dump tracepoint definitions, in syntax similar to that used
     for reconnection uploads.  */
  /* FIXME need a portable way to print function address in hex */
  snprintf (spbuf, sizeof spbuf, "tp T1:%lx:E:0:0\n",
	    (long) &write_basic_trace_file);
  write (fd, spbuf, strlen (spbuf));
  /* (Note that we would only need actions defined if we wanted to
     test tdump.) */

  /* Empty line marks the end of the definition section.  */
  write (fd, "\n", 1);

  trptr = trbuf;

  /* Write end of tracebuffer marker.  */
  memset (trptr, 0, 6);
  trptr += 6;

  write (fd, trbuf, trptr - trbuf);

  finish_trace_file (fd);
}

void
done_making_trace_files (void)
{
}

int
main (int argc, char **argv, char **envp)
{
  write_basic_trace_file ();

  write_error_trace_file ();

  done_making_trace_files ();

  return 0;
}

