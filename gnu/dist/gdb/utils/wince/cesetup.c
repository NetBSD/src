/* cesetup.c - copy/edit/rename CE SDK files into GNU install area
   Copyright 1999 Free Software Foundation, Inc.

This file is part of GNU Binutils.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "libiberty.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>

static char *
strlwr (char *s)
{
  char *in_s = s;
  while (*s)
    {
      if (isupper (*s))
	*s = tolower (*s);
      s++;
    }
  return in_s;
}
int
copy_include (char *srcn, char *destn)
{
  FILE *src, *dest;
  char line[4096], *q;
  int w;

  src = fopen (srcn, "rb");
  if (!src)
    {
      perror (srcn);
      return 1;
    }
  dest = fopen (destn, "wb");
  if (!dest)
    {
      fclose (src);
      perror (destn);
      return 1;
    }

  while (fgets (line, 4096, src))
    {
      char *last = line + strlen (line);
      if (last[-1] == '\n' && last[-2] == '\r')
	{
	  last[-2] = '\n';
	  last[-1] = 0;
	}

      if (strstr(destn, "stdlib.h"))
	{
	  if (strstr (line, "typedef char *va_list"))
	    {
	      fprintf (dest, "#include <stdarg.h>\n");
	      continue;
	    }
	  if (strstr (line, "#define va_start"))
	    continue;
	  if (strstr (line, "#define va_arg"))
	    continue;
	  if (strstr (line, "#define va_end"))
	    continue;
	  if (strstr (line, "#define _INTSIZEOF"))
	    continue;
	  if (strstr (line, "memcmp("))
	    continue;
	  if (strstr (line, "memset("))
	    continue;
	  if (strstr (line, "memcpy("))
	    continue;
	  if (strstr (line, "matherr(struct"))
	    continue;
	}

      if (q = strstr (line, "[];"))
	{
	  *q = 0;
	  fprintf(dest, "%s[0%s", line, q+1);
	  *q = '[';
	  continue;
	}

      if (strstr (line, "_VARIANT_BOOL"))
	continue;

      if (strstr (line, "extern void __asm"))
	continue;

      if (strncmp (line, "#define DebugBreak () __asm (", 27) == 0)
	{
	  fprintf (dest, "#define DebugBreak () __asm__(%s", line+27);
	  continue;
	}

      w = fputs (line, dest);
      if (w < 0)
	{
	  fclose (src);
	  fclose (dest);
	  fprintf (stderr, "%s: out of disk space!\n", destn);
	  exit (1);
	}
    }
  fclose (src);
  fclose (dest);
  return 0;
}

int
copy_lib (char *srcn, char *destn)
{
  FILE *src, *dest;
  char buffer[4096];
  int r, w;

  src = fopen (srcn, "rb");
  if (!src)
    {
      perror (srcn);
      return 1;
    }
  dest = fopen (destn, "wb");
  if (!dest)
    {
      fclose (src);
      perror (destn);
      return 1;
    }
  while ((r = fread (buffer, 1, 4096, src)) > 0)
    {
      w = fwrite (buffer, 1, r, dest);
      if (w < r)
	{
	  fclose (src);
	  fclose (dest);
	  fprintf (stderr, "%s: out of disk space!\n", destn);
	  exit (1);
	}
    }
  fclose (src);
  fclose (dest);
  return 0;
}

int
main (int argc, char **argv)
{
  char *psdk_dir;
  char *gnu_dir;
  char line[1000];
  char line2[1000];
  int rv;
  struct stat statbuf;
  DIR *dir;
  struct dirent *de;
  int count;

  if (argc < 2)
    {
      printf ("Type in the location of the CE Platform SDK : ");
      fflush (stdout);
      fgets (line, 1000, stdin);
      while (line[0] && (line[strlen (line)-1] == '\r'
			 || line[strlen (line)-1] == '\n'))
	line[strlen (line)-1] = 0;
      psdk_dir = (char *)malloc (strlen (line)+1);
      strcpy (psdk_dir, line);
    }
  else
    psdk_dir = argv[1];

  sprintf (line, "%s/include/windows.h", psdk_dir);
  rv = stat (line, &statbuf);
  if (rv < 0)
    {
      printf ("Error: could not find %s - verify the PSDK dir.\n", line);
      exit (1);
    }

  if (argc < 3)
    {
      printf ("Type in the location of the GNU CE Tools installation : ");
      fflush (stdout);
      fgets (line, 1000, stdin);
      while (line[0] && (line[strlen (line)-1] == '\r'
			 || line[strlen (line)-1] == '\n'))
	line[strlen (line)-1] = 0;
      gnu_dir = (char *)malloc (strlen (line)+1);
      strcpy (gnu_dir, line);
    }
  else
    gnu_dir = argv[2];

  sprintf (line, "%s", gnu_dir);
  dir = opendir (line);
  if (!dir)
    {
      printf ("Error: could not find %s - verify the GNU dir.\n", line);
      exit (1);
    }

  while ((de = readdir (dir)) != 0)
    {
      char *platform;
      int rv;
      struct stat statbuf;
      DIR *idir;
      struct dirent *ide;

      if (strchr (de->d_name, '-') == 0)
	continue;

      sprintf (line, "%s/%s/include/.", gnu_dir, de->d_name);
      rv = stat (line, &statbuf);
      if (rv < 0)
	continue;

      if (strncasecmp (de->d_name, "sh", 2) == 0)
	platform = "sh3";
      else if (strncasecmp (de->d_name, "mips", 4) == 0)
	platform = "mips";
      else if (strncasecmp (de->d_name, "arm", 3) == 0)
	platform = "arm";
      else
	continue;

      printf ("Installing %s files into %s/%s\n", platform, gnu_dir, de->d_name);

      sprintf (line, "%s/include", psdk_dir);
      idir = opendir (line);
      if (!idir)
	{
	  printf ("Can't read %s\n", line);
	  exit (1);
	}
      count = 0;
      while ((ide = readdir (idir)) != 0)
	{
	  if (ide->d_name[0] == '.')
	    continue;
	  sprintf (line, "%s/include/%s", psdk_dir, ide->d_name);
	  sprintf (line2, "%s/%s/include/%s", gnu_dir, de->d_name, strlwr(ide->d_name));

	  copy_include (line, line2);
	  count ++;
	}
      printf ("%d headers converted and copied\n", count);

      sprintf (line, "%s/lib/%s", psdk_dir, platform);
      idir = opendir (line);
      if (!idir)
	{
	  printf ("Can't read %s\n", line);
	  exit (1);
	}
      count = 0;
      while ((ide = readdir (idir)) != 0)
	{
	  if (ide->d_name[0] == '.')
	    continue;

	  sprintf (line, "%s/lib/%s/%s", psdk_dir, platform, ide->d_name);

	  if (strcasecmp (ide->d_name + strlen (ide->d_name) - 4, ".lib") == 0)
	    strcpy (ide->d_name + strlen (ide->d_name) - 4, ".a");
	  if (strcasecmp (ide->d_name, "corelibc.a") == 0)
	    strcpy (ide->d_name, "c.a");

	  sprintf (line2, "%s/%s/lib/lib%s", gnu_dir, de->d_name, strlwr (ide->d_name));

	  copy_lib (line, line2);
	  count++;
	}
      printf ("%d libraries copied and renamed\n", count);
    }
  return 0;
}
