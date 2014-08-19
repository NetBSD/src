/* Copyright 2004-2014 Free Software Foundation, Inc.
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
   Please email any bugs, comments, and/or additions to this file to:
   bug-gdb@gnu.org
 
   This file is part of the gdb testsuite.  */

#include <stdio.h>

sub1 ()
{
  printf ("In sub1\n");
}
  
sub2 ()
{
  printf ("In sub2\n");
}
  
main ()
{
  sub1 ();
  sub2 ();
}
