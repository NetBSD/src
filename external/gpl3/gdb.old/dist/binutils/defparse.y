%{ /* defparse.y - parser for .def files */

/* Copyright (C) 1995-2020 Free Software Foundation, Inc.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "bfd.h"
#include "libiberty.h"
#include "dlltool.h"
%}

%union {
  char *id;
  const char *id_const;
  int number;
};

%token NAME LIBRARY DESCRIPTION STACKSIZE HEAPSIZE CODE DATA
%token SECTIONS EXPORTS IMPORTS VERSIONK BASE CONSTANT
%token READ WRITE EXECUTE SHARED NONSHARED NONAME PRIVATE
%token SINGLE MULTIPLE INITINSTANCE INITGLOBAL TERMINSTANCE TERMGLOBAL
%token EQUAL
%token <id> ID
%token <number> NUMBER
%type  <number> opt_base opt_ordinal opt_NONAME opt_CONSTANT opt_DATA opt_PRIVATE
%type  <number> attr attr_list opt_number
%type  <id> opt_name opt_name2 opt_equal_name opt_import_name
%type  <id_const> keyword_as_name

%%

start: start command
	| command
	;

command:
		NAME opt_name opt_base { def_name ($2, $3); }
	|	LIBRARY opt_name opt_base option_list { def_library ($2, $3); }
	|	EXPORTS explist
	|	DESCRIPTION ID { def_description ($2);}
	|	STACKSIZE NUMBER opt_number { def_stacksize ($2, $3);}
	|	HEAPSIZE NUMBER opt_number { def_heapsize ($2, $3);}
	|	CODE attr_list { def_code ($2);}
	|	DATA attr_list  { def_data ($2);}
	|	SECTIONS seclist
	|	IMPORTS implist
	|	VERSIONK NUMBER { def_version ($2,0);}
	|	VERSIONK NUMBER '.' NUMBER { def_version ($2,$4);}
	;


explist:
		/* EMPTY */
	|	explist expline
	;

expline:
		ID opt_equal_name opt_ordinal opt_NONAME opt_CONSTANT opt_DATA opt_PRIVATE
		opt_import_name
			{ def_exports ($1, $2, $3, $4, $5, $6, $7, $8);}
	;
implist:
		implist impline
	|	impline
	;

impline:
               ID '=' ID '.' ID '.' ID opt_import_name
		 { def_import ($1,$3,$5,$7, 0, $8); }
       |       ID '=' ID '.' ID '.' NUMBER opt_import_name
		 { def_import ($1,$3,$5, 0,$7, $8); }
       |       ID '=' ID '.' ID opt_import_name
		 { def_import ($1,$3, 0,$5, 0, $6); }
       |       ID '=' ID '.' NUMBER opt_import_name
		 { def_import ($1,$3, 0, 0,$5, $6); }
       |       ID '.' ID '.' ID opt_import_name
		 { def_import ( 0,$1,$3,$5, 0, $6); }
       |       ID '.' ID '.' NUMBER opt_import_name
		 { def_import ( 0,$1,$3, 0,$5, $6); }
       |       ID '.' ID opt_import_name
		 { def_import ( 0,$1, 0,$3, 0, $4); }
       |       ID '.' NUMBER opt_import_name
		 { def_import ( 0,$1, 0, 0,$3, $4); }
;

seclist:
		seclist secline
	|	secline
	;

secline:
	ID attr_list { def_section ($1,$2);}
	;

attr_list:
	attr_list opt_comma attr
	| attr
	;

opt_comma:
	','
	|
	;
opt_number: ',' NUMBER { $$=$2;}
	|	   { $$=-1;}
	;

attr:
		READ { $$ = 1; }
	|	WRITE { $$ = 2; }
	|	EXECUTE { $$ = 4; }
	|	SHARED { $$ = 8; }
	|	NONSHARED { $$ = 0; }
	|	SINGLE { $$ = 0; }
	|	MULTIPLE { $$ = 0; }
	;

opt_CONSTANT:
		CONSTANT {$$=1;}
	|		 {$$=0;}
	;

opt_NONAME:
		NONAME {$$=1;}
	|		 {$$=0;}
	;

opt_DATA:
		DATA { $$ = 1; }
	|	     { $$ = 0; }
	;

opt_PRIVATE:
		PRIVATE { $$ = 1; }
	|		{ $$ = 0; }
	;

keyword_as_name: NAME { $$ = "NAME"; }
/*  Disabled LIBRARY keyword for a quirk in libtool. It places LIBRARY
    command after EXPORTS list, which is illegal by specification.
    See PR binutils/13710
	| LIBRARY { $$ = "LIBRARY"; } */
	| DESCRIPTION { $$ = "DESCRIPTION"; }
	| STACKSIZE { $$ = "STACKSIZE"; }
	| HEAPSIZE { $$ = "HEAPSIZE"; }
	| CODE { $$ = "CODE"; }
	| DATA { $$ = "DATA"; }
	| SECTIONS { $$ = "SECTIONS"; }
	| EXPORTS { $$ = "EXPORTS"; }
	| IMPORTS { $$ = "IMPORTS"; }
	| VERSIONK { $$ = "VERSION"; }
	| BASE { $$ = "BASE"; }
	| CONSTANT { $$ = "CONSTANT"; }
	| NONAME { $$ = "NONAME"; }
	| PRIVATE { $$ = "PRIVATE"; }
	| READ { $$ = "READ"; }
	| WRITE { $$ = "WRITE"; }
	| EXECUTE { $$ = "EXECUTE"; }
	| SHARED { $$ = "SHARED"; }
	| NONSHARED { $$ = "NONSHARED"; }
	| SINGLE { $$ = "SINGLE"; }
	| MULTIPLE { $$ = "MULTIPLE"; }
	| INITINSTANCE { $$ = "INITINSTANCE"; }
	| INITGLOBAL { $$ = "INITGLOBAL"; }
	| TERMINSTANCE { $$ = "TERMINSTANCE"; }
	| TERMGLOBAL { $$ = "TERMGLOBAL"; }
	;

opt_name2: ID { $$ = $1; }
	| '.' keyword_as_name
	  {
	    char *name = xmalloc (strlen ($2) + 2);
	    sprintf (name, ".%s", $2);
	    $$ = name;
	  }
	| '.' opt_name2
	  {
	    char *name = xmalloc (strlen ($2) + 2);
	    sprintf (name, ".%s", $2);
	    $$ = name;
	  }
	| keyword_as_name '.' opt_name2
	  {
	    char *name = xmalloc (strlen ($1) + 1 + strlen ($3) + 1);
	    sprintf (name, "%s.%s", $1, $3);
	    $$ = name;
	  }
	| ID '.' opt_name2
	  {
	    char *name = xmalloc (strlen ($1) + 1 + strlen ($3) + 1);
	    sprintf (name, "%s.%s", $1, $3);
	    $$ = name;
	  }
	;
opt_name: opt_name2 { $$ =$1; }
	|		{ $$=""; }
	;

opt_ordinal:
	  '@' NUMBER     { $$=$2;}
	|                { $$=-1;}
	;

opt_import_name:
	  EQUAL opt_name2	{ $$ = $2; }
	|		{ $$ = 0; }
	;

opt_equal_name:
          '=' opt_name2	{ $$ = $2; }
        | 		{ $$ =  0; }
	;

opt_base: BASE	'=' NUMBER	{ $$= $3;}
	|	{ $$=-1;}
	;

option_list:
		/* empty */
	|	option_list opt_comma option
	;

option:
		INITINSTANCE
	|	INITGLOBAL
	|	TERMINSTANCE
	|	TERMGLOBAL
	;
