/*
 *	$Id: binpatch.c,v 1.3 1994/02/11 07:02:53 chopps Exp $
 */

#include <sys/types.h>
#include <a.out.h>
#include <stdio.h>

extern char *optarg;
extern int optind;

volatile void error ();

int test = 1;
int testbss;
char foo = 23;


int
main(argc, argv)
     int argc;
     char *argv[];
{
  struct exec e;
  int c;
  u_long addr = 0, offset = 0;
  u_long replace = 0, do_replace = 0;
  char *symbol = 0;
  char size = 4;  /* default to long */
  char *fname;
  int fd;
  int type, off;
  u_long  lval;
  u_short sval;
  u_char  cval;
  

  while ((c = getopt (argc, argv, "a:bwlr:s:o:")) != EOF)
    switch (c)
      {
      case 'a':
	if (addr || symbol)
	  error ("only one address/symbol allowed");
	if (! strncmp (optarg, "0x", 2))
	  sscanf (optarg, "%x", &addr);
	else
	  addr = atoi (optarg);
	if (! addr)
	  error ("invalid address");
	break;

      case 'b':
	size = 1;
	break;

      case 'w':
	size = 2;
	break;

      case 'l':
	size = 4;
	break;

      case 'r':
	do_replace = 1;
	if (! strncmp (optarg, "0x", 2))
	  sscanf (optarg, "%x", &replace);
	else
	  replace = atoi (optarg);
	break;

      case 's':
	if (addr || symbol)
	  error ("only one address/symbol allowed");
	symbol = optarg;
	break;

      case 'o':
	if (offset)
	  error ("only one offset allowed");
	if (! strncmp (optarg, "0x", 2))
	  sscanf (optarg, "%x", &offset);
	else
          offset = atoi (optarg);
        break;
      }
  
  argv += optind;
  argc -= optind;


  if (argc < 1)
    error ("No file to patch.");

  fname = argv[0];
  if ((fd = open (fname, 0)) < 0)
    error ("Can't open file");

  if (read (fd, &e, sizeof (e)) != sizeof (e)
      || N_BADMAG (e))
    error ("Not a valid executable.");

  /* fake mid, so the N_ macros work on the amiga.. */
  e.a_midmag |= 127 << 16;

  if (symbol)
    {
      struct nlist nl[2];
      nl[0].n_un.n_name = symbol;
      nl[1].n_un.n_name = 0;
      if (nlist (fname, nl) != 0)
	error ("Symbol not found.");
      addr = nl[0].n_value;
      type = nl[0].n_type & N_TYPE;
    }
  else
    {
      type = N_UNDF;
      if (addr >= N_TXTADDR(e) && addr < N_DATADDR(e))
	type = N_TEXT;
      else if (addr >= N_DATADDR(e) && addr < N_DATADDR(e) + e.a_data)
	type = N_DATA;
    }
  addr += offset;

  /* if replace-mode, have to reopen the file for writing.
     Can't do that from the beginning, or nlist() will not 
     work (at least not under AmigaDOS) */
  if (do_replace)
    {
      close (fd);
      if ((fd = open (fname, 2)) == -1)
	error ("Can't reopen file for writing.");
    }

  if (type != N_TEXT && type != N_DATA)
    error ("address/symbol is not in text or data section.");

  if (type == N_TEXT)
    off = addr - N_TXTADDR(e) + N_TXTOFF(e);
  else
    off = addr - N_DATADDR(e) + N_DATOFF(e);

  if (lseek (fd, off, 0) == -1)
    error ("lseek");

  /* not beautiful, but works on big and little endian machines */
  switch (size)
    {
    case 1:
      if (read (fd, &cval, 1) != 1)
	error ("cread");
      lval = cval;
      break;

    case 2:
      if (read (fd, &sval, 2) != 2)
	error ("sread");
      lval = sval;
      break;

    case 4:
      if (read (fd, &lval, 4) != 4)
	error ("lread");
      break;
    }

  
  if (symbol)
    printf ("%s(0x%x): %d (0x%x)\n", symbol, addr, lval, lval);
  else
    printf ("0x%x: %d (0x%x)\n", addr, lval, lval);

  if (do_replace)
    {
      if (lseek (fd, off, 0) == -1)
	error ("write-lseek");
      switch (size)
	{
	case 1:
	  cval = replace;
	  if (cval != replace)
	    error ("byte-value overflow.");
	  if (write (fd, &cval, 1) != 1)
	    error ("cwrite");
	  break;

	case 2:
	  sval = replace;
	  if (sval != replace)
	    error ("word-value overflow.");
	  if (write (fd, &sval, 2) != 2)
	    error ("swrite");
	  break;

	case 4:
	  if (write (fd, &replace, 4) != 4)
	    error ("lwrite");
	  break;
	}
    }

  close (fd);
}



volatile void error (str)
     char *str;
{
  fprintf (stderr, "%s\n", str);
  exit (1);
}
