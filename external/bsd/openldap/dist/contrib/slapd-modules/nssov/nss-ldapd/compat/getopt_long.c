/*
   getopt_long.c - implementation of getopt_long() for systems that lack it

   Copyright (C) 2001, 2002, 2008 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "getopt_long.h"

/* this is a (poor) getopt_long() replacement for systems that don't have it
   (getopt_long() is generaly a GNU extention)
   this implementation is by no meens flawless, especialy the optional arguments
   to options and options following filenames is not quite right, allso
   minimal error checking is provided
   */
int getopt_long(int argc,char * const argv[],
                const char *optstring,
                const struct option *longopts,int *longindex)
{
  int i;   /* for looping through options */
  int l;   /* for length */

  /* first check if there realy is a -- option */
  if ( (optind>0)&&(optind<argc) &&
       (strncmp(argv[optind],"--",2)==0) &&
       (argv[optind][2]!='\0') )
  {
    /* check the longopts list for a valid option */
    for (i=0;longopts[i].name!=NULL;i++)
    {
      /* save the length for later */
      l=strlen(longopts[i].name);
      if (strncmp(argv[optind]+2,longopts[i].name,l)==0)
      {
        /* we have a match */
        if ( (longopts[i].has_arg==no_argument) &&
             (argv[optind][2+l]=='\0') )
        {
          optind++;
          return longopts[i].val;
        }
        else if ( (longopts[i].has_arg==required_argument) &&
                  (argv[optind][2+l]=='=') )
        {
          optarg=argv[optind]+3+l;
          optind++;
          return longopts[i].val;
        }
        else if ( (longopts[i].has_arg==required_argument) &&
                  (argv[optind][2+l]=='\0') )
        {
          optarg=argv[optind+1];
          optind+=2;
          return longopts[i].val;
        }
        else if ( (longopts[i].has_arg==optional_argument) &&
                  (argv[optind][2+l]=='=') )
        {
          optarg=argv[optind]+3+l;
          optind++;
          return longopts[i].val;
        }
        else if ( (longopts[i].has_arg==optional_argument) &&
                  (argv[optind][2+l]=='\0') )
        {
          optind++;
          return longopts[i].val;
        }
      }
    }
  }
  /* if all else fails use plain getopt() */
  return getopt(argc,argv,optstring);
}
