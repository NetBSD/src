/* NetBSD-specific values for a.out files */

/* XXX This file is mostly copied from the sunos counterpart. Most, but
 * not all, Sun OS specific things has ben changed, so some comments may
 * be slightly wrong. */

/* Size of the text section.  */
#define	N_TXTSIZE(x) \
    (N_IS_QMAGIC (x) ? (x).a_text - EXEC_BYTES_SIZE : \
     (N_MAGIC(x) != ZMAGIC) ? (x).a_text :            \
     N_HEADER_IN_TEXT(x)  ?	                      \
	    (x).a_text - EXEC_BYTES_SIZE:	      \
	    (x).a_text				      \
    )

/* Offset in an a.out of the start of the text section. */
#define N_TXTOFF(x) \
    (N_MAGIC(x) != ZMAGIC ? EXEC_BYTES_SIZE : \
     N_HEADER_IN_TEXT(x) ?	              \
	    EXEC_BYTES_SIZE :                 \
	    ZMAGIC_DISK_BLOCK_SIZE            \
    )

/* The address of the data segment in virtual memory. */
#define N_TXTADDR(x) \
    (N_IS_QMAGIC (x) ? TARGET_PAGE_SIZE + EXEC_BYTES_SIZE : \
     N_MAGIC(x) != ZMAGIC ? 0 :	                            \
     N_SHARED_LIB(x) ?                                      \
	    EXEC_BYTES_SIZE :                               \
	    TEXT_START_ADDR + EXEC_BYTES_SIZE               \
    )

/* When a file is linked against a shared library on NetBSD, the
   dynamic bit in the exec header is set, and the first symbol in the
   symbol table is __DYNAMIC.  Its value is the address of the
   following structure.  */
struct external_netbsd_dynamic
{
  /* The version number of the structure.  NetBSD creates files
     with version number 8, which is what this structure is based on. */
  bfd_byte d_version[4];
  /* The virtual address of a 24 byte structure used in debugging.
     The contents are filled in at run time by ld.so.  */
  bfd_byte d_debug[4];
  /* The virtual address of another structure with information about
     how to relocate the executable at run time.  */
  bfd_byte d_un[4];
  /* compat - now in crt_ldso */
  bfd_byte d_entry[4];
};

/* The size of the debugging structure pointed to by the debugger
   field of __DYNAMIC.  */
#define EXTERNAL_NETBSD_DYNAMIC_DEBUGGER_SIZE (24)

/* The structure pointed to by the d_un field of __DYNAMIC.  As far
   as I can tell, most of the addresses in this structure are offsets
   within the file, but some are actually virtual addresses.  */
struct internal_section_dispatch_table
{
  /* Linked list of loaded objects.  This is filled in at runtime by
     ld.so and probably by dlopen.  */
  unsigned long sdt_loaded;

  /* The address of the list of names of shared objects which must be
     included at runtime.  Each entry in the list is 16 bytes: the 4
     byte address of the string naming the object (e.g., for -lc this
     is "c"); 4 bytes of flags--the high bit is whether to search for
     the object using the library path; the 2 byte major version
     number; the 2 byte minor version number; the 4 byte address of
     the next entry in the list (zero if this is the last entry).  The
     version numbers seem to only be non-zero when doing library
     searching.  */
  unsigned long sdt_sods;

  /* The address of the path to search for the shared objects which
     must be included.  This points to a string in PATH format which
     is generated from the -L arguments to the linker.  According to
     the man page, ld.so implicitly adds ${LD_LIBRARY_PATH} to the
     beginning of this string and /lib:/usr/lib to the end.  The string
     is terminated by a null byte.  This field is zero if there is no
     additional path.  */
  unsigned long sdt_paths;

  /* The address of the global offset table.  This appears to be a
     virtual address, not a file offset.  The first entry in the
     global offset table seems to be the virtual address of the
     _dynamic structure (the same value as the __DYNAMIC symbol).
     The global offset table is used for PIC code to hold the
     addresses of variables.  A dynamically linked file which does not
     itself contain PIC code has a four byte global offset table.  */
  unsigned long sdt_got;

  /* The address of the procedure linkage table.  This appears to be a
     virtual address, not a file offset.

     On a SPARC, the table is composed of 12 byte entries, each of
     which consists of three instructions.  The first entry is
         sethi %hi(0),%g1
	 jmp %g1
	 nop
     These instructions are changed by ld.so into a jump directly into
     ld.so itself.  Each subsequent entry is
         save %sp, -96, %sp
	 call <address of first entry in procedure linkage table>
	 <reloc_number | 0x01000000>
     The reloc_number is the number of the reloc to use to resolve
     this entry.  The reloc will be a JMP_SLOT reloc against some
     symbol that is not defined in this object file but should be
     defined in a shared object (if it is not, ld.so will report a
     runtime error and exit).  The constant 0x010000000 turns the
     reloc number into a sethi of %g0, which does nothing since %g0 is
     hardwired to zero.

     When one of these entries is executed, it winds up calling into
     ld.so.  ld.so looks at the reloc number, available via the return
     address, to determine which entry this is.  It then looks at the
     reloc and patches up the entry in the table into a sethi and jmp
     to the real address followed by a nop.  This means that the reloc
     lookup only has to happen once, and it also means that the
     relocation only needs to be done if the function is actually
     called.  The relocation is expensive because ld.so must look up
     the symbol by name.

     The size of the procedure linkage table is given by the ld_plt_sz
     field.  */
  unsigned long sdt_plt;

  /* The address of the relocs.  These are in the same format as
     ordinary relocs.  Symbol index numbers refer to the symbols
     pointed to by ld_stab.  I think the only way to determine the
     number of relocs is to assume that all the bytes from ld_rel to
     ld_hash contain reloc entries.  */
  unsigned long sdt_rel;

  /* The address of a hash table of symbols.  The hash table has
     roughly the same number of entries as there are dynamic symbols;
     I think the only way to get the exact size is to assume that
     every byte from ld_hash to ld_stab is devoted to the hash table.

     Each entry in the hash table is eight bytes.  The first four
     bytes are a symbol index into the dynamic symbols.  The second
     four bytes are the index of the next hash table entry in the
     bucket.  The ld_buckets field gives the number of buckets, say B.
     The first B entries in the hash table each start a bucket which
     is chained through the second four bytes of each entry.  A value
     of zero ends the chain.

     The hash function is simply
         h = 0;
         while (*string != '\0')
	   h = (h << 1) + *string++;
	 h &= 0x7fffffff;

     To look up a symbol, compute the hash value of the name.  Take
     the modulos of hash value and the number of buckets.  Start at
     that entry in the hash table.  See if the symbol (from the first
     four bytes of the hash table entry) has the name you are looking
     for.  If not, use the chain field (the second four bytes of the
     hash table entry) to move on to the next entry in this bucket.
     If the chain field is zero you have reached the end of the
     bucket, and the symbol is not in the hash table.  */ 
  unsigned long sdt_hash;

  /* The address of the symbol table.  This is a list of
     external_nzlist structures.  The string indices are relative to
     the ld_symbols field.  I think the only way to determine the
     number of symbols is to assume that all the bytes between ld_stab
     and ld_symbols are external_nzlist structures.  */
  unsigned long sdt_nzlist;

  /* Unusued */
  unsigned long sdt_filler2;

  /* The number of buckets in the hash table.  */
  unsigned long sdt_buckets;

  /* The address of the symbol string table.  The first string in this
     string table need not be the empty string.  */
  unsigned long sdt_strings;

  /* The size in bytes of the symbol string table.  */
  unsigned long sdt_str_sz;

  /* The size in bytes of the text segment.  */
  unsigned long sdt_text_sz;

  /* The size in bytes of the procedure linkage table.  */
  unsigned long sdt_plt_sz;
};

/* The external form of the structure.  */
struct external_netbsd_dynamic_link
{
  bfd_byte sdt_loaded[4];
  bfd_byte sdt_sods[4];
  bfd_byte sdt_paths[4];
  bfd_byte sdt_got[4];
  bfd_byte sdt_plt[4];
  bfd_byte sdt_rel[4];
  bfd_byte sdt_hash[4];
  bfd_byte sdt_nzlist[4];
  bfd_byte sdt_filler2[4];
  bfd_byte sdt_buckets[4];
  bfd_byte sdt_strings[4];
  bfd_byte sdt_str_sz[4];
  bfd_byte sdt_text_sz[4];
  bfd_byte sdt_plt_sz[4];
};
