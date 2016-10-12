/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2015 Free Software Foundation, Inc.

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

#include "symcat.h"
#include "gdb/section-scripts.h"

/* Put the path to the pretty-printer script in .debug_gdb_scripts so
   gdb will automagically loaded it.
   Normally "MS" would appear here, as in
   .pushsection ".debug_gdb_scripts", "MS",@progbits,1
   but we remove it to test files appearing twice in the section.  */

#define DEFINE_GDB_SCRIPT(script_name) \
  asm("\
.pushsection \".debug_gdb_scripts\", \"S\",@progbits\n\
.byte " XSTRING (SECTION_SCRIPT_ID_SCHEME_FILE) "\n\
.asciz \"" script_name "\"\n\
.popsection \n\
");

#ifndef SCRIPT_FILE
#error "SCRIPT_FILE not defined"
#endif

/* Specify it twice to verify the file is only loaded once.  */
DEFINE_GDB_SCRIPT (SCRIPT_FILE)
DEFINE_GDB_SCRIPT (SCRIPT_FILE)

/* Inlined scripts are harder to create in the same way as
   DEFINE_GDB_SCRIPT_FILE.  Keep things simple and just define it here.
   Normally "MS" would appear here, as in
   .pushsection ".debug_gdb_scripts", "MS",@progbits,1
   but we remove it to test scripts appearing twice in the section.  */

#define DEFINE_GDB_SCRIPT_TEXT \
asm( \
".pushsection \".debug_gdb_scripts\", \"S\",@progbits\n" \
".byte " XSTRING (SECTION_SCRIPT_ID_SCHEME_TEXT) "\n" \
".ascii \"gdb.inlined-script\\n\"\n" \
".ascii \"(define test-cmd\\n\"\n" \
".ascii \" (make-command \\\"test-cmd\\\"\\n\"\n" \
".ascii \"  #:command-class COMMAND_OBSCURE\\n\"\n" \
".ascii \"  #:invoke (lambda (self arg from-tty)\\n\"\n" \
".ascii \"    (display (format #f \\\"test-cmd output, arg = ~a\\n\\\" arg)))))\\n\"\n" \
".ascii \"(register-command! test-cmd)\\n\"\n" \
".byte 0\n" \
".popsection\n" \
);

/* Specify it twice to verify the script is only executed once.  */
DEFINE_GDB_SCRIPT_TEXT
DEFINE_GDB_SCRIPT_TEXT

struct ss
{
  int a;
  int b;
};

void
init_ss (struct ss *s, int a, int b)
{
  s->a = a;
  s->b = b;
}

int
main ()
{
  struct ss ss;

  init_ss (&ss, 1, 2);

  return 0;      /* break to inspect struct and union */
}
