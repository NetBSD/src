// OBSOLETE /* Read os9/os9k symbol tables and convert to internal format, for GDB.
// OBSOLETE    Copyright 1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995,
// OBSOLETE    1996, 1997, 1998, 1999, 2000, 2001
// OBSOLETE    Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE /* This module provides three functions: os9k_symfile_init,
// OBSOLETE    which initializes to read a symbol file; os9k_new_init, which 
// OBSOLETE    discards existing cached information when all symbols are being
// OBSOLETE    discarded; and os9k_symfile_read, which reads a symbol table
// OBSOLETE    from a file.
// OBSOLETE 
// OBSOLETE    os9k_symfile_read only does the minimum work necessary for letting the
// OBSOLETE    user "name" things symbolically; it does not read the entire symtab.
// OBSOLETE    Instead, it reads the external and static symbols and puts them in partial
// OBSOLETE    symbol tables.  When more extensive information is requested of a
// OBSOLETE    file, the corresponding partial symbol table is mutated into a full
// OBSOLETE    fledged symbol table by going back and reading the symbols
// OBSOLETE    for real.  os9k_psymtab_to_symtab() is the function that does this */
// OBSOLETE 
// OBSOLETE #include "defs.h"
// OBSOLETE #include "gdb_string.h"
// OBSOLETE #include "gdb_assert.h"
// OBSOLETE #include <stdio.h>
// OBSOLETE 
// OBSOLETE #if defined(USG) || defined(__CYGNUSCLIB__)
// OBSOLETE #include <sys/types.h>
// OBSOLETE #include <fcntl.h>
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #include "obstack.h"
// OBSOLETE #include "gdb_stat.h"
// OBSOLETE #include "symtab.h"
// OBSOLETE #include "breakpoint.h"
// OBSOLETE #include "command.h"
// OBSOLETE #include "target.h"
// OBSOLETE #include "gdbcore.h"		/* for bfd stuff */
// OBSOLETE #include "libaout.h"		/* FIXME Secret internal BFD stuff for a.out */
// OBSOLETE #include "symfile.h"
// OBSOLETE #include "objfiles.h"
// OBSOLETE #include "buildsym.h"
// OBSOLETE #include "gdb-stabs.h"
// OBSOLETE #include "demangle.h"
// OBSOLETE #include "language.h"		/* Needed inside partial-stab.h */
// OBSOLETE #include "complaints.h"
// OBSOLETE #include "os9k.h"
// OBSOLETE #include "stabsread.h"
// OBSOLETE 
// OBSOLETE extern void _initialize_os9kread (void);
// OBSOLETE 
// OBSOLETE /* Each partial symbol table entry contains a pointer to private data for the
// OBSOLETE    read_symtab() function to use when expanding a partial symbol table entry
// OBSOLETE    to a full symbol table entry.
// OBSOLETE 
// OBSOLETE    For dbxread this structure contains the offset within the file symbol table
// OBSOLETE    of first local symbol for this file, and count of the section
// OBSOLETE    of the symbol table devoted to this file's symbols (actually, the section
// OBSOLETE    bracketed may contain more than just this file's symbols).  It also contains
// OBSOLETE    further information needed to locate the symbols if they are in an ELF file.
// OBSOLETE 
// OBSOLETE    If ldsymcnt is 0, the only reason for this thing's existence is the
// OBSOLETE    dependency list.  Nothing else will happen when it is read in.  */
// OBSOLETE 
// OBSOLETE #define LDSYMOFF(p) (((struct symloc *)((p)->read_symtab_private))->ldsymoff)
// OBSOLETE #define LDSYMCNT(p) (((struct symloc *)((p)->read_symtab_private))->ldsymnum)
// OBSOLETE 
// OBSOLETE struct symloc
// OBSOLETE   {
// OBSOLETE     int ldsymoff;
// OBSOLETE     int ldsymnum;
// OBSOLETE   };
// OBSOLETE 
// OBSOLETE /* Remember what we deduced to be the source language of this psymtab. */
// OBSOLETE static enum language psymtab_language = language_unknown;
// OBSOLETE 
// OBSOLETE /* keep partial symbol table file nested depth */
// OBSOLETE static int psymfile_depth = 0;
// OBSOLETE 
// OBSOLETE /* keep symbol table file nested depth */
// OBSOLETE static int symfile_depth = 0;
// OBSOLETE 
// OBSOLETE extern int previous_stab_code;
// OBSOLETE 
// OBSOLETE /* Name of last function encountered.  Used in Solaris to approximate
// OBSOLETE    object file boundaries.  */
// OBSOLETE static char *last_function_name;
// OBSOLETE 
// OBSOLETE /* Complaints about the symbols we have encountered.  */
// OBSOLETE extern struct complaint lbrac_complaint;
// OBSOLETE 
// OBSOLETE extern struct complaint unknown_symtype_complaint;
// OBSOLETE 
// OBSOLETE extern struct complaint unknown_symchar_complaint;
// OBSOLETE 
// OBSOLETE extern struct complaint lbrac_rbrac_complaint;
// OBSOLETE 
// OBSOLETE extern struct complaint repeated_header_complaint;
// OBSOLETE 
// OBSOLETE extern struct complaint repeated_header_name_complaint;
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE static struct complaint lbrac_unmatched_complaint =
// OBSOLETE {"unmatched Increment Block Entry before symtab pos %d", 0, 0};
// OBSOLETE 
// OBSOLETE static struct complaint lbrac_mismatch_complaint =
// OBSOLETE {"IBE/IDE symbol mismatch at symtab pos %d", 0, 0};
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE /* Local function prototypes */
// OBSOLETE 
// OBSOLETE static void read_minimal_symbols (struct objfile *);
// OBSOLETE 
// OBSOLETE static void os9k_read_ofile_symtab (struct partial_symtab *);
// OBSOLETE 
// OBSOLETE static void os9k_psymtab_to_symtab (struct partial_symtab *);
// OBSOLETE 
// OBSOLETE static void os9k_psymtab_to_symtab_1 (struct partial_symtab *);
// OBSOLETE 
// OBSOLETE static void read_os9k_psymtab (struct objfile *, CORE_ADDR, int);
// OBSOLETE 
// OBSOLETE static int fill_sym (FILE *, bfd *);
// OBSOLETE 
// OBSOLETE static void os9k_symfile_init (struct objfile *);
// OBSOLETE 
// OBSOLETE static void os9k_new_init (struct objfile *);
// OBSOLETE 
// OBSOLETE static void os9k_symfile_read (struct objfile *, int);
// OBSOLETE 
// OBSOLETE static void os9k_symfile_finish (struct objfile *);
// OBSOLETE 
// OBSOLETE static void os9k_process_one_symbol (int, int, CORE_ADDR, char *,
// OBSOLETE 				     struct section_offsets *,
// OBSOLETE 				     struct objfile *);
// OBSOLETE 
// OBSOLETE static struct partial_symtab *os9k_start_psymtab (struct objfile *, char *,
// OBSOLETE 						  CORE_ADDR, int, int,
// OBSOLETE 						  struct partial_symbol **,
// OBSOLETE 						  struct partial_symbol **);
// OBSOLETE 
// OBSOLETE static struct partial_symtab *os9k_end_psymtab (struct partial_symtab *,
// OBSOLETE 						char **, int, int, CORE_ADDR,
// OBSOLETE 						struct partial_symtab **,
// OBSOLETE 						int);
// OBSOLETE 
// OBSOLETE static void record_minimal_symbol (char *, CORE_ADDR, int, struct objfile *);
// OBSOLETE 
// OBSOLETE #define HANDLE_RBRAC(val) \
// OBSOLETE   if ((val) > pst->texthigh) pst->texthigh = (val);
// OBSOLETE 
// OBSOLETE #define SWAP_STBHDR(hdrp, abfd) \
// OBSOLETE   { \
// OBSOLETE     (hdrp)->fmtno = bfd_get_16(abfd, (unsigned char *)&(hdrp)->fmtno); \
// OBSOLETE     (hdrp)->crc = bfd_get_32(abfd, (unsigned char *)&(hdrp)->crc); \
// OBSOLETE     (hdrp)->offset = bfd_get_32(abfd, (unsigned char *)&(hdrp)->offset); \
// OBSOLETE     (hdrp)->nsym = bfd_get_32(abfd, (unsigned char *)&(hdrp)->nsym); \
// OBSOLETE   }
// OBSOLETE #define SWAP_STBSYM(symp, abfd) \
// OBSOLETE   { \
// OBSOLETE     (symp)->value = bfd_get_32(abfd, (unsigned char *)&(symp)->value); \
// OBSOLETE     (symp)->type = bfd_get_16(abfd, (unsigned char *)&(symp)->type); \
// OBSOLETE     (symp)->stroff = bfd_get_32(abfd, (unsigned char *)&(symp)->stroff); \
// OBSOLETE   }
// OBSOLETE #define N_DATA 0
// OBSOLETE #define N_BSS 1
// OBSOLETE #define N_RDATA 2
// OBSOLETE #define N_IDATA 3
// OBSOLETE #define N_TEXT 4
// OBSOLETE #define N_ABS 6
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE record_minimal_symbol (char *name, CORE_ADDR address, int type,
// OBSOLETE 		       struct objfile *objfile)
// OBSOLETE {
// OBSOLETE   enum minimal_symbol_type ms_type;
// OBSOLETE 
// OBSOLETE   switch (type)
// OBSOLETE     {
// OBSOLETE     case N_TEXT:
// OBSOLETE       ms_type = mst_text;
// OBSOLETE       address += ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE       break;
// OBSOLETE     case N_DATA:
// OBSOLETE       ms_type = mst_data;
// OBSOLETE       break;
// OBSOLETE     case N_BSS:
// OBSOLETE       ms_type = mst_bss;
// OBSOLETE       break;
// OBSOLETE     case N_RDATA:
// OBSOLETE       ms_type = mst_bss;
// OBSOLETE       break;
// OBSOLETE     case N_IDATA:
// OBSOLETE       ms_type = mst_data;
// OBSOLETE       break;
// OBSOLETE     case N_ABS:
// OBSOLETE       ms_type = mst_abs;
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE       ms_type = mst_unknown;
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   prim_record_minimal_symbol (name, address, ms_type, objfile);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* read and process .stb file and store in minimal symbol table */
// OBSOLETE typedef char mhhdr[80];
// OBSOLETE struct stbhdr
// OBSOLETE   {
// OBSOLETE     mhhdr comhdr;
// OBSOLETE     char *name;
// OBSOLETE     short fmtno;
// OBSOLETE     int crc;
// OBSOLETE     int offset;
// OBSOLETE     int nsym;
// OBSOLETE     char *pad;
// OBSOLETE   };
// OBSOLETE struct stbsymbol
// OBSOLETE   {
// OBSOLETE     int value;
// OBSOLETE     short type;
// OBSOLETE     int stroff;
// OBSOLETE   };
// OBSOLETE #define STBSYMSIZE 10
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE read_minimal_symbols (struct objfile *objfile)
// OBSOLETE {
// OBSOLETE   FILE *fp;
// OBSOLETE   bfd *abfd;
// OBSOLETE   struct stbhdr hdr;
// OBSOLETE   struct stbsymbol sym;
// OBSOLETE   int ch, i, j, off;
// OBSOLETE   char buf[64], buf1[128];
// OBSOLETE 
// OBSOLETE   fp = objfile->auxf1;
// OBSOLETE   if (fp == NULL)
// OBSOLETE     return;
// OBSOLETE   abfd = objfile->obfd;
// OBSOLETE   fread (&hdr.comhdr[0], sizeof (mhhdr), 1, fp);
// OBSOLETE   i = 0;
// OBSOLETE   ch = getc (fp);
// OBSOLETE   while (ch != -1)
// OBSOLETE     {
// OBSOLETE       buf[i] = (char) ch;
// OBSOLETE       i++;
// OBSOLETE       if (ch == 0)
// OBSOLETE 	break;
// OBSOLETE       ch = getc (fp);
// OBSOLETE     };
// OBSOLETE   if (i % 2)
// OBSOLETE     ch = getc (fp);
// OBSOLETE   hdr.name = &buf[0];
// OBSOLETE 
// OBSOLETE   fread (&hdr.fmtno, sizeof (hdr.fmtno), 1, fp);
// OBSOLETE   fread (&hdr.crc, sizeof (hdr.crc), 1, fp);
// OBSOLETE   fread (&hdr.offset, sizeof (hdr.offset), 1, fp);
// OBSOLETE   fread (&hdr.nsym, sizeof (hdr.nsym), 1, fp);
// OBSOLETE   SWAP_STBHDR (&hdr, abfd);
// OBSOLETE 
// OBSOLETE   /* read symbols */
// OBSOLETE   init_minimal_symbol_collection ();
// OBSOLETE   off = hdr.offset;
// OBSOLETE   for (i = hdr.nsym; i > 0; i--)
// OBSOLETE     {
// OBSOLETE       fseek (fp, (long) off, 0);
// OBSOLETE       fread (&sym.value, sizeof (sym.value), 1, fp);
// OBSOLETE       fread (&sym.type, sizeof (sym.type), 1, fp);
// OBSOLETE       fread (&sym.stroff, sizeof (sym.stroff), 1, fp);
// OBSOLETE       SWAP_STBSYM (&sym, abfd);
// OBSOLETE       fseek (fp, (long) sym.stroff, 0);
// OBSOLETE       j = 0;
// OBSOLETE       ch = getc (fp);
// OBSOLETE       while (ch != -1)
// OBSOLETE 	{
// OBSOLETE 	  buf1[j] = (char) ch;
// OBSOLETE 	  j++;
// OBSOLETE 	  if (ch == 0)
// OBSOLETE 	    break;
// OBSOLETE 	  ch = getc (fp);
// OBSOLETE 	};
// OBSOLETE       record_minimal_symbol (buf1, sym.value, sym.type & 7, objfile);
// OBSOLETE       off += STBSYMSIZE;
// OBSOLETE     };
// OBSOLETE   install_minimal_symbols (objfile);
// OBSOLETE   return;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Scan and build partial symbols for a symbol file.
// OBSOLETE    We have been initialized by a call to os9k_symfile_init, which 
// OBSOLETE    put all the relevant info into a "struct os9k_symfile_info",
// OBSOLETE    hung off the objfile structure.
// OBSOLETE 
// OBSOLETE    MAINLINE is true if we are reading the main symbol
// OBSOLETE    table (as opposed to a shared lib or dynamically loaded file).  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE os9k_symfile_read (struct objfile *objfile, int mainline)
// OBSOLETE {
// OBSOLETE   bfd *sym_bfd;
// OBSOLETE   struct cleanup *back_to;
// OBSOLETE 
// OBSOLETE   sym_bfd = objfile->obfd;
// OBSOLETE   /* If we are reinitializing, or if we have never loaded syms yet, init */
// OBSOLETE   if (mainline
// OBSOLETE       || (objfile->global_psymbols.size == 0
// OBSOLETE 	  && objfile->static_psymbols.size == 0))
// OBSOLETE     init_psymbol_list (objfile, DBX_SYMCOUNT (objfile));
// OBSOLETE 
// OBSOLETE   free_pending_blocks ();
// OBSOLETE   back_to = make_cleanup (really_free_pendings, 0);
// OBSOLETE 
// OBSOLETE   make_cleanup_discard_minimal_symbols ();
// OBSOLETE   read_minimal_symbols (objfile);
// OBSOLETE 
// OBSOLETE   /* Now that the symbol table data of the executable file are all in core,
// OBSOLETE      process them and define symbols accordingly.  */
// OBSOLETE   read_os9k_psymtab (objfile,
// OBSOLETE 		     DBX_TEXT_ADDR (objfile),
// OBSOLETE 		     DBX_TEXT_SIZE (objfile));
// OBSOLETE 
// OBSOLETE   do_cleanups (back_to);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Initialize anything that needs initializing when a completely new
// OBSOLETE    symbol file is specified (not just adding some symbols from another
// OBSOLETE    file, e.g. a shared library).  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE os9k_new_init (struct objfile *ignore)
// OBSOLETE {
// OBSOLETE   stabsread_new_init ();
// OBSOLETE   buildsym_new_init ();
// OBSOLETE   psymfile_depth = 0;
// OBSOLETE /*
// OBSOLETE    init_header_files ();
// OBSOLETE  */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* os9k_symfile_init ()
// OBSOLETE    It is passed a struct objfile which contains, among other things,
// OBSOLETE    the BFD for the file whose symbols are being read, and a slot for a pointer
// OBSOLETE    to "private data" which we fill with goodies.
// OBSOLETE 
// OBSOLETE    Since BFD doesn't know how to read debug symbols in a format-independent
// OBSOLETE    way (and may never do so...), we have to do it ourselves.  We will never
// OBSOLETE    be called unless this is an a.out (or very similar) file. 
// OBSOLETE    FIXME, there should be a cleaner peephole into the BFD environment here.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE os9k_symfile_init (struct objfile *objfile)
// OBSOLETE {
// OBSOLETE   bfd *sym_bfd = objfile->obfd;
// OBSOLETE   char *name = bfd_get_filename (sym_bfd);
// OBSOLETE   char dbgname[512], stbname[512];
// OBSOLETE   FILE *symfile = 0;
// OBSOLETE   FILE *minfile = 0;
// OBSOLETE   asection *text_sect;
// OBSOLETE 
// OBSOLETE   strcpy (dbgname, name);
// OBSOLETE   strcat (dbgname, ".dbg");
// OBSOLETE   strcpy (stbname, name);
// OBSOLETE   strcat (stbname, ".stb");
// OBSOLETE 
// OBSOLETE   if ((symfile = fopen (dbgname, "r")) == NULL)
// OBSOLETE     {
// OBSOLETE       warning ("Symbol file %s not found", dbgname);
// OBSOLETE     }
// OBSOLETE   objfile->auxf2 = symfile;
// OBSOLETE 
// OBSOLETE   if ((minfile = fopen (stbname, "r")) == NULL)
// OBSOLETE     {
// OBSOLETE       warning ("Symbol file %s not found", stbname);
// OBSOLETE     }
// OBSOLETE   objfile->auxf1 = minfile;
// OBSOLETE 
// OBSOLETE   /* Allocate struct to keep track of the symfile */
// OBSOLETE   objfile->sym_stab_info = (struct dbx_symfile_info *)
// OBSOLETE     xmmalloc (objfile->md, sizeof (struct dbx_symfile_info));
// OBSOLETE   DBX_SYMFILE_INFO (objfile)->stab_section_info = NULL;
// OBSOLETE 
// OBSOLETE   text_sect = bfd_get_section_by_name (sym_bfd, ".text");
// OBSOLETE   if (!text_sect)
// OBSOLETE     error ("Can't find .text section in file");
// OBSOLETE   DBX_TEXT_ADDR (objfile) = bfd_section_vma (sym_bfd, text_sect);
// OBSOLETE   DBX_TEXT_SIZE (objfile) = bfd_section_size (sym_bfd, text_sect);
// OBSOLETE 
// OBSOLETE   DBX_SYMBOL_SIZE (objfile) = 0;	/* variable size symbol */
// OBSOLETE   DBX_SYMCOUNT (objfile) = 0;	/* used to be bfd_get_symcount(sym_bfd) */
// OBSOLETE   DBX_SYMTAB_OFFSET (objfile) = 0;	/* used to be SYMBOL_TABLE_OFFSET */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Perform any local cleanups required when we are done with a particular
// OBSOLETE    objfile.  I.E, we are in the process of discarding all symbol information
// OBSOLETE    for an objfile, freeing up all memory held for it, and unlinking the
// OBSOLETE    objfile struct from the global list of known objfiles. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE os9k_symfile_finish (struct objfile *objfile)
// OBSOLETE {
// OBSOLETE   if (objfile->sym_stab_info != NULL)
// OBSOLETE     {
// OBSOLETE       xmfree (objfile->md, objfile->sym_stab_info);
// OBSOLETE     }
// OBSOLETE /*
// OBSOLETE    free_header_files ();
// OBSOLETE  */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE struct st_dbghdr
// OBSOLETE {
// OBSOLETE   int sync;
// OBSOLETE   short rev;
// OBSOLETE   int crc;
// OBSOLETE   short os;
// OBSOLETE   short cpu;
// OBSOLETE };
// OBSOLETE #define SYNC 		(int)0xefbefeca
// OBSOLETE 
// OBSOLETE #define SWAP_DBGHDR(hdrp, abfd) \
// OBSOLETE   { \
// OBSOLETE     (hdrp)->sync = bfd_get_32(abfd, (unsigned char *)&(hdrp)->sync); \
// OBSOLETE     (hdrp)->rev = bfd_get_16(abfd, (unsigned char *)&(hdrp)->rev); \
// OBSOLETE     (hdrp)->crc = bfd_get_32(abfd, (unsigned char *)&(hdrp)->crc); \
// OBSOLETE     (hdrp)->os = bfd_get_16(abfd, (unsigned char *)&(hdrp)->os); \
// OBSOLETE     (hdrp)->cpu = bfd_get_16(abfd, (unsigned char *)&(hdrp)->cpu); \
// OBSOLETE   }
// OBSOLETE 
// OBSOLETE #define N_SYM_CMPLR     0
// OBSOLETE #define N_SYM_SLINE     1
// OBSOLETE #define N_SYM_SYM       2
// OBSOLETE #define N_SYM_LBRAC     3
// OBSOLETE #define N_SYM_RBRAC     4
// OBSOLETE #define N_SYM_SE        5
// OBSOLETE 
// OBSOLETE struct internal_symstruct
// OBSOLETE   {
// OBSOLETE     short n_type;
// OBSOLETE     short n_desc;
// OBSOLETE     long n_value;
// OBSOLETE     char *n_strx;
// OBSOLETE   };
// OBSOLETE static struct internal_symstruct symbol;
// OBSOLETE static struct internal_symstruct *symbuf = &symbol;
// OBSOLETE static char strbuf[4096];
// OBSOLETE static struct st_dbghdr dbghdr;
// OBSOLETE static short cmplrid;
// OBSOLETE 
// OBSOLETE #define VER_PRE_ULTRAC	((short)4)
// OBSOLETE #define VER_ULTRAC	((short)5)
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE fill_sym (FILE *dbg_file, bfd *abfd)
// OBSOLETE {
// OBSOLETE   short si, nmask;
// OBSOLETE   long li;
// OBSOLETE   int ii;
// OBSOLETE   char *p;
// OBSOLETE 
// OBSOLETE   int nbytes = fread (&si, sizeof (si), 1, dbg_file);
// OBSOLETE   if (nbytes == 0)
// OBSOLETE     return 0;
// OBSOLETE   if (nbytes < 0)
// OBSOLETE     perror_with_name ("reading .dbg file.");
// OBSOLETE   symbuf->n_desc = 0;
// OBSOLETE   symbuf->n_value = 0;
// OBSOLETE   symbuf->n_strx = NULL;
// OBSOLETE   symbuf->n_type = bfd_get_16 (abfd, (unsigned char *) &si);
// OBSOLETE   symbuf->n_type = 0xf & symbuf->n_type;
// OBSOLETE   switch (symbuf->n_type)
// OBSOLETE     {
// OBSOLETE     case N_SYM_CMPLR:
// OBSOLETE       fread (&si, sizeof (si), 1, dbg_file);
// OBSOLETE       symbuf->n_desc = bfd_get_16 (abfd, (unsigned char *) &si);
// OBSOLETE       cmplrid = symbuf->n_desc & 0xff;
// OBSOLETE       break;
// OBSOLETE     case N_SYM_SLINE:
// OBSOLETE       fread (&li, sizeof (li), 1, dbg_file);
// OBSOLETE       symbuf->n_value = bfd_get_32 (abfd, (unsigned char *) &li);
// OBSOLETE       fread (&li, sizeof (li), 1, dbg_file);
// OBSOLETE       li = bfd_get_32 (abfd, (unsigned char *) &li);
// OBSOLETE       symbuf->n_strx = (char *) (li >> 12);
// OBSOLETE       symbuf->n_desc = li & 0xfff;
// OBSOLETE       break;
// OBSOLETE     case N_SYM_SYM:
// OBSOLETE       fread (&li, sizeof (li), 1, dbg_file);
// OBSOLETE       symbuf->n_value = bfd_get_32 (abfd, (unsigned char *) &li);
// OBSOLETE       si = 0;
// OBSOLETE       do
// OBSOLETE 	{
// OBSOLETE 	  ii = getc (dbg_file);
// OBSOLETE 	  strbuf[si++] = (char) ii;
// OBSOLETE 	}
// OBSOLETE       while (ii != 0 || si % 2 != 0);
// OBSOLETE       symbuf->n_strx = strbuf;
// OBSOLETE       p = (char *) strchr (strbuf, ':');
// OBSOLETE       if (!p)
// OBSOLETE 	break;
// OBSOLETE       if ((p[1] == 'F' || p[1] == 'f') && cmplrid == VER_PRE_ULTRAC)
// OBSOLETE 	{
// OBSOLETE 	  fread (&si, sizeof (si), 1, dbg_file);
// OBSOLETE 	  nmask = bfd_get_16 (abfd, (unsigned char *) &si);
// OBSOLETE 	  for (ii = 0; ii < nmask; ii++)
// OBSOLETE 	    fread (&si, sizeof (si), 1, dbg_file);
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE     case N_SYM_LBRAC:
// OBSOLETE       fread (&li, sizeof (li), 1, dbg_file);
// OBSOLETE       symbuf->n_value = bfd_get_32 (abfd, (unsigned char *) &li);
// OBSOLETE       break;
// OBSOLETE     case N_SYM_RBRAC:
// OBSOLETE       fread (&li, sizeof (li), 1, dbg_file);
// OBSOLETE       symbuf->n_value = bfd_get_32 (abfd, (unsigned char *) &li);
// OBSOLETE       break;
// OBSOLETE     case N_SYM_SE:
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Given pointers to an a.out symbol table in core containing dbx
// OBSOLETE    style data, setup partial_symtab's describing each source file for
// OBSOLETE    which debugging information is available.
// OBSOLETE    SYMFILE_NAME is the name of the file we are reading from. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE read_os9k_psymtab (struct objfile *objfile, CORE_ADDR text_addr, int text_size)
// OBSOLETE {
// OBSOLETE   register struct internal_symstruct *bufp = 0;		/* =0 avoids gcc -Wall glitch */
// OBSOLETE   register char *namestring;
// OBSOLETE   int past_first_source_file = 0;
// OBSOLETE   CORE_ADDR last_o_file_start = 0;
// OBSOLETE #if 0
// OBSOLETE   struct cleanup *back_to;
// OBSOLETE #endif
// OBSOLETE   bfd *abfd;
// OBSOLETE   FILE *fp;
// OBSOLETE 
// OBSOLETE   /* End of the text segment of the executable file.  */
// OBSOLETE   static CORE_ADDR end_of_text_addr;
// OBSOLETE 
// OBSOLETE   /* Current partial symtab */
// OBSOLETE   static struct partial_symtab *pst = 0;
// OBSOLETE 
// OBSOLETE   /* List of current psymtab's include files */
// OBSOLETE   char **psymtab_include_list;
// OBSOLETE   int includes_allocated;
// OBSOLETE   int includes_used;
// OBSOLETE 
// OBSOLETE   /* Index within current psymtab dependency list */
// OBSOLETE   struct partial_symtab **dependency_list;
// OBSOLETE   int dependencies_used, dependencies_allocated;
// OBSOLETE 
// OBSOLETE   includes_allocated = 30;
// OBSOLETE   includes_used = 0;
// OBSOLETE   psymtab_include_list = (char **) alloca (includes_allocated *
// OBSOLETE 					   sizeof (char *));
// OBSOLETE 
// OBSOLETE   dependencies_allocated = 30;
// OBSOLETE   dependencies_used = 0;
// OBSOLETE   dependency_list =
// OBSOLETE     (struct partial_symtab **) alloca (dependencies_allocated *
// OBSOLETE 				       sizeof (struct partial_symtab *));
// OBSOLETE 
// OBSOLETE   last_source_file = NULL;
// OBSOLETE 
// OBSOLETE #ifdef END_OF_TEXT_DEFAULT
// OBSOLETE   end_of_text_addr = END_OF_TEXT_DEFAULT;
// OBSOLETE #else
// OBSOLETE   end_of_text_addr = text_addr + ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile))
// OBSOLETE     + text_size;		/* Relocate */
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   abfd = objfile->obfd;
// OBSOLETE   fp = objfile->auxf2;
// OBSOLETE   if (!fp)
// OBSOLETE     return;
// OBSOLETE 
// OBSOLETE   fread (&dbghdr.sync, sizeof (dbghdr.sync), 1, fp);
// OBSOLETE   fread (&dbghdr.rev, sizeof (dbghdr.rev), 1, fp);
// OBSOLETE   fread (&dbghdr.crc, sizeof (dbghdr.crc), 1, fp);
// OBSOLETE   fread (&dbghdr.os, sizeof (dbghdr.os), 1, fp);
// OBSOLETE   fread (&dbghdr.cpu, sizeof (dbghdr.cpu), 1, fp);
// OBSOLETE   SWAP_DBGHDR (&dbghdr, abfd);
// OBSOLETE 
// OBSOLETE   symnum = 0;
// OBSOLETE   while (1)
// OBSOLETE     {
// OBSOLETE       int ret;
// OBSOLETE       long cursymoffset;
// OBSOLETE 
// OBSOLETE       /* Get the symbol for this run and pull out some info */
// OBSOLETE       QUIT;			/* allow this to be interruptable */
// OBSOLETE       cursymoffset = ftell (objfile->auxf2);
// OBSOLETE       ret = fill_sym (objfile->auxf2, abfd);
// OBSOLETE       if (ret <= 0)
// OBSOLETE 	break;
// OBSOLETE       else
// OBSOLETE 	symnum++;
// OBSOLETE       bufp = symbuf;
// OBSOLETE 
// OBSOLETE       /* Special case to speed up readin. */
// OBSOLETE       if (bufp->n_type == (short) N_SYM_SLINE)
// OBSOLETE 	continue;
// OBSOLETE 
// OBSOLETE #define CUR_SYMBOL_VALUE bufp->n_value
// OBSOLETE       /* partial-stab.h */
// OBSOLETE 
// OBSOLETE       switch (bufp->n_type)
// OBSOLETE 	{
// OBSOLETE 	  char *p;
// OBSOLETE 
// OBSOLETE 	case N_SYM_CMPLR:
// OBSOLETE 	  continue;
// OBSOLETE 
// OBSOLETE 	case N_SYM_SE:
// OBSOLETE 	  CUR_SYMBOL_VALUE += ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE 	  if (psymfile_depth == 1 && pst)
// OBSOLETE 	    {
// OBSOLETE 	      os9k_end_psymtab (pst, psymtab_include_list, includes_used,
// OBSOLETE 				symnum, CUR_SYMBOL_VALUE,
// OBSOLETE 				dependency_list, dependencies_used);
// OBSOLETE 	      pst = (struct partial_symtab *) 0;
// OBSOLETE 	      includes_used = 0;
// OBSOLETE 	      dependencies_used = 0;
// OBSOLETE 	    }
// OBSOLETE 	  psymfile_depth--;
// OBSOLETE 	  continue;
// OBSOLETE 
// OBSOLETE 	case N_SYM_SYM:	/* Typedef or automatic variable. */
// OBSOLETE 	  namestring = bufp->n_strx;
// OBSOLETE 	  p = (char *) strchr (namestring, ':');
// OBSOLETE 	  if (!p)
// OBSOLETE 	    continue;		/* Not a debugging symbol.   */
// OBSOLETE 
// OBSOLETE 	  /* Main processing section for debugging symbols which
// OBSOLETE 	     the initial read through the symbol tables needs to worry
// OBSOLETE 	     about.  If we reach this point, the symbol which we are
// OBSOLETE 	     considering is definitely one we are interested in.
// OBSOLETE 	     p must also contain the (valid) index into the namestring
// OBSOLETE 	     which indicates the debugging type symbol.  */
// OBSOLETE 
// OBSOLETE 	  switch (p[1])
// OBSOLETE 	    {
// OBSOLETE 	    case 'S':
// OBSOLETE 	      {
// OBSOLETE 		unsigned long valu;
// OBSOLETE 		enum language tmp_language;
// OBSOLETE 		char *str, *p;
// OBSOLETE 		int n;
// OBSOLETE 
// OBSOLETE 		valu = CUR_SYMBOL_VALUE;
// OBSOLETE 		if (valu)
// OBSOLETE 		  valu += ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE 		past_first_source_file = 1;
// OBSOLETE 
// OBSOLETE 		p = strchr (namestring, ':');
// OBSOLETE 		if (p)
// OBSOLETE 		  n = p - namestring;
// OBSOLETE 		else
// OBSOLETE 		  n = strlen (namestring);
// OBSOLETE 		str = alloca (n + 1);
// OBSOLETE 		strncpy (str, namestring, n);
// OBSOLETE 		str[n] = '\0';
// OBSOLETE 
// OBSOLETE 		if (psymfile_depth == 0)
// OBSOLETE 		  {
// OBSOLETE 		    if (!pst)
// OBSOLETE 		      pst = os9k_start_psymtab (objfile,
// OBSOLETE 						str, valu,
// OBSOLETE 						cursymoffset,
// OBSOLETE 						symnum - 1,
// OBSOLETE 					      objfile->global_psymbols.next,
// OBSOLETE 					     objfile->static_psymbols.next);
// OBSOLETE 		  }
// OBSOLETE 		else
// OBSOLETE 		  {		/* this is a include file */
// OBSOLETE 		    tmp_language = deduce_language_from_filename (str);
// OBSOLETE 		    if (tmp_language != language_unknown
// OBSOLETE 			&& (tmp_language != language_c
// OBSOLETE 			    || psymtab_language != language_cplus))
// OBSOLETE 		      psymtab_language = tmp_language;
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    if (pst && STREQ (str, pst->filename))
// OBSOLETE    continue;
// OBSOLETE    {
// OBSOLETE    register int i;
// OBSOLETE    for (i = 0; i < includes_used; i++)
// OBSOLETE    if (STREQ (str, psymtab_include_list[i]))
// OBSOLETE    {
// OBSOLETE    i = -1; 
// OBSOLETE    break;
// OBSOLETE    }
// OBSOLETE    if (i == -1)
// OBSOLETE    continue;
// OBSOLETE    }
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE 		    psymtab_include_list[includes_used++] = str;
// OBSOLETE 		    if (includes_used >= includes_allocated)
// OBSOLETE 		      {
// OBSOLETE 			char **orig = psymtab_include_list;
// OBSOLETE 
// OBSOLETE 			psymtab_include_list = (char **)
// OBSOLETE 			  alloca ((includes_allocated *= 2) * sizeof (char *));
// OBSOLETE 			memcpy ((PTR) psymtab_include_list, (PTR) orig,
// OBSOLETE 				includes_used * sizeof (char *));
// OBSOLETE 		      }
// OBSOLETE 
// OBSOLETE 		  }
// OBSOLETE 		psymfile_depth++;
// OBSOLETE 		continue;
// OBSOLETE 	      }
// OBSOLETE 
// OBSOLETE 	    case 'v':
// OBSOLETE 	      add_psymbol_to_list (namestring, p - namestring,
// OBSOLETE 				   VAR_NAMESPACE, LOC_STATIC,
// OBSOLETE 				   &objfile->static_psymbols,
// OBSOLETE 				   0, CUR_SYMBOL_VALUE,
// OBSOLETE 				   psymtab_language, objfile);
// OBSOLETE 	      continue;
// OBSOLETE 	    case 'V':
// OBSOLETE 	      add_psymbol_to_list (namestring, p - namestring,
// OBSOLETE 				   VAR_NAMESPACE, LOC_STATIC,
// OBSOLETE 				   &objfile->global_psymbols,
// OBSOLETE 				   0, CUR_SYMBOL_VALUE,
// OBSOLETE 				   psymtab_language, objfile);
// OBSOLETE 	      continue;
// OBSOLETE 
// OBSOLETE 	    case 'T':
// OBSOLETE 	      if (p != namestring)	/* a name is there, not just :T... */
// OBSOLETE 		{
// OBSOLETE 		  add_psymbol_to_list (namestring, p - namestring,
// OBSOLETE 				       STRUCT_NAMESPACE, LOC_TYPEDEF,
// OBSOLETE 				       &objfile->static_psymbols,
// OBSOLETE 				       CUR_SYMBOL_VALUE, 0,
// OBSOLETE 				       psymtab_language, objfile);
// OBSOLETE 		  if (p[2] == 't')
// OBSOLETE 		    {
// OBSOLETE 		      /* Also a typedef with the same name.  */
// OBSOLETE 		      add_psymbol_to_list (namestring, p - namestring,
// OBSOLETE 					   VAR_NAMESPACE, LOC_TYPEDEF,
// OBSOLETE 					   &objfile->static_psymbols,
// OBSOLETE 				      CUR_SYMBOL_VALUE, 0, psymtab_language,
// OBSOLETE 					   objfile);
// OBSOLETE 		      p += 1;
// OBSOLETE 		    }
// OBSOLETE 		  /* The semantics of C++ state that "struct foo { ... }"
// OBSOLETE 		     also defines a typedef for "foo".  Unfortuantely, cfront
// OBSOLETE 		     never makes the typedef when translating from C++ to C.
// OBSOLETE 		     We make the typedef here so that "ptype foo" works as
// OBSOLETE 		     expected for cfront translated code.  */
// OBSOLETE 		  else if (psymtab_language == language_cplus)
// OBSOLETE 		    {
// OBSOLETE 		      /* Also a typedef with the same name.  */
// OBSOLETE 		      add_psymbol_to_list (namestring, p - namestring,
// OBSOLETE 					   VAR_NAMESPACE, LOC_TYPEDEF,
// OBSOLETE 					   &objfile->static_psymbols,
// OBSOLETE 				      CUR_SYMBOL_VALUE, 0, psymtab_language,
// OBSOLETE 					   objfile);
// OBSOLETE 		    }
// OBSOLETE 		}
// OBSOLETE 	      goto check_enum;
// OBSOLETE 	    case 't':
// OBSOLETE 	      if (p != namestring)	/* a name is there, not just :T... */
// OBSOLETE 		{
// OBSOLETE 		  add_psymbol_to_list (namestring, p - namestring,
// OBSOLETE 				       VAR_NAMESPACE, LOC_TYPEDEF,
// OBSOLETE 				       &objfile->static_psymbols,
// OBSOLETE 				       CUR_SYMBOL_VALUE, 0,
// OBSOLETE 				       psymtab_language, objfile);
// OBSOLETE 		}
// OBSOLETE 	    check_enum:
// OBSOLETE 	      /* If this is an enumerated type, we need to
// OBSOLETE 	         add all the enum constants to the partial symbol
// OBSOLETE 	         table.  This does not cover enums without names, e.g.
// OBSOLETE 	         "enum {a, b} c;" in C, but fortunately those are
// OBSOLETE 	         rare.  There is no way for GDB to find those from the
// OBSOLETE 	         enum type without spending too much time on it.  Thus
// OBSOLETE 	         to solve this problem, the compiler needs to put out the
// OBSOLETE 	         enum in a nameless type.  GCC2 does this.  */
// OBSOLETE 
// OBSOLETE 	      /* We are looking for something of the form
// OBSOLETE 	         <name> ":" ("t" | "T") [<number> "="] "e" <size>
// OBSOLETE 	         {<constant> ":" <value> ","} ";".  */
// OBSOLETE 
// OBSOLETE 	      /* Skip over the colon and the 't' or 'T'.  */
// OBSOLETE 	      p += 2;
// OBSOLETE 	      /* This type may be given a number.  Also, numbers can come
// OBSOLETE 	         in pairs like (0,26).  Skip over it.  */
// OBSOLETE 	      while ((*p >= '0' && *p <= '9')
// OBSOLETE 		     || *p == '(' || *p == ',' || *p == ')'
// OBSOLETE 		     || *p == '=')
// OBSOLETE 		p++;
// OBSOLETE 
// OBSOLETE 	      if (*p++ == 'e')
// OBSOLETE 		{
// OBSOLETE 		  /* We have found an enumerated type. skip size */
// OBSOLETE 		  while (*p >= '0' && *p <= '9')
// OBSOLETE 		    p++;
// OBSOLETE 		  /* According to comments in read_enum_type
// OBSOLETE 		     a comma could end it instead of a semicolon.
// OBSOLETE 		     I don't know where that happens.
// OBSOLETE 		     Accept either.  */
// OBSOLETE 		  while (*p && *p != ';' && *p != ',')
// OBSOLETE 		    {
// OBSOLETE 		      char *q;
// OBSOLETE 
// OBSOLETE 		      /* Check for and handle cretinous dbx symbol name
// OBSOLETE 		         continuation! 
// OBSOLETE 		         if (*p == '\\')
// OBSOLETE 		         p = next_symbol_text (objfile);
// OBSOLETE 		       */
// OBSOLETE 
// OBSOLETE 		      /* Point to the character after the name
// OBSOLETE 		         of the enum constant.  */
// OBSOLETE 		      for (q = p; *q && *q != ':'; q++)
// OBSOLETE 			;
// OBSOLETE 		      /* Note that the value doesn't matter for
// OBSOLETE 		         enum constants in psymtabs, just in symtabs.  */
// OBSOLETE 		      add_psymbol_to_list (p, q - p,
// OBSOLETE 					   VAR_NAMESPACE, LOC_CONST,
// OBSOLETE 					   &objfile->static_psymbols, 0,
// OBSOLETE 					   0, psymtab_language, objfile);
// OBSOLETE 		      /* Point past the name.  */
// OBSOLETE 		      p = q;
// OBSOLETE 		      /* Skip over the value.  */
// OBSOLETE 		      while (*p && *p != ',')
// OBSOLETE 			p++;
// OBSOLETE 		      /* Advance past the comma.  */
// OBSOLETE 		      if (*p)
// OBSOLETE 			p++;
// OBSOLETE 		    }
// OBSOLETE 		}
// OBSOLETE 	      continue;
// OBSOLETE 	    case 'c':
// OBSOLETE 	      /* Constant, e.g. from "const" in Pascal.  */
// OBSOLETE 	      add_psymbol_to_list (namestring, p - namestring,
// OBSOLETE 				   VAR_NAMESPACE, LOC_CONST,
// OBSOLETE 				&objfile->static_psymbols, CUR_SYMBOL_VALUE,
// OBSOLETE 				   0, psymtab_language, objfile);
// OBSOLETE 	      continue;
// OBSOLETE 
// OBSOLETE 	    case 'f':
// OBSOLETE 	      CUR_SYMBOL_VALUE += ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE 	      if (pst && pst->textlow == 0)
// OBSOLETE 		pst->textlow = CUR_SYMBOL_VALUE;
// OBSOLETE 
// OBSOLETE 	      add_psymbol_to_list (namestring, p - namestring,
// OBSOLETE 				   VAR_NAMESPACE, LOC_BLOCK,
// OBSOLETE 				&objfile->static_psymbols, CUR_SYMBOL_VALUE,
// OBSOLETE 				   0, psymtab_language, objfile);
// OBSOLETE 	      continue;
// OBSOLETE 
// OBSOLETE 	    case 'F':
// OBSOLETE 	      CUR_SYMBOL_VALUE += ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE 	      if (pst && pst->textlow == 0)
// OBSOLETE 		pst->textlow = CUR_SYMBOL_VALUE;
// OBSOLETE 
// OBSOLETE 	      add_psymbol_to_list (namestring, p - namestring,
// OBSOLETE 				   VAR_NAMESPACE, LOC_BLOCK,
// OBSOLETE 				&objfile->global_psymbols, CUR_SYMBOL_VALUE,
// OBSOLETE 				   0, psymtab_language, objfile);
// OBSOLETE 	      continue;
// OBSOLETE 
// OBSOLETE 	    case 'p':
// OBSOLETE 	    case 'l':
// OBSOLETE 	    case 's':
// OBSOLETE 	      continue;
// OBSOLETE 
// OBSOLETE 	    case ':':
// OBSOLETE 	      /* It is a C++ nested symbol.  We don't need to record it
// OBSOLETE 	         (I don't think); if we try to look up foo::bar::baz,
// OBSOLETE 	         then symbols for the symtab containing foo should get
// OBSOLETE 	         read in, I think.  */
// OBSOLETE 	      /* Someone says sun cc puts out symbols like
// OBSOLETE 	         /foo/baz/maclib::/usr/local/bin/maclib,
// OBSOLETE 	         which would get here with a symbol type of ':'.  */
// OBSOLETE 	      continue;
// OBSOLETE 
// OBSOLETE 	    default:
// OBSOLETE 	      /* Unexpected symbol descriptor.  The second and subsequent stabs
// OBSOLETE 	         of a continued stab can show up here.  The question is
// OBSOLETE 	         whether they ever can mimic a normal stab--it would be
// OBSOLETE 	         nice if not, since we certainly don't want to spend the
// OBSOLETE 	         time searching to the end of every string looking for
// OBSOLETE 	         a backslash.  */
// OBSOLETE 
// OBSOLETE 	      complain (&unknown_symchar_complaint, p[1]);
// OBSOLETE 	      continue;
// OBSOLETE 	    }
// OBSOLETE 
// OBSOLETE 	case N_SYM_RBRAC:
// OBSOLETE 	  CUR_SYMBOL_VALUE += ANOFFSET (objfile->section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE #ifdef HANDLE_RBRAC
// OBSOLETE 	  HANDLE_RBRAC (CUR_SYMBOL_VALUE);
// OBSOLETE 	  continue;
// OBSOLETE #endif
// OBSOLETE 	case N_SYM_LBRAC:
// OBSOLETE 	  continue;
// OBSOLETE 
// OBSOLETE 	default:
// OBSOLETE 	  /* If we haven't found it yet, ignore it.  It's probably some
// OBSOLETE 	     new type we don't know about yet.  */
// OBSOLETE 	  complain (&unknown_symtype_complaint,
// OBSOLETE 		    local_hex_string ((unsigned long) bufp->n_type));
// OBSOLETE 	  continue;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   DBX_SYMCOUNT (objfile) = symnum;
// OBSOLETE 
// OBSOLETE   /* If there's stuff to be cleaned up, clean it up.  */
// OBSOLETE   if (DBX_SYMCOUNT (objfile) > 0
// OBSOLETE /*FIXME, does this have a bug at start address 0? */
// OBSOLETE       && last_o_file_start
// OBSOLETE       && objfile->ei.entry_point < bufp->n_value
// OBSOLETE       && objfile->ei.entry_point >= last_o_file_start)
// OBSOLETE     {
// OBSOLETE       objfile->ei.entry_file_lowpc = last_o_file_start;
// OBSOLETE       objfile->ei.entry_file_highpc = bufp->n_value;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (pst)
// OBSOLETE     {
// OBSOLETE       os9k_end_psymtab (pst, psymtab_include_list, includes_used,
// OBSOLETE 			symnum, end_of_text_addr,
// OBSOLETE 			dependency_list, dependencies_used);
// OBSOLETE     }
// OBSOLETE /*
// OBSOLETE    do_cleanups (back_to);
// OBSOLETE  */
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Allocate and partially fill a partial symtab.  It will be
// OBSOLETE    completely filled at the end of the symbol list.
// OBSOLETE 
// OBSOLETE    SYMFILE_NAME is the name of the symbol-file we are reading from, and ADDR
// OBSOLETE    is the address relative to which its symbols are (incremental) or 0
// OBSOLETE    (normal). */
// OBSOLETE 
// OBSOLETE 
// OBSOLETE static struct partial_symtab *
// OBSOLETE os9k_start_psymtab (struct objfile *objfile, char *filename, CORE_ADDR textlow,
// OBSOLETE 		    int ldsymoff, int ldsymcnt,
// OBSOLETE 		    struct partial_symbol **global_syms,
// OBSOLETE 		    struct partial_symbol **static_syms)
// OBSOLETE {
// OBSOLETE   struct partial_symtab *result =
// OBSOLETE   start_psymtab_common (objfile, objfile->section_offsets,
// OBSOLETE 			filename, textlow, global_syms, static_syms);
// OBSOLETE 
// OBSOLETE   result->read_symtab_private = (char *)
// OBSOLETE     obstack_alloc (&objfile->psymbol_obstack, sizeof (struct symloc));
// OBSOLETE 
// OBSOLETE   LDSYMOFF (result) = ldsymoff;
// OBSOLETE   LDSYMCNT (result) = ldsymcnt;
// OBSOLETE   result->read_symtab = os9k_psymtab_to_symtab;
// OBSOLETE 
// OBSOLETE   /* Deduce the source language from the filename for this psymtab. */
// OBSOLETE   psymtab_language = deduce_language_from_filename (filename);
// OBSOLETE   return result;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Close off the current usage of PST.  
// OBSOLETE    Returns PST or NULL if the partial symtab was empty and thrown away.
// OBSOLETE    FIXME:  List variables and peculiarities of same.  */
// OBSOLETE 
// OBSOLETE static struct partial_symtab *
// OBSOLETE os9k_end_psymtab (struct partial_symtab *pst, char **include_list,
// OBSOLETE 		  int num_includes, int capping_symbol_cnt,
// OBSOLETE 		  CORE_ADDR capping_text,
// OBSOLETE 		  struct partial_symtab **dependency_list,
// OBSOLETE 		  int number_dependencies)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE   struct partial_symtab *p1;
// OBSOLETE   struct objfile *objfile = pst->objfile;
// OBSOLETE 
// OBSOLETE   if (capping_symbol_cnt != -1)
// OBSOLETE     LDSYMCNT (pst) = capping_symbol_cnt - LDSYMCNT (pst);
// OBSOLETE 
// OBSOLETE   /* Under Solaris, the N_SO symbols always have a value of 0,
// OBSOLETE      instead of the usual address of the .o file.  Therefore,
// OBSOLETE      we have to do some tricks to fill in texthigh and textlow.
// OBSOLETE      The first trick is in partial-stab.h: if we see a static
// OBSOLETE      or global function, and the textlow for the current pst
// OBSOLETE      is still 0, then we use that function's address for 
// OBSOLETE      the textlow of the pst.
// OBSOLETE 
// OBSOLETE      Now, to fill in texthigh, we remember the last function seen
// OBSOLETE      in the .o file (also in partial-stab.h).  Also, there's a hack in
// OBSOLETE      bfd/elf.c and gdb/elfread.c to pass the ELF st_size field
// OBSOLETE      to here via the misc_info field.  Therefore, we can fill in
// OBSOLETE      a reliable texthigh by taking the address plus size of the
// OBSOLETE      last function in the file.
// OBSOLETE 
// OBSOLETE      Unfortunately, that does not cover the case where the last function
// OBSOLETE      in the file is static.  See the paragraph below for more comments
// OBSOLETE      on this situation.
// OBSOLETE 
// OBSOLETE      Finally, if we have a valid textlow for the current file, we run
// OBSOLETE      down the partial_symtab_list filling in previous texthighs that
// OBSOLETE      are still unknown.  */
// OBSOLETE 
// OBSOLETE   if (pst->texthigh == 0 && last_function_name)
// OBSOLETE     {
// OBSOLETE       char *p;
// OBSOLETE       int n;
// OBSOLETE       struct minimal_symbol *minsym;
// OBSOLETE 
// OBSOLETE       p = strchr (last_function_name, ':');
// OBSOLETE       if (p == NULL)
// OBSOLETE 	p = last_function_name;
// OBSOLETE       n = p - last_function_name;
// OBSOLETE       p = alloca (n + 1);
// OBSOLETE       strncpy (p, last_function_name, n);
// OBSOLETE       p[n] = 0;
// OBSOLETE 
// OBSOLETE       minsym = lookup_minimal_symbol (p, NULL, objfile);
// OBSOLETE 
// OBSOLETE       if (minsym)
// OBSOLETE 	{
// OBSOLETE 	  pst->texthigh = SYMBOL_VALUE_ADDRESS (minsym) + (long) MSYMBOL_INFO (minsym);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* This file ends with a static function, and it's
// OBSOLETE 	     difficult to imagine how hard it would be to track down
// OBSOLETE 	     the elf symbol.  Luckily, most of the time no one will notice,
// OBSOLETE 	     since the next file will likely be compiled with -g, so
// OBSOLETE 	     the code below will copy the first fuction's start address 
// OBSOLETE 	     back to our texthigh variable.  (Also, if this file is the
// OBSOLETE 	     last one in a dynamically linked program, texthigh already
// OBSOLETE 	     has the right value.)  If the next file isn't compiled
// OBSOLETE 	     with -g, then the last function in this file winds up owning
// OBSOLETE 	     all of the text space up to the next -g file, or the end (minus
// OBSOLETE 	     shared libraries).  This only matters for single stepping,
// OBSOLETE 	     and even then it will still work, except that it will single
// OBSOLETE 	     step through all of the covered functions, instead of setting
// OBSOLETE 	     breakpoints around them as it usualy does.  This makes it
// OBSOLETE 	     pretty slow, but at least it doesn't fail.
// OBSOLETE 
// OBSOLETE 	     We can fix this with a fairly big change to bfd, but we need
// OBSOLETE 	     to coordinate better with Cygnus if we want to do that.  FIXME.  */
// OBSOLETE 	}
// OBSOLETE       last_function_name = NULL;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* this test will be true if the last .o file is only data */
// OBSOLETE   if (pst->textlow == 0)
// OBSOLETE     pst->textlow = pst->texthigh;
// OBSOLETE 
// OBSOLETE   /* If we know our own starting text address, then walk through all other
// OBSOLETE      psymtabs for this objfile, and if any didn't know their ending text
// OBSOLETE      address, set it to our starting address.  Take care to not set our
// OBSOLETE      own ending address to our starting address, nor to set addresses on
// OBSOLETE      `dependency' files that have both textlow and texthigh zero.  */
// OBSOLETE   if (pst->textlow)
// OBSOLETE     {
// OBSOLETE       ALL_OBJFILE_PSYMTABS (objfile, p1)
// OBSOLETE       {
// OBSOLETE 	if (p1->texthigh == 0 && p1->textlow != 0 && p1 != pst)
// OBSOLETE 	  {
// OBSOLETE 	    p1->texthigh = pst->textlow;
// OBSOLETE 	    /* if this file has only data, then make textlow match texthigh */
// OBSOLETE 	    if (p1->textlow == 0)
// OBSOLETE 	      p1->textlow = p1->texthigh;
// OBSOLETE 	  }
// OBSOLETE       }
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* End of kludge for patching Solaris textlow and texthigh.  */
// OBSOLETE 
// OBSOLETE   pst->n_global_syms =
// OBSOLETE     objfile->global_psymbols.next - (objfile->global_psymbols.list + pst->globals_offset);
// OBSOLETE   pst->n_static_syms =
// OBSOLETE     objfile->static_psymbols.next - (objfile->static_psymbols.list + pst->statics_offset);
// OBSOLETE 
// OBSOLETE   pst->number_of_dependencies = number_dependencies;
// OBSOLETE   if (number_dependencies)
// OBSOLETE     {
// OBSOLETE       pst->dependencies = (struct partial_symtab **)
// OBSOLETE 	obstack_alloc (&objfile->psymbol_obstack,
// OBSOLETE 		    number_dependencies * sizeof (struct partial_symtab *));
// OBSOLETE       memcpy (pst->dependencies, dependency_list,
// OBSOLETE 	      number_dependencies * sizeof (struct partial_symtab *));
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     pst->dependencies = 0;
// OBSOLETE 
// OBSOLETE   for (i = 0; i < num_includes; i++)
// OBSOLETE     {
// OBSOLETE       struct partial_symtab *subpst =
// OBSOLETE       allocate_psymtab (include_list[i], objfile);
// OBSOLETE 
// OBSOLETE       subpst->section_offsets = pst->section_offsets;
// OBSOLETE       subpst->read_symtab_private =
// OBSOLETE 	(char *) obstack_alloc (&objfile->psymbol_obstack,
// OBSOLETE 				sizeof (struct symloc));
// OBSOLETE       LDSYMOFF (subpst) =
// OBSOLETE 	LDSYMCNT (subpst) =
// OBSOLETE 	subpst->textlow =
// OBSOLETE 	subpst->texthigh = 0;
// OBSOLETE 
// OBSOLETE       /* We could save slight bits of space by only making one of these,
// OBSOLETE          shared by the entire set of include files.  FIXME-someday.  */
// OBSOLETE       subpst->dependencies = (struct partial_symtab **)
// OBSOLETE 	obstack_alloc (&objfile->psymbol_obstack,
// OBSOLETE 		       sizeof (struct partial_symtab *));
// OBSOLETE       subpst->dependencies[0] = pst;
// OBSOLETE       subpst->number_of_dependencies = 1;
// OBSOLETE 
// OBSOLETE       subpst->globals_offset =
// OBSOLETE 	subpst->n_global_syms =
// OBSOLETE 	subpst->statics_offset =
// OBSOLETE 	subpst->n_static_syms = 0;
// OBSOLETE 
// OBSOLETE       subpst->readin = 0;
// OBSOLETE       subpst->symtab = 0;
// OBSOLETE       subpst->read_symtab = pst->read_symtab;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   sort_pst_symbols (pst);
// OBSOLETE 
// OBSOLETE   /* If there is already a psymtab or symtab for a file of this name, 
// OBSOLETE      remove it.
// OBSOLETE      (If there is a symtab, more drastic things also happen.)
// OBSOLETE      This happens in VxWorks.  */
// OBSOLETE   free_named_symtabs (pst->filename);
// OBSOLETE 
// OBSOLETE   if (num_includes == 0
// OBSOLETE       && number_dependencies == 0
// OBSOLETE       && pst->n_global_syms == 0
// OBSOLETE       && pst->n_static_syms == 0)
// OBSOLETE     {
// OBSOLETE       /* Throw away this psymtab, it's empty.  We can't deallocate it, since
// OBSOLETE          it is on the obstack, but we can forget to chain it on the list.  */
// OBSOLETE       /* Indicate that psymtab was thrown away.  */
// OBSOLETE 
// OBSOLETE       discard_psymtab (pst);
// OBSOLETE 
// OBSOLETE       pst = (struct partial_symtab *) NULL;
// OBSOLETE     }
// OBSOLETE   return pst;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE os9k_psymtab_to_symtab_1 (struct partial_symtab *pst)
// OBSOLETE {
// OBSOLETE   struct cleanup *old_chain;
// OBSOLETE   int i;
// OBSOLETE 
// OBSOLETE   if (!pst)
// OBSOLETE     return;
// OBSOLETE 
// OBSOLETE   if (pst->readin)
// OBSOLETE     {
// OBSOLETE       fprintf_unfiltered (gdb_stderr, "Psymtab for %s already read in.  Shouldn't happen.\n",
// OBSOLETE 			  pst->filename);
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Read in all partial symtabs on which this one is dependent */
// OBSOLETE   for (i = 0; i < pst->number_of_dependencies; i++)
// OBSOLETE     if (!pst->dependencies[i]->readin)
// OBSOLETE       {
// OBSOLETE 	/* Inform about additional files that need to be read in.  */
// OBSOLETE 	if (info_verbose)
// OBSOLETE 	  {
// OBSOLETE 	    fputs_filtered (" ", gdb_stdout);
// OBSOLETE 	    wrap_here ("");
// OBSOLETE 	    fputs_filtered ("and ", gdb_stdout);
// OBSOLETE 	    wrap_here ("");
// OBSOLETE 	    printf_filtered ("%s...", pst->dependencies[i]->filename);
// OBSOLETE 	    wrap_here ("");	/* Flush output */
// OBSOLETE 	    gdb_flush (gdb_stdout);
// OBSOLETE 	  }
// OBSOLETE 	os9k_psymtab_to_symtab_1 (pst->dependencies[i]);
// OBSOLETE       }
// OBSOLETE 
// OBSOLETE   if (LDSYMCNT (pst))		/* Otherwise it's a dummy */
// OBSOLETE     {
// OBSOLETE       /* Init stuff necessary for reading in symbols */
// OBSOLETE       stabsread_init ();
// OBSOLETE       buildsym_init ();
// OBSOLETE       old_chain = make_cleanup (really_free_pendings, 0);
// OBSOLETE 
// OBSOLETE       /* Read in this file's symbols */
// OBSOLETE       os9k_read_ofile_symtab (pst);
// OBSOLETE       sort_symtab_syms (pst->symtab);
// OBSOLETE       do_cleanups (old_chain);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   pst->readin = 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Read in all of the symbols for a given psymtab for real.
// OBSOLETE    Be verbose about it if the user wants that.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE os9k_psymtab_to_symtab (struct partial_symtab *pst)
// OBSOLETE {
// OBSOLETE   bfd *sym_bfd;
// OBSOLETE 
// OBSOLETE   if (!pst)
// OBSOLETE     return;
// OBSOLETE 
// OBSOLETE   if (pst->readin)
// OBSOLETE     {
// OBSOLETE       fprintf_unfiltered (gdb_stderr, "Psymtab for %s already read in.  Shouldn't happen.\n",
// OBSOLETE 			  pst->filename);
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (LDSYMCNT (pst) || pst->number_of_dependencies)
// OBSOLETE     {
// OBSOLETE       /* Print the message now, before reading the string table,
// OBSOLETE          to avoid disconcerting pauses.  */
// OBSOLETE       if (info_verbose)
// OBSOLETE 	{
// OBSOLETE 	  printf_filtered ("Reading in symbols for %s...", pst->filename);
// OBSOLETE 	  gdb_flush (gdb_stdout);
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       sym_bfd = pst->objfile->obfd;
// OBSOLETE       os9k_psymtab_to_symtab_1 (pst);
// OBSOLETE 
// OBSOLETE       /* Match with global symbols.  This only needs to be done once,
// OBSOLETE          after all of the symtabs and dependencies have been read in.   */
// OBSOLETE       scan_file_globals (pst->objfile);
// OBSOLETE 
// OBSOLETE       /* Finish up the debug error message.  */
// OBSOLETE       if (info_verbose)
// OBSOLETE 	printf_filtered ("done.\n");
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Read in a defined section of a specific object file's symbols. */
// OBSOLETE static void
// OBSOLETE os9k_read_ofile_symtab (struct partial_symtab *pst)
// OBSOLETE {
// OBSOLETE   register struct internal_symstruct *bufp;
// OBSOLETE   unsigned char type;
// OBSOLETE   unsigned max_symnum;
// OBSOLETE   register bfd *abfd;
// OBSOLETE   struct objfile *objfile;
// OBSOLETE   int sym_offset;		/* Offset to start of symbols to read */
// OBSOLETE   CORE_ADDR text_offset;	/* Start of text segment for symbols */
// OBSOLETE   int text_size;		/* Size of text segment for symbols */
// OBSOLETE   FILE *dbg_file;
// OBSOLETE 
// OBSOLETE   objfile = pst->objfile;
// OBSOLETE   sym_offset = LDSYMOFF (pst);
// OBSOLETE   max_symnum = LDSYMCNT (pst);
// OBSOLETE   text_offset = pst->textlow;
// OBSOLETE   text_size = pst->texthigh - pst->textlow;
// OBSOLETE 
// OBSOLETE   current_objfile = objfile;
// OBSOLETE   subfile_stack = NULL;
// OBSOLETE   last_source_file = NULL;
// OBSOLETE 
// OBSOLETE   abfd = objfile->obfd;
// OBSOLETE   dbg_file = objfile->auxf2;
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE   /* It is necessary to actually read one symbol *before* the start
// OBSOLETE      of this symtab's symbols, because the GCC_COMPILED_FLAG_SYMBOL
// OBSOLETE      occurs before the N_SO symbol.
// OBSOLETE      Detecting this in read_dbx_symtab
// OBSOLETE      would slow down initial readin, so we look for it here instead. */
// OBSOLETE   if (!processing_acc_compilation && sym_offset >= (int) symbol_size)
// OBSOLETE     {
// OBSOLETE       fseek (objefile->auxf2, sym_offset, SEEK_CUR);
// OBSOLETE       fill_sym (objfile->auxf2, abfd);
// OBSOLETE       bufp = symbuf;
// OBSOLETE 
// OBSOLETE       processing_gcc_compilation = 0;
// OBSOLETE       if (bufp->n_type == N_TEXT)
// OBSOLETE 	{
// OBSOLETE 	  if (STREQ (namestring, GCC_COMPILED_FLAG_SYMBOL))
// OBSOLETE 	    processing_gcc_compilation = 1;
// OBSOLETE 	  else if (STREQ (namestring, GCC2_COMPILED_FLAG_SYMBOL))
// OBSOLETE 	    processing_gcc_compilation = 2;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       /* Try to select a C++ demangling based on the compilation unit
// OBSOLETE          producer. */
// OBSOLETE 
// OBSOLETE       if (processing_gcc_compilation)
// OBSOLETE 	{
// OBSOLETE 	  if (AUTO_DEMANGLING)
// OBSOLETE 	    {
// OBSOLETE 	      set_demangling_style (GNU_DEMANGLING_STYLE_STRING);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       /* The N_SO starting this symtab is the first symbol, so we
// OBSOLETE          better not check the symbol before it.  I'm not this can
// OBSOLETE          happen, but it doesn't hurt to check for it.  */
// OBSOLETE       bfd_seek (symfile_bfd, sym_offset, SEEK_CUR);
// OBSOLETE       processing_gcc_compilation = 0;
// OBSOLETE     }
// OBSOLETE #endif /* 0 */
// OBSOLETE 
// OBSOLETE   fseek (dbg_file, (long) sym_offset, 0);
// OBSOLETE /*
// OBSOLETE    if (bufp->n_type != (unsigned char)N_SYM_SYM)
// OBSOLETE    error("First symbol in segment of executable not a source symbol");
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE   for (symnum = 0; symnum < max_symnum; symnum++)
// OBSOLETE     {
// OBSOLETE       QUIT;			/* Allow this to be interruptable */
// OBSOLETE       fill_sym (dbg_file, abfd);
// OBSOLETE       bufp = symbuf;
// OBSOLETE       type = bufp->n_type;
// OBSOLETE 
// OBSOLETE       os9k_process_one_symbol ((int) type, (int) bufp->n_desc,
// OBSOLETE 	 (CORE_ADDR) bufp->n_value, bufp->n_strx, pst->section_offsets, objfile);
// OBSOLETE 
// OBSOLETE       /* We skip checking for a new .o or -l file; that should never
// OBSOLETE          happen in this routine. */
// OBSOLETE #if 0
// OBSOLETE       else
// OBSOLETE       if (type == N_TEXT)
// OBSOLETE 	{
// OBSOLETE 	  /* I don't think this code will ever be executed, because
// OBSOLETE 	     the GCC_COMPILED_FLAG_SYMBOL usually is right before
// OBSOLETE 	     the N_SO symbol which starts this source file.
// OBSOLETE 	     However, there is no reason not to accept
// OBSOLETE 	     the GCC_COMPILED_FLAG_SYMBOL anywhere.  */
// OBSOLETE 
// OBSOLETE 	  if (STREQ (namestring, GCC_COMPILED_FLAG_SYMBOL))
// OBSOLETE 	    processing_gcc_compilation = 1;
// OBSOLETE 	  else if (STREQ (namestring, GCC2_COMPILED_FLAG_SYMBOL))
// OBSOLETE 	    processing_gcc_compilation = 2;
// OBSOLETE 
// OBSOLETE 	  if (AUTO_DEMANGLING)
// OBSOLETE 	    {
// OBSOLETE 	      set_demangling_style (GNU_DEMANGLING_STYLE_STRING);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       else if (type & N_EXT || type == (unsigned char) N_TEXT
// OBSOLETE 	       || type == (unsigned char) N_NBTEXT
// OBSOLETE 	)
// OBSOLETE 	{
// OBSOLETE 	  /* Global symbol: see if we came across a dbx defintion for
// OBSOLETE 	     a corresponding symbol.  If so, store the value.  Remove
// OBSOLETE 	     syms from the chain when their values are stored, but
// OBSOLETE 	     search the whole chain, as there may be several syms from
// OBSOLETE 	     different files with the same name. */
// OBSOLETE 	  /* This is probably not true.  Since the files will be read
// OBSOLETE 	     in one at a time, each reference to a global symbol will
// OBSOLETE 	     be satisfied in each file as it appears. So we skip this
// OBSOLETE 	     section. */
// OBSOLETE 	  ;
// OBSOLETE 	}
// OBSOLETE #endif /* 0 */
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   current_objfile = NULL;
// OBSOLETE 
// OBSOLETE   /* In a Solaris elf file, this variable, which comes from the
// OBSOLETE      value of the N_SO symbol, will still be 0.  Luckily, text_offset,
// OBSOLETE      which comes from pst->textlow is correct. */
// OBSOLETE   if (last_source_start_addr == 0)
// OBSOLETE     last_source_start_addr = text_offset;
// OBSOLETE   pst->symtab = end_symtab (text_offset + text_size, objfile, SECT_OFF_TEXT (objfile));
// OBSOLETE   end_stabs ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* This handles a single symbol from the symbol-file, building symbols
// OBSOLETE    into a GDB symtab.  It takes these arguments and an implicit argument.
// OBSOLETE 
// OBSOLETE    TYPE is the type field of the ".stab" symbol entry.
// OBSOLETE    DESC is the desc field of the ".stab" entry.
// OBSOLETE    VALU is the value field of the ".stab" entry.
// OBSOLETE    NAME is the symbol name, in our address space.
// OBSOLETE    SECTION_OFFSETS is a set of amounts by which the sections of this object
// OBSOLETE    file were relocated when it was loaded into memory.
// OBSOLETE    All symbols that refer
// OBSOLETE    to memory locations need to be offset by these amounts.
// OBSOLETE    OBJFILE is the object file from which we are reading symbols.
// OBSOLETE    It is used in end_symtab.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE os9k_process_one_symbol (int type, int desc, CORE_ADDR valu, char *name,
// OBSOLETE 			 struct section_offsets *section_offsets,
// OBSOLETE 			 struct objfile *objfile)
// OBSOLETE {
// OBSOLETE   register struct context_stack *new;
// OBSOLETE   /* The stab type used for the definition of the last function.
// OBSOLETE      N_STSYM or N_GSYM for SunOS4 acc; N_FUN for other compilers.  */
// OBSOLETE   static int function_stab_type = 0;
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE   /* Something is wrong if we see real data before
// OBSOLETE      seeing a source file name.  */
// OBSOLETE   if (last_source_file == NULL && type != (unsigned char) N_SO)
// OBSOLETE     {
// OBSOLETE       /* Ignore any symbols which appear before an N_SO symbol.
// OBSOLETE          Currently no one puts symbols there, but we should deal
// OBSOLETE          gracefully with the case.  A complain()t might be in order,
// OBSOLETE          but this should not be an error ().  */
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE #endif /* 0 */
// OBSOLETE 
// OBSOLETE   switch (type)
// OBSOLETE     {
// OBSOLETE     case N_SYM_LBRAC:
// OBSOLETE       /* On most machines, the block addresses are relative to the
// OBSOLETE          N_SO, the linker did not relocate them (sigh).  */
// OBSOLETE       valu += ANOFFSET (section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE       new = push_context (desc, valu);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case N_SYM_RBRAC:
// OBSOLETE       valu += ANOFFSET (section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE       new = pop_context ();
// OBSOLETE 
// OBSOLETE #if !defined (OS9K_VARIABLES_INSIDE_BLOCK)
// OBSOLETE #define OS9K_VARIABLES_INSIDE_BLOCK(desc, gcc_p) 1
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE       if (!OS9K_VARIABLES_INSIDE_BLOCK (desc, processing_gcc_compilation))
// OBSOLETE 	local_symbols = new->locals;
// OBSOLETE 
// OBSOLETE       if (context_stack_depth > 1)
// OBSOLETE 	{
// OBSOLETE 	  /* This is not the outermost LBRAC...RBRAC pair in the function,
// OBSOLETE 	     its local symbols preceded it, and are the ones just recovered
// OBSOLETE 	     from the context stack.  Define the block for them (but don't
// OBSOLETE 	     bother if the block contains no symbols.  Should we complain
// OBSOLETE 	     on blocks without symbols?  I can't think of any useful purpose
// OBSOLETE 	     for them).  */
// OBSOLETE 	  if (local_symbols != NULL)
// OBSOLETE 	    {
// OBSOLETE 	      /* Muzzle a compiler bug that makes end < start.  (which
// OBSOLETE 	         compilers?  Is this ever harmful?).  */
// OBSOLETE 	      if (new->start_addr > valu)
// OBSOLETE 		{
// OBSOLETE 		  complain (&lbrac_rbrac_complaint);
// OBSOLETE 		  new->start_addr = valu;
// OBSOLETE 		}
// OBSOLETE 	      /* Make a block for the local symbols within.  */
// OBSOLETE 	      finish_block (0, &local_symbols, new->old_blocks,
// OBSOLETE 			    new->start_addr, valu, objfile);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  if (context_stack_depth == 0)
// OBSOLETE 	    {
// OBSOLETE 	      within_function = 0;
// OBSOLETE 	      /* Make a block for the local symbols within.  */
// OBSOLETE 	      finish_block (new->name, &local_symbols, new->old_blocks,
// OBSOLETE 			    new->start_addr, valu, objfile);
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      /* attach local_symbols to the end of new->locals */
// OBSOLETE 	      if (!new->locals)
// OBSOLETE 		new->locals = local_symbols;
// OBSOLETE 	      else
// OBSOLETE 		{
// OBSOLETE 		  struct pending *p;
// OBSOLETE 
// OBSOLETE 		  p = new->locals;
// OBSOLETE 		  while (p->next)
// OBSOLETE 		    p = p->next;
// OBSOLETE 		  p->next = local_symbols;
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       if (OS9K_VARIABLES_INSIDE_BLOCK (desc, processing_gcc_compilation))
// OBSOLETE 	/* Now pop locals of block just finished.  */
// OBSOLETE 	local_symbols = new->locals;
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE     case N_SYM_SLINE:
// OBSOLETE       /* This type of "symbol" really just records
// OBSOLETE          one line-number -- core-address correspondence.
// OBSOLETE          Enter it in the line list for this symbol table. */
// OBSOLETE       /* Relocate for dynamic loading and for ELF acc fn-relative syms.  */
// OBSOLETE       valu += ANOFFSET (section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE       /* FIXME: loses if sizeof (char *) > sizeof (int) */
// OBSOLETE       gdb_assert (sizeof (name) <= sizeof (int));
// OBSOLETE       record_line (current_subfile, (int) name, valu);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE       /* The following symbol types need to have the appropriate offset added
// OBSOLETE          to their value; then we process symbol definitions in the name.  */
// OBSOLETE     case N_SYM_SYM:
// OBSOLETE 
// OBSOLETE       if (name)
// OBSOLETE 	{
// OBSOLETE 	  char deftype;
// OBSOLETE 	  char *dirn, *n;
// OBSOLETE 	  char *p = strchr (name, ':');
// OBSOLETE 	  if (p == NULL)
// OBSOLETE 	    deftype = '\0';
// OBSOLETE 	  else
// OBSOLETE 	    deftype = p[1];
// OBSOLETE 
// OBSOLETE 
// OBSOLETE 	  switch (deftype)
// OBSOLETE 	    {
// OBSOLETE 	    case 'S':
// OBSOLETE 	      valu += ANOFFSET (section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE 	      n = strrchr (name, '/');
// OBSOLETE 	      if (n != NULL)
// OBSOLETE 		{
// OBSOLETE 		  *n = '\0';
// OBSOLETE 		  n++;
// OBSOLETE 		  dirn = name;
// OBSOLETE 		}
// OBSOLETE 	      else
// OBSOLETE 		{
// OBSOLETE 		  n = name;
// OBSOLETE 		  dirn = NULL;
// OBSOLETE 		}
// OBSOLETE 	      *p = '\0';
// OBSOLETE 	      if (symfile_depth++ == 0)
// OBSOLETE 		{
// OBSOLETE 		  if (last_source_file)
// OBSOLETE 		    {
// OBSOLETE 		      end_symtab (valu, objfile, SECT_OFF_TEXT (objfile));
// OBSOLETE 		      end_stabs ();
// OBSOLETE 		    }
// OBSOLETE 		  start_stabs ();
// OBSOLETE 		  os9k_stabs = 1;
// OBSOLETE 		  start_symtab (n, dirn, valu);
// OBSOLETE 		  record_debugformat ("OS9");
// OBSOLETE 		}
// OBSOLETE 	      else
// OBSOLETE 		{
// OBSOLETE 		  push_subfile ();
// OBSOLETE 		  start_subfile (n, dirn != NULL ? dirn : current_subfile->dirname);
// OBSOLETE 		}
// OBSOLETE 	      break;
// OBSOLETE 
// OBSOLETE 	    case 'f':
// OBSOLETE 	    case 'F':
// OBSOLETE 	      valu += ANOFFSET (section_offsets, SECT_OFF_TEXT (objfile));
// OBSOLETE 	      function_stab_type = type;
// OBSOLETE 
// OBSOLETE 	      within_function = 1;
// OBSOLETE 	      new = push_context (0, valu);
// OBSOLETE 	      new->name = define_symbol (valu, name, desc, type, objfile);
// OBSOLETE 	      break;
// OBSOLETE 
// OBSOLETE 	    case 'V':
// OBSOLETE 	    case 'v':
// OBSOLETE 	      valu += ANOFFSET (section_offsets, SECT_OFF_DATA (objfile));
// OBSOLETE 	      define_symbol (valu, name, desc, type, objfile);
// OBSOLETE 	      break;
// OBSOLETE 
// OBSOLETE 	    default:
// OBSOLETE 	      define_symbol (valu, name, desc, type, objfile);
// OBSOLETE 	      break;
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case N_SYM_SE:
// OBSOLETE       if (--symfile_depth != 0)
// OBSOLETE 	start_subfile (pop_subfile (), current_subfile->dirname);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     default:
// OBSOLETE       complain (&unknown_symtype_complaint,
// OBSOLETE 		local_hex_string ((unsigned long) type));
// OBSOLETE       /* FALLTHROUGH */
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case N_SYM_CMPLR:
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE   previous_stab_code = type;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static struct sym_fns os9k_sym_fns =
// OBSOLETE {
// OBSOLETE   bfd_target_os9k_flavour,
// OBSOLETE   os9k_new_init,		/* sym_new_init: init anything gbl to entire symtab */
// OBSOLETE   os9k_symfile_init,		/* sym_init: read initial info, setup for sym_read() */
// OBSOLETE   os9k_symfile_read,		/* sym_read: read a symbol file into symtab */
// OBSOLETE   os9k_symfile_finish,		/* sym_finish: finished with file, cleanup */
// OBSOLETE   default_symfile_offsets,	/* sym_offsets: parse user's offsets to internal form */
// OBSOLETE   NULL				/* next: pointer to next struct sym_fns */
// OBSOLETE };
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_os9kread (void)
// OBSOLETE {
// OBSOLETE   add_symtab_fns (&os9k_sym_fns);
// OBSOLETE }
