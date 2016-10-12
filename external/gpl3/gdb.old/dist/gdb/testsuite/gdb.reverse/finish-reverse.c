/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2015 Free Software Foundation, Inc.

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

/* Test gdb's "return" command in reverse.  */

int void_test = 0;
int main_test = 0;

char      char_returnval      = '1';
short     short_returnval     = 1;
int       int_returnval       = 1;
long      long_returnval      = 1;
long long long_long_returnval = 1;
float     float_returnval     = 1;
double    double_returnval    = 1;

union {
  char      char_testval;
  short     short_testval;
  int       int_testval;
  long      long_testval;
  long long long_long_testval;
  float     float_testval;
  double    double_testval;
  char      ffff[80];
} testval;

void void_func ()
{
  void_test = 1;		/* VOID FUNC */
}

char char_func ()
{
  return char_returnval;	/* CHAR FUNC */
}

short short_func ()
{
  return short_returnval;	/* SHORT FUNC */
}

int int_func ()
{
  return int_returnval;		/* INT FUNC */
}

long long_func ()
{
  return long_returnval;	/* LONG FUNC */
}

long long long_long_func ()
{
  return long_long_returnval;	/* LONG LONG FUNC */
}

float float_func ()
{
  return float_returnval;	/* FLOAT FUNC */
}

double double_func ()
{
  return double_returnval;	/* DOUBLE FUNC */
}

int main (int argc, char **argv)
{
  char char_resultval;
  short short_resultval;
  int int_resultval;
  long long_resultval;
  long long long_long_resultval;
  float float_resultval;
  double double_resultval;
  int i;

  /* A "test load" that will insure that the function really returns 
     a ${type} (as opposed to just a truncated or part of a ${type}).  */
  for (i = 0; i < sizeof (testval.ffff); i++)
    testval.ffff[i] = 0xff;

  void_func (); 				/* call to void_func */
  char_resultval      = char_func ();		/* void_checkpoint */
  short_resultval     = short_func ();		/* char_checkpoint */
  int_resultval       = int_func ();		/* short_checkpoint */
  long_resultval      = long_func ();		/* int_checkpoint */
  long_long_resultval = long_long_func ();	/* long_checkpoint */

  /* On machines using IEEE floating point, the test pattern of all
     1-bits established above turns out to be a floating-point NaN
     ("Not a Number").  According to the IEEE rules, NaN's aren't even
     equal to themselves.  This can lead to stupid conversations with
     GDB like:

       (gdb) p testval.float_testval == testval.float_testval
       $7 = 0
       (gdb)

     This is the correct answer, but it's not the sort of thing
     return2.exp wants to see.  So to make things work the way they
     ought, we'll set aside the `union' cleverness and initialize the
     test values explicitly here.  These values have interesting bits
     throughout the value, so we'll still detect truncated values.  */

  testval.float_testval = 2.7182818284590452354;/* long_long_checkpoint */
  float_resultval     = float_func ();		
  testval.double_testval = 3.14159265358979323846; /* float_checkpoint */
  double_resultval    = double_func ();		
  main_test = 1;				/* double_checkpoint */
  return 0;
} /* end of main */

