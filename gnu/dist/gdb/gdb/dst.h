// OBSOLETE /* <apollo/dst.h> */
// OBSOLETE /* Apollo object module DST (debug symbol table) description */
// OBSOLETE 
// OBSOLETE #ifndef apollo_dst_h
// OBSOLETE #define apollo_dst_h
// OBSOLETE 
// OBSOLETE #if defined(apollo) && !defined(__GNUC__)
// OBSOLETE #define ALIGNED1  __attribute( (aligned(1)) )
// OBSOLETE #else
// OBSOLETE /* Remove attribute directives from non-Apollo code: */
// OBSOLETE #define ALIGNED1		/* nil */
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Identification of this version of the debug symbol table.  Producers of the
// OBSOLETE    debug symbol table must write these values into the version number field of
// OBSOLETE    the compilation unit record in .blocks .
// OBSOLETE  */
// OBSOLETE #define dst_version_major    1
// OBSOLETE #define dst_version_minor    3
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    ** Enumeration of debug record types appearing in .blocks and .symbols ...
// OBSOLETE  */
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_typ_pad,		/*  0 */
// OBSOLETE     dst_typ_comp_unit,		/*  1 */
// OBSOLETE     dst_typ_section_tab,	/*  2 */
// OBSOLETE     dst_typ_file_tab,		/*  3 */
// OBSOLETE     dst_typ_block,		/*  4 */
// OBSOLETE     dst_typ_5,
// OBSOLETE     dst_typ_var,
// OBSOLETE     dst_typ_pointer,		/*  7 */
// OBSOLETE     dst_typ_array,		/*  8 */
// OBSOLETE     dst_typ_subrange,		/*  9 */
// OBSOLETE     dst_typ_set,		/* 10 */
// OBSOLETE     dst_typ_implicit_enum,	/* 11 */
// OBSOLETE     dst_typ_explicit_enum,	/* 12 */
// OBSOLETE     dst_typ_short_rec,		/* 13 */
// OBSOLETE     dst_typ_old_record,
// OBSOLETE     dst_typ_short_union,	/* 15 */
// OBSOLETE     dst_typ_old_union,
// OBSOLETE     dst_typ_file,		/* 17 */
// OBSOLETE     dst_typ_offset,		/* 18 */
// OBSOLETE     dst_typ_alias,		/* 19 */
// OBSOLETE     dst_typ_signature,		/* 20 */
// OBSOLETE     dst_typ_21,
// OBSOLETE     dst_typ_old_label,		/* 22 */
// OBSOLETE     dst_typ_scope,		/* 23 */
// OBSOLETE     dst_typ_end_scope,		/* 24 */
// OBSOLETE     dst_typ_25,
// OBSOLETE     dst_typ_26,
// OBSOLETE     dst_typ_string_tab,		/* 27 */
// OBSOLETE     dst_typ_global_name_tab,	/* 28 */
// OBSOLETE     dst_typ_forward,		/* 29 */
// OBSOLETE     dst_typ_aux_size,		/* 30 */
// OBSOLETE     dst_typ_aux_align,		/* 31 */
// OBSOLETE     dst_typ_aux_field_size,	/* 32 */
// OBSOLETE     dst_typ_aux_field_off,	/* 33 */
// OBSOLETE     dst_typ_aux_field_align,	/* 34 */
// OBSOLETE     dst_typ_aux_qual,		/* 35 */
// OBSOLETE     dst_typ_aux_var_bound,	/* 36 */
// OBSOLETE     dst_typ_extension,		/* 37 */
// OBSOLETE     dst_typ_string,		/* 38 */
// OBSOLETE     dst_typ_old_entry,
// OBSOLETE     dst_typ_const,		/* 40 */
// OBSOLETE     dst_typ_reference,		/* 41 */
// OBSOLETE     dst_typ_record,		/* 42 */
// OBSOLETE     dst_typ_union,		/* 43 */
// OBSOLETE     dst_typ_aux_type_deriv,	/* 44 */
// OBSOLETE     dst_typ_locpool,		/* 45 */
// OBSOLETE     dst_typ_variable,		/* 46 */
// OBSOLETE     dst_typ_label,		/* 47 */
// OBSOLETE     dst_typ_entry,		/* 48 */
// OBSOLETE     dst_typ_aux_lifetime,	/* 49 */
// OBSOLETE     dst_typ_aux_ptr_base,	/* 50 */
// OBSOLETE     dst_typ_aux_src_range,	/* 51 */
// OBSOLETE     dst_typ_aux_reg_val,	/* 52 */
// OBSOLETE     dst_typ_aux_unit_names,	/* 53 */
// OBSOLETE     dst_typ_aux_sect_info,	/* 54 */
// OBSOLETE     dst_typ_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_rec_type_t;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    ** Dummy bounds for variably dimensioned arrays:
// OBSOLETE  */
// OBSOLETE #define dst_dummy_array_size  100
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    ** Reference to another item in the symbol table.
// OBSOLETE    **
// OBSOLETE    ** The value of a dst_rel_offset_t is the relative offset from the start of the
// OBSOLETE    ** referencing record to the start of the referenced record, string, etc. 
// OBSOLETE    **
// OBSOLETE    ** The value of a NIL dst_rel_offset_t is zero.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE typedef long dst_rel_offset_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* FIXME: Here and many places we make assumptions about sizes of host
// OBSOLETE    data types, structure layout, etc.  Only needs to be fixed if we care
// OBSOLETE    about cross-debugging, though.  */
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    ** Section-relative reference. 
// OBSOLETE    **
// OBSOLETE    ** The section index field is an index into the local compilation unit's
// OBSOLETE    ** section table (see dst_rec_section_tab_t)--NOT into the object module
// OBSOLETE    ** section table!
// OBSOLETE    **
// OBSOLETE    ** The sect_offset field is the offset in bytes into the section.
// OBSOLETE    **
// OBSOLETE    ** A NIL dst_sect_ref_t has a sect_index field of zero.  Indexes originate
// OBSOLETE    ** at one.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short sect_index;
// OBSOLETE     unsigned long sect_offset ALIGNED1;
// OBSOLETE   }
// OBSOLETE dst_sect_ref_t;
// OBSOLETE 
// OBSOLETE #define dst_sect_index_nil    0
// OBSOLETE #define dst_sect_index_origin 1
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    ** Source location descriptor.
// OBSOLETE    **
// OBSOLETE    ** The file_index field is an index into the local compilation unit's
// OBSOLETE    ** file table (see dst_rec_file_tab_t).
// OBSOLETE    **
// OBSOLETE    ** A NIL dst_src_loc_t has a file_index field of zero.  Indexes originate
// OBSOLETE    ** at one.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     boolean reserved:1;		/* reserved for future use */
// OBSOLETE     int file_index:11;		/* index into .blocks source file list */
// OBSOLETE     int line_number:20;		/* source line number */
// OBSOLETE   }
// OBSOLETE dst_src_loc_t;
// OBSOLETE 
// OBSOLETE #define dst_file_index_nil    0
// OBSOLETE #define dst_file_index_origin 1
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    ** Standard (primitive) type codes.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_non_std_type,
// OBSOLETE     dst_int8_type,		/* 8 bit integer */
// OBSOLETE     dst_int16_type,		/* 16 bit integer */
// OBSOLETE     dst_int32_type,		/* 32 bit integer */
// OBSOLETE     dst_uint8_type,		/* 8 bit unsigned integer */
// OBSOLETE     dst_uint16_type,		/* 16 bit unsigned integer */
// OBSOLETE     dst_uint32_type,		/* 32 bit unsigned integer */
// OBSOLETE     dst_real32_type,		/* single precision ieee floatining point */
// OBSOLETE     dst_real64_type,		/* double precision ieee floatining point */
// OBSOLETE     dst_complex_type,		/* single precision complex */
// OBSOLETE     dst_dcomplex_type,		/* double precision complex */
// OBSOLETE     dst_bool8_type,		/* boolean =logical*1 */
// OBSOLETE     dst_bool16_type,		/* boolean =logical*2 */
// OBSOLETE     dst_bool32_type,		/* boolean =logical*4 */
// OBSOLETE     dst_char_type,		/* 8 bit ascii character */
// OBSOLETE     dst_string_type,		/* string of 8 bit ascii characters */
// OBSOLETE     dst_ptr_type,		/* univ_pointer */
// OBSOLETE     dst_set_type,		/* generic 256 bit set */
// OBSOLETE     dst_proc_type,		/* generic procedure (signature not specified) */
// OBSOLETE     dst_func_type,		/* generic function (signature not specified) */
// OBSOLETE     dst_void_type,		/* c void type */
// OBSOLETE     dst_uchar_type,		/* c unsigned char */
// OBSOLETE     dst_std_type_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_std_type_t;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    ** General data type descriptor
// OBSOLETE    **
// OBSOLETE    ** If the user_defined_type bit is clear, then the type is a standard type, and
// OBSOLETE    ** the remaining bits contain the dst_std_type_t of the type.  If the bit is
// OBSOLETE    ** set, then the type is defined in a separate dst record, which is referenced
// OBSOLETE    ** by the remaining bits as a dst_rel_offset_t.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE typedef union
// OBSOLETE   {
// OBSOLETE     struct
// OBSOLETE       {
// OBSOLETE 	boolean user_defined_type:1;	/* tag field */
// OBSOLETE 	int must_be_zero:23;	/* 23 bits of pad */
// OBSOLETE 	dst_std_type_t dtc:8;	/* 8 bit primitive data */
// OBSOLETE       }
// OBSOLETE     std_type;
// OBSOLETE 
// OBSOLETE     struct
// OBSOLETE       {
// OBSOLETE 	boolean user_defined_type:1;	/* tag field */
// OBSOLETE 	int doffset:31;		/* offset to type record */
// OBSOLETE       }
// OBSOLETE     user_type;
// OBSOLETE   }
// OBSOLETE dst_type_t ALIGNED1;
// OBSOLETE 
// OBSOLETE /* The user_type.doffset field is a 31-bit signed value.  Some versions of C
// OBSOLETE    do not support signed bit fields.  The following macro will extract that
// OBSOLETE    field as a signed value:
// OBSOLETE  */
// OBSOLETE #define dst_user_type_offset(type_rec) \
// OBSOLETE     ( ((int) ((type_rec).user_type.doffset << 1)) >> 1 )
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*================================================*/
// OBSOLETE /*========== RECORDS IN .blocks SECTION ==========*/
// OBSOLETE /*================================================*/
// OBSOLETE 
// OBSOLETE /*-----------------------
// OBSOLETE   COMPILATION UNIT record 
// OBSOLETE   -----------------------
// OBSOLETE   This must be the first record in each .blocks section.
// OBSOLETE   Provides a set of information describing the output of a single compilation
// OBSOLETE   and pointers to additional information for the compilation unit.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_pc_code_locs,		/* ranges in loc strings are pc ranges */
// OBSOLETE     dst_comp_unit_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_comp_unit_flag_t;
// OBSOLETE 
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_lang_unk,		/* unknown language */
// OBSOLETE     dst_lang_pas,		/* Pascal */
// OBSOLETE     dst_lang_ftn,		/* FORTRAN */
// OBSOLETE     dst_lang_c,			/* C */
// OBSOLETE     dst_lang_mod2,		/* Modula-2 */
// OBSOLETE     dst_lang_asm_m68k,		/* 68K assembly language */
// OBSOLETE     dst_lang_asm_a88k,		/* AT assembly language */
// OBSOLETE     dst_lang_ada,		/* Ada */
// OBSOLETE     dst_lang_cxx,		/* C++ */
// OBSOLETE     dst_lang_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_lang_type_t;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     struct
// OBSOLETE       {
// OBSOLETE 	unsigned char major_part;	/* = dst_version_major */
// OBSOLETE 	unsigned char minor_part;	/* = dst_version_minor */
// OBSOLETE       }
// OBSOLETE     version;			/* version of dst */
// OBSOLETE     unsigned short flags;	/* mask of dst_comp_unit_flag_t */
// OBSOLETE     unsigned short lang_type;	/* source language */
// OBSOLETE     unsigned short number_of_blocks;	/* number of blocks records */
// OBSOLETE     dst_rel_offset_t root_block_offset;		/* offset to root block (module?) */
// OBSOLETE     dst_rel_offset_t section_table /* offset to section table record */ ;
// OBSOLETE     dst_rel_offset_t file_table;	/* offset to file table record */
// OBSOLETE     unsigned long data_size;	/* total size of .blocks data */
// OBSOLETE   }
// OBSOLETE dst_rec_comp_unit_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*--------------------
// OBSOLETE   SECTION TABLE record
// OBSOLETE   --------------------
// OBSOLETE   There must be one section table associated with each compilation unit.
// OBSOLETE   Other debug records refer to sections via their index in this table.  The
// OBSOLETE   section base addresses in the table are virtual addresses of the sections,
// OBSOLETE   relocated by the linker.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short number_of_sections;	/* size of array: */
// OBSOLETE     unsigned long section_base[dst_dummy_array_size] ALIGNED1;
// OBSOLETE   }
// OBSOLETE dst_rec_section_tab_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------
// OBSOLETE   FILE TABLE record
// OBSOLETE   -----------------
// OBSOLETE   There must be one file table associated with each compilation unit describing
// OBSOLETE   the source (and include) files used by each compilation unit.  Other debug 
// OBSOLETE   records refer to files via their index in this table.  The first entry is the
// OBSOLETE   primary source file.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     long dtm;			/* time last modified (time_$clock_t) */
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string for source file */
// OBSOLETE   }
// OBSOLETE dst_file_desc_t;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short number_of_files;	/* size of array: */
// OBSOLETE     dst_file_desc_t files[dst_dummy_array_size] ALIGNED1;
// OBSOLETE   }
// OBSOLETE dst_rec_file_tab_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------
// OBSOLETE   NAME TABLE record
// OBSOLETE   -----------------
// OBSOLETE   A name table record may appear as an auxiliary record to the file table,
// OBSOLETE   providing additional qualification of the file indexes for languages that 
// OBSOLETE   need it (i.e. Ada).  Name table entries parallel file table entries of the
// OBSOLETE   same file index.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short number_of_names;	/* size of array: */
// OBSOLETE     dst_rel_offset_t names[dst_dummy_array_size] ALIGNED1;
// OBSOLETE   }
// OBSOLETE dst_rec_name_tab_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*--------------
// OBSOLETE   BLOCK record
// OBSOLETE   --------------
// OBSOLETE   Describes a lexical program block--a procedure, function, module, etc.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE /* Block types.  These may be used in any way desired by the compiler writers. 
// OBSOLETE    The debugger uses them only to give a description to the user of the type of
// OBSOLETE    a block.  The debugger makes no other assumptions about the meaning of any
// OBSOLETE    of these.  For example, the fact that a block is executable (e.g., program)
// OBSOLETE    or not (e.g., module) is expressed in block attributes (see below), not
// OBSOLETE    guessed at from the block type.
// OBSOLETE  */
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_block_module,		/* some pascal = modula = ada types */
// OBSOLETE     dst_block_program,
// OBSOLETE     dst_block_procedure,
// OBSOLETE     dst_block_function,		/* C function */
// OBSOLETE     dst_block_subroutine,	/* some fortran block types */
// OBSOLETE     dst_block_block_data,
// OBSOLETE     dst_block_stmt_function,
// OBSOLETE     dst_block_package,		/* a few particular to Ada */
// OBSOLETE     dst_block_package_body,
// OBSOLETE     dst_block_subunit,
// OBSOLETE     dst_block_task,
// OBSOLETE     dst_block_file,		/* a C outer scope? */
// OBSOLETE     dst_block_class,		/* C++ or Simula */
// OBSOLETE     dst_block_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_block_type_t;
// OBSOLETE 
// OBSOLETE /* Block attributes.  This is the information used by the debugger to represent
// OBSOLETE    the semantics of blocks.
// OBSOLETE  */
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_block_main_entry,	/* the block's entry point is a main entry into
// OBSOLETE 				   the compilation unit */
// OBSOLETE     dst_block_executable,	/* the block has an entry point */
// OBSOLETE     dst_block_attr_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_block_attr_t;
// OBSOLETE 
// OBSOLETE /* Code range.  Each block has associated with it one or more code ranges. An
// OBSOLETE    individual code range is identified by a range of source (possibly nil) and
// OBSOLETE    a range of executable code.  For example, a block which has its executable
// OBSOLETE    code spread over multiple sections will have one code range per section.
// OBSOLETE  */
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned long code_size;	/* size of executable code (in bytes ) */
// OBSOLETE     dst_sect_ref_t code_start;	/* starting address of executable code */
// OBSOLETE     dst_sect_ref_t lines_start;	/* start of line number tables */
// OBSOLETE   }
// OBSOLETE dst_code_range_t;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_block_type_t block_type:8;
// OBSOLETE     unsigned short flags:8;	/* mask of dst_block_attr_t flags */
// OBSOLETE     dst_rel_offset_t sibling_block_off;		/* offset to next sibling block */
// OBSOLETE     dst_rel_offset_t child_block_off;	/* offset to first contained block */
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to block name string */
// OBSOLETE     dst_sect_ref_t symbols_start;	/* start of debug symbols  */
// OBSOLETE     unsigned short n_of_code_ranges;	/* size of array... */
// OBSOLETE     dst_code_range_t code_ranges[dst_dummy_array_size] ALIGNED1;
// OBSOLETE   }
// OBSOLETE dst_rec_block_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*--------------------------
// OBSOLETE   AUX SECT INFO TABLE record
// OBSOLETE   --------------------------
// OBSOLETE   Appears as an auxiliary to a block record.  Expands code range information
// OBSOLETE   by providing references into additional, language-dependent sections for 
// OBSOLETE   information related to specific code ranges of the block.  Sect info table
// OBSOLETE   entries parallel code range array entries of the same index.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned char tag;		/* currently can only be zero */
// OBSOLETE     unsigned char number_of_refs;	/* size of array: */
// OBSOLETE     dst_sect_ref_t refs[dst_dummy_array_size] ALIGNED1;
// OBSOLETE   }
// OBSOLETE dst_rec_sect_info_tab_t ALIGNED1;
// OBSOLETE 
// OBSOLETE /*=================================================*/
// OBSOLETE /*========== RECORDS IN .symbols SECTION ==========*/
// OBSOLETE /*=================================================*/
// OBSOLETE 
// OBSOLETE /*-----------------
// OBSOLETE   CONSTANT record
// OBSOLETE   -----------------
// OBSOLETE   Describes a symbolic constant.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     float r;			/* real part */
// OBSOLETE     float i;			/* imaginary part */
// OBSOLETE   }
// OBSOLETE dst_complex_t;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     double dr;			/* real part */
// OBSOLETE     double di;			/* imaginary part */
// OBSOLETE   }
// OBSOLETE dst_double_complex_t;
// OBSOLETE 
// OBSOLETE /* The following record provides a way of describing constant values with 
// OBSOLETE    non-standard type and no limit on size. 
// OBSOLETE  */
// OBSOLETE typedef union
// OBSOLETE   {
// OBSOLETE     char char_data[dst_dummy_array_size];
// OBSOLETE     short int_data[dst_dummy_array_size];
// OBSOLETE     long long_data[dst_dummy_array_size];
// OBSOLETE   }
// OBSOLETE dst_big_kon_t;
// OBSOLETE 
// OBSOLETE /* Representation of the value of a general constant.
// OBSOLETE  */
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short length;	/* size of constant value (bytes) */
// OBSOLETE 
// OBSOLETE     union
// OBSOLETE       {
// OBSOLETE 	unsigned short kon_int8;
// OBSOLETE 	short kon_int16;
// OBSOLETE 	long kon_int32 ALIGNED1;
// OBSOLETE 	float kon_real ALIGNED1;
// OBSOLETE 	double kon_dbl ALIGNED1;
// OBSOLETE 	dst_complex_t kon_cplx ALIGNED1;
// OBSOLETE 	dst_double_complex_t kon_dcplx ALIGNED1;
// OBSOLETE 	char kon_char;
// OBSOLETE 	dst_big_kon_t kon ALIGNED1;
// OBSOLETE       }
// OBSOLETE     val;			/* value data of constant */
// OBSOLETE   }
// OBSOLETE dst_const_t ALIGNED1;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of const definition */
// OBSOLETE     dst_type_t type_desc;	/* type of this (manifest) constant */
// OBSOLETE     dst_const_t value;
// OBSOLETE   }
// OBSOLETE dst_rec_const_t ALIGNED1;
// OBSOLETE 
// OBSOLETE /*----------------
// OBSOLETE   VARIABLE record
// OBSOLETE   ----------------
// OBSOLETE   Describes a program variable.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE /* Variable attributes.  These define certain variable semantics to the
// OBSOLETE    debugger.
// OBSOLETE  */
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_var_attr_read_only,	/* is read-only (a program literal) */
// OBSOLETE     dst_var_attr_volatile,	/* same as compiler's VOLATILE attribute */
// OBSOLETE     dst_var_attr_global,	/* is a global definition or reference */
// OBSOLETE     dst_var_attr_compiler_gen,	/* is compiler-generated */
// OBSOLETE     dst_var_attr_static,	/* has static location */
// OBSOLETE     dst_var_attr_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_var_attr_t;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_rel_offset_t loffset;	/* offset to loc string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of variable definition */
// OBSOLETE     dst_type_t type_desc;	/* type descriptor */
// OBSOLETE     unsigned short attributes;	/* mask of dst_var_attr_t flags */
// OBSOLETE   }
// OBSOLETE dst_rec_variable_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*----------------
// OBSOLETE   old VAR record
// OBSOLETE  -----------------
// OBSOLETE  Used by older compilers to describe a variable
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_var_loc_unknown,	/* Actually defined as "unknown" */
// OBSOLETE     dst_var_loc_abs,		/* Absolute address */
// OBSOLETE     dst_var_loc_sect_off,	/* Absolute address as a section offset */
// OBSOLETE     dst_var_loc_ind_sect_off,	/* An indexed section offset ???? */
// OBSOLETE     dst_var_loc_reg,		/* register */
// OBSOLETE     dst_var_loc_reg_rel,	/* register relative - usually fp */
// OBSOLETE     dst_var_loc_ind_reg_rel,	/* Indexed register relative */
// OBSOLETE     dst_var_loc_ftn_ptr_based,	/* Fortran pointer based */
// OBSOLETE     dst_var_loc_pc_rel,		/* PC relative. Really. */
// OBSOLETE     dst_var_loc_external,	/* External */
// OBSOLETE     dst_var_loc_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_var_loc_t;
// OBSOLETE 
// OBSOLETE /* Locations come in two versions. The short, and the long. The difference
// OBSOLETE  * between the short and the long is the addition of a statement number
// OBSOLETE  * field to the start andend of the range of the long, and and unkown
// OBSOLETE  * purpose field in the middle. Also, loc_type and loc_index aren't
// OBSOLETE  * bitfields in the long version.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short loc_type:4;
// OBSOLETE     unsigned short loc_index:12;
// OBSOLETE     long location;
// OBSOLETE     short start_line;		/* start_line and end_line? */
// OBSOLETE     short end_line;		/* I'm guessing here.       */
// OBSOLETE   }
// OBSOLETE dst_var_loc_short_t;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short loc_type;
// OBSOLETE     unsigned short loc_index;
// OBSOLETE     long location;
// OBSOLETE     short unknown;		/* Always 0003 or 3b3c. Why? */
// OBSOLETE     short start_statement;
// OBSOLETE     short start_line;
// OBSOLETE     short end_statement;
// OBSOLETE     short end_line;
// OBSOLETE   }
// OBSOLETE dst_var_loc_long_t;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of description */
// OBSOLETE     dst_type_t type_desc;	/* Type description */
// OBSOLETE     unsigned short attributes;	/* mask of dst_var_attr_t flags */
// OBSOLETE     unsigned short no_of_locs:15;	/* Number of locations */
// OBSOLETE     unsigned short short_locs:1;	/* True if short locations. */
// OBSOLETE     union
// OBSOLETE       {
// OBSOLETE 	dst_var_loc_short_t shorts[dst_dummy_array_size];
// OBSOLETE 	dst_var_loc_long_t longs[dst_dummy_array_size];
// OBSOLETE       }
// OBSOLETE     locs;
// OBSOLETE   }
// OBSOLETE dst_rec_var_t;
// OBSOLETE 
// OBSOLETE /*----------------
// OBSOLETE   old LABEL record
// OBSOLETE  -----------------
// OBSOLETE  Used by older compilers to describe a label
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of description */
// OBSOLETE     char location[12];		/* location string */
// OBSOLETE   }
// OBSOLETE dst_rec_old_label_t ALIGNED1;
// OBSOLETE 
// OBSOLETE /*----------------
// OBSOLETE   POINTER record
// OBSOLETE   ----------------
// OBSOLETE   Describes a pointer type.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to the name string for this type */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of definition */
// OBSOLETE     dst_type_t type_desc;	/* base type of this pointer */
// OBSOLETE   }
// OBSOLETE dst_rec_pointer_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-------------
// OBSOLETE   ARRAY record
// OBSOLETE   -------------
// OBSOLETE   Describes an array type.
// OBSOLETE 
// OBSOLETE   Multidimensional arrays are described with a number of dst_rec_array_t 
// OBSOLETE   records, one per array dimension, each linked to the next through the
// OBSOLETE   elem_type_desc.doffset field.  Each record must have its multi_dim flag
// OBSOLETE   set.
// OBSOLETE 
// OBSOLETE   If column_major is true (as with FORTRAN arrays) then the last array bound in
// OBSOLETE   the declaration is the first array index in memory, which is the opposite of
// OBSOLETE   the usual case (as with Pascal and C arrays).
// OBSOLETE 
// OBSOLETE   Variable array bounds are described by auxiliary records; if aux_var_bound
// OBSOLETE   records are present, the lo_bound and hi_bound fields of this record are
// OBSOLETE   ignored by the debugger.
// OBSOLETE 
// OBSOLETE   span_comp identifies one of the language-dependent ways in which the distance
// OBSOLETE   between successive array elements (span) is calculated.  
// OBSOLETE      dst_use_span_field    -- the span is the value of span field.
// OBSOLETE      dst_compute_from_prev -- the span is the size of the previous dimension.
// OBSOLETE      dst_compute_from_next -- the span is the size of the next dimension.
// OBSOLETE   In the latter two cases, the span field contains an amount of padding to add
// OBSOLETE   to the size of the appropriate dimension to calculate the span.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_use_span_field,
// OBSOLETE     dst_compute_from_prev,
// OBSOLETE     dst_compute_from_next,
// OBSOLETE     dst_span_comp_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_span_comp_t;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of definition */
// OBSOLETE     dst_type_t elem_type_desc;	/* array element type */
// OBSOLETE     dst_type_t indx_type_desc;	/* array index type */
// OBSOLETE     long lo_bound;		/* lower bound of index */
// OBSOLETE     long hi_bound;		/* upper bound of index */
// OBSOLETE     unsigned long span;		/* see above */
// OBSOLETE     unsigned long size;		/* total array size (bytes) */
// OBSOLETE     boolean multi_dim:1;
// OBSOLETE     boolean is_packed:1;	/* true if packed array */
// OBSOLETE     boolean is_signed:1;	/* true if packed elements are signed */
// OBSOLETE     dst_span_comp_t span_comp:2;	/* how to compute span */
// OBSOLETE     boolean column_major:1;
// OBSOLETE     unsigned short reserved:2;	/* must be zero */
// OBSOLETE     unsigned short elem_size:8;	/* element size if packed (bits) */
// OBSOLETE   }
// OBSOLETE dst_rec_array_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------
// OBSOLETE   SUBRANGE record
// OBSOLETE   -----------------
// OBSOLETE   Describes a subrange type.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE /* Variable subrange bounds are described by auxiliary records; if aux_var_bound
// OBSOLETE    records are present, the lo_bound and hi_bound fields of this record are
// OBSOLETE    ignored by the debugger.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of subrange definition */
// OBSOLETE     dst_type_t type_desc;	/* parent type */
// OBSOLETE     long lo_bound;		/* lower bound of subrange */
// OBSOLETE     long hi_bound;		/* upper bound of subrange */
// OBSOLETE     unsigned short size;	/* storage size (bytes) */
// OBSOLETE   }
// OBSOLETE dst_rec_subrange_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*---------------
// OBSOLETE   STRING record 
// OBSOLETE   ---------------
// OBSOLETE   Describes a string type.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE /* Variable subrange bounds are described by auxiliary records; if aux_var_bound
// OBSOLETE    records are present, the lo_bound and hi_bound fields of this record are
// OBSOLETE    ignored by the debugger.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of string definition */
// OBSOLETE     dst_type_t elem_type_desc;	/* element type */
// OBSOLETE     dst_type_t indx_type_desc;	/* index type */
// OBSOLETE     long lo_bound;		/* lower bound */
// OBSOLETE     long hi_bound;		/* upper bound */
// OBSOLETE     unsigned long size;		/* total string size (bytes) if fixed */
// OBSOLETE   }
// OBSOLETE dst_rec_string_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*---------------
// OBSOLETE   SET record 
// OBSOLETE   ---------------
// OBSOLETE   Describes a set type.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of definition */
// OBSOLETE     dst_type_t type_desc;	/* element type */
// OBSOLETE     unsigned short nbits;	/* number of bits in set */
// OBSOLETE     unsigned short size;	/* storage size (bytes) */
// OBSOLETE   }
// OBSOLETE dst_rec_set_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------------------
// OBSOLETE   IMPLICIT ENUMERATION record 
// OBSOLETE   -----------------------------
// OBSOLETE   Describes an enumeration type with implicit element values = 0, 1, 2, ...
// OBSOLETE   (Pascal-style).
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of definition */
// OBSOLETE     unsigned short nelems;	/* number of elements in enumeration */
// OBSOLETE     unsigned short size;	/* storage size (bytes) */
// OBSOLETE     /* offsets to name strings of elements 0, 1, 2, ... */
// OBSOLETE     dst_rel_offset_t elem_noffsets[dst_dummy_array_size];
// OBSOLETE   }
// OBSOLETE dst_rec_implicit_enum_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------------------
// OBSOLETE   EXPLICIT ENUMERATION record 
// OBSOLETE   -----------------------------
// OBSOLETE   Describes an enumeration type with explicitly assigned element values
// OBSOLETE   (C-style).
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to element name string */
// OBSOLETE     long value;			/* element value */
// OBSOLETE   }
// OBSOLETE dst_enum_elem_t;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of definition */
// OBSOLETE     unsigned short nelems;	/* number of elements in enumeration */
// OBSOLETE     unsigned short size;	/* storage size (bytes) */
// OBSOLETE     /* name/value pairs, one describing each enumeration value: */
// OBSOLETE     dst_enum_elem_t elems[dst_dummy_array_size];
// OBSOLETE   }
// OBSOLETE dst_rec_explicit_enum_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------------
// OBSOLETE   RECORD / UNION record 
// OBSOLETE   -----------------------
// OBSOLETE   Describes a record (struct) or union.
// OBSOLETE 
// OBSOLETE   If the record is larger than 2**16 bytes then an attached aux record
// OBSOLETE   specifies its size.  Also, if the record is stored in short form then
// OBSOLETE   attached records specify field offsets larger than 2**16 bytes.
// OBSOLETE 
// OBSOLETE   Whether the fields[] array or sfields[] array is used is selected by
// OBSOLETE   the dst_rec_type_t of the overall dst record.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    Record field descriptor, short form.  This form handles only fields which
// OBSOLETE    are an even number of bytes long, located some number of bytes from the
// OBSOLETE    start of the record.
// OBSOLETE  */
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to field name string */
// OBSOLETE     dst_type_t type_desc;	/* field type */
// OBSOLETE     unsigned short foffset;	/* field offset from start of record (bytes) */
// OBSOLETE   }
// OBSOLETE dst_short_field_t ALIGNED1;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_type_t type_desc;	/* field type */
// OBSOLETE     unsigned short foffset;	/* byte offset */
// OBSOLETE     unsigned short is_packed:1;	/* True if field is packed */
// OBSOLETE     unsigned short bit_offset:6;	/* Bit offset */
// OBSOLETE     unsigned short size:6;	/* Size in bits */
// OBSOLETE     unsigned short sign:1;	/* True if signed */
// OBSOLETE     unsigned short pad:2;	/* Padding. Must be 0 */
// OBSOLETE   }
// OBSOLETE dst_old_field_t ALIGNED1;
// OBSOLETE 
// OBSOLETE /* Tag enumeration for long record field descriptor:
// OBSOLETE  */
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_field_byte,
// OBSOLETE     dst_field_bit,
// OBSOLETE     dst_field_loc,
// OBSOLETE     dst_field_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_field_format_t;
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    Record field descriptor, long form.  The format of the field information
// OBSOLETE    is identified by the format_tag, which contains one of the above values.
// OBSOLETE    The field_byte variant is equivalent to the short form of field descriptor.
// OBSOLETE    The field_bit variant handles fields which are any number of bits long,
// OBSOLETE    located some number of bits from the start of the record.  The field_loc
// OBSOLETE    variant allows the location of the field to be described by a general loc
// OBSOLETE    string.
// OBSOLETE  */
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name of field */
// OBSOLETE     dst_type_t type_desc;	/* type of field */
// OBSOLETE     union
// OBSOLETE       {
// OBSOLETE 	struct
// OBSOLETE 	  {
// OBSOLETE 	    dst_field_format_t format_tag:2;	/* dst_field_byte */
// OBSOLETE 	    unsigned long offset:30;	/* offset of field in bytes */
// OBSOLETE 	  }
// OBSOLETE 	field_byte ALIGNED1;
// OBSOLETE 	struct
// OBSOLETE 	  {
// OBSOLETE 	    dst_field_format_t format_tag:2;	/* dst_field_bit */
// OBSOLETE 	    unsigned long nbits:6;	/* bit size of field */
// OBSOLETE 	    unsigned long is_signed:1;	/* signed/unsigned attribute */
// OBSOLETE 	    unsigned long bit_offset:3;		/* bit offset from byte boundary */
// OBSOLETE 	    int pad:4;		/* must be zero */
// OBSOLETE 	    unsigned short byte_offset;		/* offset of byte boundary */
// OBSOLETE 	  }
// OBSOLETE 	field_bit ALIGNED1;
// OBSOLETE 	struct
// OBSOLETE 	  {
// OBSOLETE 	    dst_field_format_t format_tag:2;	/* dst_field_loc */
// OBSOLETE 	    int loffset:30;	/* dst_rel_offset_t to loc string */
// OBSOLETE 	  }
// OBSOLETE 	field_loc ALIGNED1;
// OBSOLETE       }
// OBSOLETE     f ALIGNED1;
// OBSOLETE   }
// OBSOLETE dst_field_t;
// OBSOLETE 
// OBSOLETE /* The field_loc.loffset field is a 30-bit signed value.  Some versions of C do
// OBSOLETE    not support signed bit fields.  The following macro will extract that field
// OBSOLETE    as a signed value:
// OBSOLETE  */
// OBSOLETE #define dst_field_loffset(field_rec) \
// OBSOLETE     ( ((int) ((field_rec).f.field_loc.loffset << 2)) >> 2 )
// OBSOLETE 
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to record name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line where this record is defined */
// OBSOLETE     unsigned short size;	/* storage size (bytes) */
// OBSOLETE     unsigned short nfields;	/* number of fields in this record */
// OBSOLETE     union
// OBSOLETE       {
// OBSOLETE 	dst_field_t fields[dst_dummy_array_size];
// OBSOLETE 	dst_short_field_t sfields[dst_dummy_array_size];
// OBSOLETE 	dst_old_field_t ofields[dst_dummy_array_size];
// OBSOLETE       }
// OBSOLETE     f;				/* array of fields */
// OBSOLETE   }
// OBSOLETE dst_rec_record_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-------------
// OBSOLETE   FILE record
// OBSOLETE   -------------
// OBSOLETE   Describes a file type.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line where type was defined */
// OBSOLETE     dst_type_t type_desc;	/* file element type */
// OBSOLETE   }
// OBSOLETE dst_rec_file_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*---------------
// OBSOLETE   OFFSET record 
// OBSOLETE   ---------------
// OBSOLETE    Describes a Pascal offset type.
// OBSOLETE    (This type, an undocumented Domain Pascal extension, is currently not
// OBSOLETE    supported by the debugger)
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to the name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of definition */
// OBSOLETE     dst_type_t area_type_desc;	/* area type */
// OBSOLETE     dst_type_t base_type_desc;	/* base type */
// OBSOLETE     long lo_bound;		/* low bound of the offset range */
// OBSOLETE     long hi_bound;		/* high bound of the offset range */
// OBSOLETE     long bias;			/* bias */
// OBSOLETE     unsigned short scale;	/* scale factor */
// OBSOLETE     unsigned short size;	/* storage size (bytes) */
// OBSOLETE   }
// OBSOLETE dst_rec_offset_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*--------------
// OBSOLETE   ALIAS record 
// OBSOLETE   --------------
// OBSOLETE   Describes a type alias (e.g., typedef).
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of definition */
// OBSOLETE     dst_type_t type_desc;	/* parent type */
// OBSOLETE   }
// OBSOLETE dst_rec_alias_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*------------------
// OBSOLETE   SIGNATURE record
// OBSOLETE   ------------------
// OBSOLETE   Describes a procedure/function type.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE /* Enumeration of argument semantics.  Note that most are mutually
// OBSOLETE    exclusive.
// OBSOLETE  */
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_arg_attr_val,		/* passed by value */
// OBSOLETE     dst_arg_attr_ref,		/* passed by reference */
// OBSOLETE     dst_arg_attr_name,		/* passed by name */
// OBSOLETE     dst_arg_attr_in,		/* readable in the callee */
// OBSOLETE     dst_arg_attr_out,		/* writable in the callee */
// OBSOLETE     dst_arg_attr_hidden,	/* not visible in the caller */
// OBSOLETE     dst_arg_attr_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_arg_attr_t;
// OBSOLETE 
// OBSOLETE /* Argument descriptor.  Actually points to a variable record for most of the
// OBSOLETE    information.
// OBSOLETE  */
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t var_offset;	/* offset to variable record */
// OBSOLETE     unsigned short attributes;	/* a mask of dst_arg_attr_t flags */
// OBSOLETE   }
// OBSOLETE dst_arg_t ALIGNED1;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of function definition */
// OBSOLETE     dst_rel_offset_t result;	/* offset to function result variable record */
// OBSOLETE     unsigned short nargs;	/* number of arguments */
// OBSOLETE     dst_arg_t args[dst_dummy_array_size];
// OBSOLETE   }
// OBSOLETE dst_rec_signature_t ALIGNED1;
// OBSOLETE 
// OBSOLETE /*--------------
// OBSOLETE   SCOPE record
// OBSOLETE   --------------
// OBSOLETE   Obsolete. Use the new ENTRY type instead.
// OBSOLETE   Old compilers may put this in as the first entry in a function,
// OBSOLETE   terminated by an end of scope entry.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* Name offset */
// OBSOLETE     dst_src_loc_t start_line;	/* Starting line */
// OBSOLETE     dst_src_loc_t end_line;	/* Ending line */
// OBSOLETE   }
// OBSOLETE dst_rec_scope_t ALIGNED1;
// OBSOLETE 
// OBSOLETE /*--------------
// OBSOLETE   ENTRY record
// OBSOLETE   --------------
// OBSOLETE   Describes a procedure/function entry point.  An entry record is to a
// OBSOLETE   signature record roughly as a variable record is to a type descriptor record.
// OBSOLETE 
// OBSOLETE   The entry_number field is keyed to the entry numbers in .lines -- the 
// OBSOLETE   debugger locates the code location of an entry by searching the line
// OBSOLETE   number table for an entry numbered with the value of entry_number.  The
// OBSOLETE   main entry is numbered zero.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to entry name string */
// OBSOLETE     dst_rel_offset_t loffset;	/* where to jump to call this entry */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of definition */
// OBSOLETE     dst_rel_offset_t sig_desc;	/* offset to signature descriptor */
// OBSOLETE     unsigned int entry_number:8;
// OBSOLETE     int pad:8;			/* must be zero */
// OBSOLETE   }
// OBSOLETE dst_rec_entry_t ALIGNED1;
// OBSOLETE 
// OBSOLETE /*-----------------------
// OBSOLETE   Old format ENTRY record
// OBSOLETE   -----------------------
// OBSOLETE   Supposedly obsolete but still used by some compilers.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* Offset to entry name string */
// OBSOLETE     dst_src_loc_t src_loc;	/* Location in source */
// OBSOLETE     dst_rel_offset_t sig_desc;	/* Signature description */
// OBSOLETE     char unknown[36];
// OBSOLETE   }
// OBSOLETE dst_rec_old_entry_t ALIGNED1;
// OBSOLETE 
// OBSOLETE /*--------------
// OBSOLETE   LABEL record 
// OBSOLETE   --------------
// OBSOLETE   Describes a program label.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t noffset;	/* offset to label string */
// OBSOLETE     dst_rel_offset_t loffset;	/* offset to loc string */
// OBSOLETE     dst_src_loc_t src_loc;	/* file/line of definition */
// OBSOLETE   }
// OBSOLETE dst_rec_label_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------------
// OBSOLETE   AUXILIARY SIZE record
// OBSOLETE   -----------------------
// OBSOLETE   May appear in the auxiliary record list of any type or variable record to
// OBSOLETE   modify the default size of the type or variable.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned long size;		/* size (bytes) */
// OBSOLETE   }
// OBSOLETE dst_rec_aux_size_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------------
// OBSOLETE   AUXILIARY ALIGN record
// OBSOLETE   -----------------------
// OBSOLETE   May appear in the auxiliary record list of any type or variable record to
// OBSOLETE   modify the default alignment of the type or variable.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short alignment;	/* # of low order zero bits */
// OBSOLETE   }
// OBSOLETE dst_rec_aux_align_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------------------
// OBSOLETE   AUXILIARY FIELD SIZE record
// OBSOLETE   -----------------------------
// OBSOLETE   May appear in the auxiliary record list of any RECORD/UNION record to 
// OBSOLETE   modify the default size of a field.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short field_no;	/* field number */
// OBSOLETE     unsigned long size;		/* size (bits) */
// OBSOLETE   }
// OBSOLETE dst_rec_aux_field_size_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------------------
// OBSOLETE   AUXILIARY FIELD OFFSET record
// OBSOLETE   -----------------------------
// OBSOLETE   May appear in the auxiliary record list of any RECORD/UNION record to 
// OBSOLETE   specify a field offset larger than 2**16.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short field_no;	/* field number */
// OBSOLETE     unsigned long foffset;	/* offset */
// OBSOLETE   }
// OBSOLETE dst_rec_aux_field_off_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------------------
// OBSOLETE   AUXILIARY FIELD ALIGN record
// OBSOLETE   -----------------------------
// OBSOLETE   May appear in the auxiliary record list of any RECORD/UNION record to 
// OBSOLETE   modify the default alignment of a field.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short field_no;	/* field number */
// OBSOLETE     unsigned short alignment;	/* number of low order zero bits */
// OBSOLETE   }
// OBSOLETE dst_rec_aux_field_align_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*----------------------------
// OBSOLETE   AUXILIARY VAR BOUND record
// OBSOLETE   ----------------------------
// OBSOLETE   May appear in the auxiliary record list of any ARRAY, SUBRANGE or STRING
// OBSOLETE   record to describe a variable bound for the range of the type.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_low_bound,		/* the low bound is variable */
// OBSOLETE     dst_high_bound,		/* the high bound is variable */
// OBSOLETE     dst_var_bound_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_var_bound_t;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short which;	/* which bound */
// OBSOLETE     dst_rel_offset_t voffset ALIGNED1;	/* variable that defines bound */
// OBSOLETE   }
// OBSOLETE dst_rec_aux_var_bound_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*----------------------------------
// OBSOLETE   AUXILIARY TYPE DERIVATION record 
// OBSOLETE   ----------------------------------
// OBSOLETE   May appear in the auxiliary record list of any RECORD/UNION record to denote
// OBSOLETE   class inheritance of that type from a parent type.
// OBSOLETE 
// OBSOLETE   Inheritance implies that it is possible to convert the inheritor type to the
// OBSOLETE   inherited type, retaining those fields which were inherited.  To allow this,
// OBSOLETE   orig_field_no, a field number into the record type, is provided.  If 
// OBSOLETE   orig_is_pointer is false, then the start of the inherited record is located
// OBSOLETE   at the location of the field indexed by orig_field_no.  If orig_is_pointer
// OBSOLETE   is true, then it is located at the address contained in the field indexed
// OBSOLETE   by orig_field_no (assumed to be a pointer).
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_type_t parent_type;	/* reference to inherited type */
// OBSOLETE     unsigned short orig_field_no;
// OBSOLETE     boolean orig_is_pointer:1;
// OBSOLETE     int unused:15;		/* must be zero */
// OBSOLETE   }
// OBSOLETE dst_rec_aux_type_deriv_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*------------------------------------
// OBSOLETE   AUXILIARY VARIABLE LIFETIME record
// OBSOLETE   ------------------------------------
// OBSOLETE   May appear in the auxiliary record list of a VARIABLE record to add location
// OBSOLETE   information for an additional variable lifetime.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t loffset;
// OBSOLETE   }
// OBSOLETE dst_rec_aux_lifetime_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-------------------------------
// OBSOLETE   AUXILIARY POINTER BASE record 
// OBSOLETE   -------------------------------
// OBSOLETE   May appear in the auxiliary record list of a VARIABLE record to provide a
// OBSOLETE   pointer base to substitute for references to any such bases in the location
// OBSOLETE   string of the variable.  A pointer base is another VARIABLE record.  When
// OBSOLETE   the variable is evaluated by the debugger, it uses the current value of the
// OBSOLETE   pointer base variable in computing its location.
// OBSOLETE 
// OBSOLETE   This is useful for representing FORTRAN pointer-based variables.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t voffset;
// OBSOLETE   }
// OBSOLETE dst_rec_aux_ptr_base_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*---------------------------------
// OBSOLETE   AUXILIARY REGISTER VALUE record 
// OBSOLETE   ---------------------------------
// OBSOLETE   May appear in the auxiliary record list of an ENTRY record to specify
// OBSOLETE   a register that must be set to a specific value before jumping to the entry
// OBSOLETE   point in a debugger "call".  The debugger must set the debuggee register,
// OBSOLETE   specified by the register code, to the value of the *address* to which the
// OBSOLETE   location string resolves.  If the address is register-relative, then the
// OBSOLETE   call cannot be made unless the current stack frame is the lexical parent
// OBSOLETE   of the entry.  An example of this is when a (Pascal) nested procedure
// OBSOLETE   contains references to its parent's variables, which it accesses through
// OBSOLETE   a static link register.  The static link register must be set to some
// OBSOLETE   address relative to the parent's stack base register.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short reg;		/* identifies register to set (isp enum) */
// OBSOLETE     dst_rel_offset_t loffset;	/* references a location string */
// OBSOLETE   }
// OBSOLETE dst_rec_aux_reg_val_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*==========================================================*/
// OBSOLETE /*========== RECORDS USED IN .blocks AND .symbols ==========*/
// OBSOLETE /*==========================================================*/
// OBSOLETE 
// OBSOLETE /*---------------------
// OBSOLETE   STRING TABLE record
// OBSOLETE   ---------------------
// OBSOLETE   A string table record contains any number of null-terminated, variable length
// OBSOLETE   strings.   The length field gives the size in bytes of the text field, which
// OBSOLETE   can be any size.
// OBSOLETE 
// OBSOLETE   The global name table shares this format.  This record appears in the
// OBSOLETE   .blocks section.  Each string in the table identifies a global defined in
// OBSOLETE   the current compilation unit.
// OBSOLETE 
// OBSOLETE   The loc pool record shares this format as well.  Loc strings are described
// OBSOLETE   elsewhere.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned long length;
// OBSOLETE     char text[dst_dummy_array_size];
// OBSOLETE   }
// OBSOLETE dst_rec_string_tab_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-----------------------
// OBSOLETE   AUXILIARY QUAL record 
// OBSOLETE   -----------------------
// OBSOLETE   May appear in the auxiliary record list of any BLOCK, VARIABLE, or type record
// OBSOLETE   to provide it with a fully-qualified, language-dependent name.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t lang_qual_name;
// OBSOLETE   }
// OBSOLETE dst_rec_aux_qual_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*----------------
// OBSOLETE   FORWARD record
// OBSOLETE   ----------------
// OBSOLETE   Reference to a record somewhere else.  This allows identical definitions in
// OBSOLETE   different scopes to share data.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rel_offset_t rec_off;
// OBSOLETE   }
// OBSOLETE dst_rec_forward_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*-------------------------------
// OBSOLETE   AUXILIARY SOURCE RANGE record
// OBSOLETE   -------------------------------
// OBSOLETE   May appear in the auxiliary record list of any BLOCK record to specify a
// OBSOLETE   range of source lines over which the block is active.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_src_loc_t first_line;	/* first source line */
// OBSOLETE     dst_src_loc_t last_line;	/* last source line */
// OBSOLETE   }
// OBSOLETE dst_rec_aux_src_range_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*------------------
// OBSOLETE   EXTENSION record 
// OBSOLETE   ------------------
// OBSOLETE   Provision for "foreign" records, such as might be generated by a non-Apollo
// OBSOLETE   compiler.  Apollo software will ignore these.
// OBSOLETE */
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     unsigned short rec_size;	/* record size (bytes) */
// OBSOLETE     unsigned short ext_type;	/* defined by whoever generates it */
// OBSOLETE     unsigned short ext_data;	/* place-holder for arbitrary amount of data */
// OBSOLETE   }
// OBSOLETE dst_rec_extension_t ALIGNED1;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    ** DEBUG SYMBOL record -- The wrapper for all .blocks and .symbols records.
// OBSOLETE    **
// OBSOLETE    ** This record ties together all previous .blocks and .symbols records 
// OBSOLETE    ** together in a union with a common header.  The rec_type field of the
// OBSOLETE    ** header identifies the record type.  The rec_flags field currently only
// OBSOLETE    ** defines auxiliary record lists. 
// OBSOLETE    **
// OBSOLETE    ** If a record carries with it a non-null auxiliary record list, its
// OBSOLETE    ** dst_flag_has_aux_recs flag is set, and each of the records that follow
// OBSOLETE    ** it are treated as its auxiliary records, until the end of the compilation
// OBSOLETE    ** unit or scope is reached, or until an auxiliary record with its
// OBSOLETE    ** dst_flag_last_aux_rec flag set is reached.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_flag_has_aux_recs,
// OBSOLETE     dst_flag_last_aux_rec,
// OBSOLETE     dst_rec_flag_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_rec_flags_t;
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     dst_rec_type_t rec_type:8;	/* record type */
// OBSOLETE     int rec_flags:8;		/* mask of dst_rec_flags_t */
// OBSOLETE     union			/* switched on rec_type field above */
// OBSOLETE       {
// OBSOLETE 	/* dst_typ_pad requires no additional fields */
// OBSOLETE 	dst_rec_comp_unit_t comp_unit_;
// OBSOLETE 	dst_rec_section_tab_t section_tab_;
// OBSOLETE 	dst_rec_file_tab_t file_tab_;
// OBSOLETE 	dst_rec_block_t block_;
// OBSOLETE 	dst_rec_var_t var_;
// OBSOLETE 	dst_rec_pointer_t pointer_;
// OBSOLETE 	dst_rec_array_t array_;
// OBSOLETE 	dst_rec_subrange_t subrange_;
// OBSOLETE 	dst_rec_set_t set_;
// OBSOLETE 	dst_rec_implicit_enum_t implicit_enum_;
// OBSOLETE 	dst_rec_explicit_enum_t explicit_enum_;
// OBSOLETE 	/* dst_typ_short_{rec,union} are represented by 'rec' (below) */
// OBSOLETE 	dst_rec_file_t file_;
// OBSOLETE 	dst_rec_offset_t offset_;
// OBSOLETE 	dst_rec_alias_t alias_;
// OBSOLETE 	dst_rec_signature_t signature_;
// OBSOLETE 	dst_rec_old_label_t old_label_;
// OBSOLETE 	dst_rec_scope_t scope_;
// OBSOLETE 	/* dst_typ_end_scope requires no additional fields */
// OBSOLETE 	dst_rec_string_tab_t string_tab_;
// OBSOLETE 	/* dst_typ_global_name_tab is represented by 'string_tab' (above) */
// OBSOLETE 	dst_rec_forward_t forward_;
// OBSOLETE 	dst_rec_aux_size_t aux_size_;
// OBSOLETE 	dst_rec_aux_align_t aux_align_;
// OBSOLETE 	dst_rec_aux_field_size_t aux_field_size_;
// OBSOLETE 	dst_rec_aux_field_off_t aux_field_off_;
// OBSOLETE 	dst_rec_aux_field_align_t aux_field_align_;
// OBSOLETE 	dst_rec_aux_qual_t aux_qual_;
// OBSOLETE 	dst_rec_aux_var_bound_t aux_var_bound_;
// OBSOLETE 	dst_rec_extension_t extension_;
// OBSOLETE 	dst_rec_string_t string_;
// OBSOLETE 	dst_rec_const_t const_;
// OBSOLETE 	/* dst_typ_reference is represented by 'pointer' (above) */
// OBSOLETE 	dst_rec_record_t record_;
// OBSOLETE 	/* dst_typ_union is represented by 'record' (above) */
// OBSOLETE 	dst_rec_aux_type_deriv_t aux_type_deriv_;
// OBSOLETE 	/* dst_typ_locpool is represented by 'string_tab' (above) */
// OBSOLETE 	dst_rec_variable_t variable_;
// OBSOLETE 	dst_rec_label_t label_;
// OBSOLETE 	dst_rec_entry_t entry_;
// OBSOLETE 	dst_rec_aux_lifetime_t aux_lifetime_;
// OBSOLETE 	dst_rec_aux_ptr_base_t aux_ptr_base_;
// OBSOLETE 	dst_rec_aux_src_range_t aux_src_range_;
// OBSOLETE 	dst_rec_aux_reg_val_t aux_reg_val_;
// OBSOLETE 	dst_rec_name_tab_t aux_unit_names_;
// OBSOLETE 	dst_rec_sect_info_tab_t aux_sect_info_;
// OBSOLETE       }
// OBSOLETE     rec_data ALIGNED1;
// OBSOLETE   }
// OBSOLETE dst_rec_t, *dst_rec_ptr_t;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*===============================================*/
// OBSOLETE /*========== .lines SECTION DEFINITIONS =========*/
// OBSOLETE /*===============================================*/
// OBSOLETE /*
// OBSOLETE    The .lines section contains a sequence of line number tables.  There is no
// OBSOLETE    record structure within the section.  The start of the table for a routine
// OBSOLETE    is pointed to by the block record, and the end of the table is signaled by
// OBSOLETE    an escape code.
// OBSOLETE 
// OBSOLETE    A line number table is a sequence of bytes.  The default entry contains a line
// OBSOLETE    number delta (-7..+7) in the high 4 bits and a pc delta (0..15) in the low 4 
// OBSOLETE    bits. Special cases, including when one or both of the values is too large
// OBSOLETE    to fit in 4 bits and other special cases are handled through escape entries.
// OBSOLETE    Escape entries are identified by the value 0x8 in the high 4 bits.  The low 4
// OBSOLETE    bits are occupied by a function code.  Some escape entries are followed by
// OBSOLETE    additional arguments, which may be bytes, words, or longwords.  This data is
// OBSOLETE    not aligned. 
// OBSOLETE 
// OBSOLETE    The initial PC offset, file number and line number are zero.  Normally, the
// OBSOLETE    table begins with a dst_ln_file escape which establishes the initial file
// OBSOLETE    and line number.  All PC deltas are unsigned (thus the table is ordered by
// OBSOLETE    increasing PC); line number deltas are signed.  The table ends with a 
// OBSOLETE    dst_ln_end escape, which is followed by a final table entry whose PC delta
// OBSOLETE    gives the code size of the last statement.
// OBSOLETE 
// OBSOLETE    Escape     Semantic
// OBSOLETE    ---------  ------------------------------------------------------------
// OBSOLETE    file       Changes file state.  The current source file remains constant
// OBSOLETE    until another file escape.  Though the line number state is
// OBSOLETE    also updated by a file escape, a file escape does NOT 
// OBSOLETE    constitute a line table entry.
// OBSOLETE 
// OBSOLETE    statement  Alters the statement number of the next table entry.  By 
// OBSOLETE    default, all table entries refer to the first statement on a
// OBSOLETE    line.  Statement number one is the second statement, and so on.
// OBSOLETE 
// OBSOLETE    entry      Identifies the next table entry as the position of an entry 
// OBSOLETE    point for the current block.  The PC position should follow 
// OBSOLETE    any procedure prologue code.  An argument specifies the entry
// OBSOLETE    number, which is keyed to the entry number of the corresponding
// OBSOLETE    .symbols ENTRY record.
// OBSOLETE 
// OBSOLETE    exit       Identifies the next table entry as the last position within 
// OBSOLETE    the current block before a procedure epiloge and subsequent
// OBSOLETE    procedure exit.
// OBSOLETE 
// OBSOLETE    gap        By default, the executable code corresponding to a table entry 
// OBSOLETE    is assumed to extend to the beginning of the next table entry.
// OBSOLETE    If this is not the case--there is a "hole" in the table--then
// OBSOLETE    a gap escape should follow the first table entry to specify
// OBSOLETE    where the code for that entry ends.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE #define dst_ln_escape_flag    -8
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    Escape function codes:
// OBSOLETE  */
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_ln_pad,			/* pad byte */
// OBSOLETE     dst_ln_file,		/* file escape.  Next 4 bytes are a dst_src_loc_t */
// OBSOLETE     dst_ln_dln1_dpc1,		/* 1 byte line delta, 1 byte pc delta */
// OBSOLETE     dst_ln_dln2_dpc2,		/* 2 bytes line delta, 2 bytes pc delta */
// OBSOLETE     dst_ln_ln4_pc4,		/* 4 bytes ABSOLUTE line number, 4 bytes ABSOLUTE pc */
// OBSOLETE     dst_ln_dln1_dpc0,		/* 1 byte line delta, pc delta = 0 */
// OBSOLETE     dst_ln_ln_off_1,		/* statement escape, stmt # = 1 (2nd stmt on line) */
// OBSOLETE     dst_ln_ln_off,		/* statement escape, stmt # = next byte */
// OBSOLETE     dst_ln_entry,		/* entry escape, next byte is entry number */
// OBSOLETE     dst_ln_exit,		/* exit escape */
// OBSOLETE     dst_ln_stmt_end,		/* gap escape, 4 bytes pc delta */
// OBSOLETE     dst_ln_escape_11,		/* reserved */
// OBSOLETE     dst_ln_escape_12,		/* reserved */
// OBSOLETE     dst_ln_escape_13,		/* reserved */
// OBSOLETE     dst_ln_nxt_byte,		/* next byte contains the real escape code */
// OBSOLETE     dst_ln_end,			/* end escape, final entry follows */
// OBSOLETE     dst_ln_escape_END_OF_ENUM
// OBSOLETE   }
// OBSOLETE dst_ln_escape_t;
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE    Line number table entry
// OBSOLETE  */
// OBSOLETE typedef union
// OBSOLETE   {
// OBSOLETE     struct
// OBSOLETE       {
// OBSOLETE 	unsigned int ln_delta:4;	/* 4 bit line number delta */
// OBSOLETE 	unsigned int pc_delta:4;	/* 4 bit pc delta */
// OBSOLETE       }
// OBSOLETE     delta;
// OBSOLETE 
// OBSOLETE     struct
// OBSOLETE       {
// OBSOLETE 	unsigned int esc_flag:4;	/* alias for ln_delta */
// OBSOLETE 	dst_ln_escape_t esc_code:4;	/* escape function code */
// OBSOLETE       }
// OBSOLETE     esc;
// OBSOLETE 
// OBSOLETE     char sdata;			/* signed data byte */
// OBSOLETE     unsigned char udata;	/* unsigned data byte */
// OBSOLETE   }
// OBSOLETE dst_ln_entry_t,
// OBSOLETE  *dst_ln_entry_ptr_t,
// OBSOLETE   dst_ln_table_t[dst_dummy_array_size];
// OBSOLETE 
// OBSOLETE /* The following macro will extract the ln_delta field as a signed value:
// OBSOLETE  */
// OBSOLETE #define dst_ln_ln_delta(ln_rec) \
// OBSOLETE     ( ((short) ((ln_rec).delta.ln_delta << 12)) >> 12 )
// OBSOLETE 
// OBSOLETE 
// OBSOLETE 
// OBSOLETE 
// OBSOLETE typedef struct dst_sec_struct
// OBSOLETE   {
// OBSOLETE     char *buffer;
// OBSOLETE     long position;
// OBSOLETE     long size;
// OBSOLETE     long base;
// OBSOLETE   }
// OBSOLETE dst_sec;
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Macros for access to the data */
// OBSOLETE 
// OBSOLETE #define DST_comp_unit(x) 	((x)->rec_data.comp_unit_)
// OBSOLETE #define DST_section_tab(x) 	((x)->rec_data.section_tab_)
// OBSOLETE #define DST_file_tab(x) 	((x)->rec_data.file_tab_)
// OBSOLETE #define DST_block(x) 		((x)->rec_data.block_)
// OBSOLETE #define	DST_var(x)		((x)->rec_data.var_)
// OBSOLETE #define DST_pointer(x) 		((x)->rec_data.pointer_)
// OBSOLETE #define DST_array(x) 		((x)->rec_data.array_)
// OBSOLETE #define DST_subrange(x) 	((x)->rec_data.subrange_)
// OBSOLETE #define DST_set(x)	 	((x)->rec_data.set_)
// OBSOLETE #define DST_implicit_enum(x) 	((x)->rec_data.implicit_enum_)
// OBSOLETE #define DST_explicit_enum(x) 	((x)->rec_data.explicit_enum_)
// OBSOLETE #define DST_short_rec(x) 	((x)->rec_data.record_)
// OBSOLETE #define DST_short_union(x) 	((x)->rec_data.record_)
// OBSOLETE #define DST_file(x) 		((x)->rec_data.file_)
// OBSOLETE #define DST_offset(x) 		((x)->rec_data.offset_)
// OBSOLETE #define DST_alias(x)	 	((x)->rec_data.alias_)
// OBSOLETE #define DST_signature(x) 	((x)->rec_data.signature_)
// OBSOLETE #define DST_old_label(x) 	((x)->rec_data.old_label_)
// OBSOLETE #define DST_scope(x) 		((x)->rec_data.scope_)
// OBSOLETE #define DST_string_tab(x) 	((x)->rec_data.string_tab_)
// OBSOLETE #define DST_global_name_tab(x) 	((x)->rec_data.string_tab_)
// OBSOLETE #define DST_forward(x) 		((x)->rec_data.forward_)
// OBSOLETE #define DST_aux_size(x) 	((x)->rec_data.aux_size_)
// OBSOLETE #define DST_aux_align(x) 	((x)->rec_data.aux_align_)
// OBSOLETE #define DST_aux_field_size(x) 	((x)->rec_data.aux_field_size_)
// OBSOLETE #define DST_aux_field_off(x) 	((x)->rec_data.aux_field_off_)
// OBSOLETE #define DST_aux_field_align(x) 	((x)->rec_data.aux_field_align_)
// OBSOLETE #define DST_aux_qual(x) 	((x)->rec_data.aux_qual_)
// OBSOLETE #define DST_aux_var_bound(x) 	((x)->rec_data.aux_var_bound_)
// OBSOLETE #define DST_extension(x) 	((x)->rec_data.extension_)
// OBSOLETE #define DST_string(x) 		((x)->rec_data.string_)
// OBSOLETE #define DST_const(x) 		((x)->rec_data.const_)
// OBSOLETE #define DST_reference(x) 	((x)->rec_data.pointer_)
// OBSOLETE #define DST_record(x) 		((x)->rec_data.record_)
// OBSOLETE #define DST_union(x) 		((x)->rec_data.record_)
// OBSOLETE #define DST_aux_type_deriv(x) 	((x)->rec_data.aux_type_deriv_)
// OBSOLETE #define DST_locpool(x) 		((x)->rec_data.string_tab_)
// OBSOLETE #define DST_variable(x) 	((x)->rec_data.variable_)
// OBSOLETE #define DST_label(x) 		((x)->rec_data.label_)
// OBSOLETE #define DST_entry(x) 		((x)->rec_data.entry_)
// OBSOLETE #define DST_aux_lifetime(x) 	((x)->rec_data.aux_lifetime_)
// OBSOLETE #define DST_aux_ptr_base(x) 	((x)->rec_data.aux_ptr_base_)
// OBSOLETE #define DST_aux_src_range(x) 	((x)->rec_data.aux_src_range_)
// OBSOLETE #define DST_aux_reg_val(x) 	((x)->rec_data.aux_reg_val_)
// OBSOLETE #define DST_aux_unit_names(x) 	((x)->rec_data.aux_unit_names_)
// OBSOLETE #define DST_aux_sect_info(x) 	((x)->rec_data.aux_sect_info_)
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * Type codes for loc strings. I'm not entirely certain about all of
// OBSOLETE  * these, but they seem to work.
// OBSOLETE  *                              troy@cbme.unsw.EDU.AU
// OBSOLETE  * If you find a variable whose location can't be decoded, you should
// OBSOLETE  * find out it's code using "dstdump -s filename". It will record an
// OBSOLETE  * entry for the variable, and give a text representation of what
// OBSOLETE  * the locstring means. Before that explaination there will be a
// OBSOLETE  * number. In the LOCSTRING table, that number will appear before
// OBSOLETE  * the start of the location string. Location string codes are
// OBSOLETE  * five bit codes with a 3 bit argument. Check the high 5 bits of
// OBSOLETE  * the one byte code, and figure out where it goes in here.
// OBSOLETE  * Then figure out exactly what the meaning is and code it in
// OBSOLETE  * dstread.c
// OBSOLETE  *
// OBSOLETE  * Note that ranged locs mean that the variable is in different locations
// OBSOLETE  * depending on the current PC. We ignore these because (a) gcc can't handle
// OBSOLETE  * them, and (b), If you don't use high levels of optimisation they won't
// OBSOLETE  * occur.
// OBSOLETE  */
// OBSOLETE typedef enum
// OBSOLETE   {
// OBSOLETE     dst_lsc_end,		/* End of string */
// OBSOLETE     dst_lsc_indirect,		/* Indirect through previous. Arg == 6 */
// OBSOLETE     /* Or register ax (x=arg) */
// OBSOLETE     dst_lsc_dreg,		/* register dx (x=arg) */
// OBSOLETE     dst_lsc_03,
// OBSOLETE     dst_lsc_section,		/* Section (arg+1) */
// OBSOLETE     dst_lsc_05,
// OBSOLETE     dst_lsc_06,
// OBSOLETE     dst_lsc_add,		/* Add (arg+1)*2 */
// OBSOLETE     dst_lsc_sub,		/* Subtract (arg+1)*2 */
// OBSOLETE     dst_lsc_09,
// OBSOLETE     dst_lsc_0a,
// OBSOLETE     dst_lsc_sec_byte,		/* Section of next byte+1 */
// OBSOLETE     dst_lsc_add_byte,		/* Add next byte (arg == 5) or next word
// OBSOLETE 				 * (arg == 6)
// OBSOLETE 				 */
// OBSOLETE     dst_lsc_sub_byte,		/* Subtract next byte. (arg == 1) or next
// OBSOLETE 				 * word (arg == 6 ?)
// OBSOLETE 				 */
// OBSOLETE     dst_lsc_sbreg,		/* Stack base register (frame pointer). Arg==0 */
// OBSOLETE     dst_lsc_0f,
// OBSOLETE     dst_lsc_ranged,		/* location is pc dependent */
// OBSOLETE     dst_lsc_11,
// OBSOLETE     dst_lsc_12,
// OBSOLETE     dst_lsc_13,
// OBSOLETE     dst_lsc_14,
// OBSOLETE     dst_lsc_15,
// OBSOLETE     dst_lsc_16,
// OBSOLETE     dst_lsc_17,
// OBSOLETE     dst_lsc_18,
// OBSOLETE     dst_lsc_19,
// OBSOLETE     dst_lsc_1a,
// OBSOLETE     dst_lsc_1b,
// OBSOLETE     dst_lsc_1c,
// OBSOLETE     dst_lsc_1d,
// OBSOLETE     dst_lsc_1e,
// OBSOLETE     dst_lsc_1f
// OBSOLETE   }
// OBSOLETE dst_loc_string_code_t;
// OBSOLETE 
// OBSOLETE /* If the following occurs after an addition/subtraction, that addition
// OBSOLETE  * or subtraction should be multiplied by 256. It's a complete byte, not
// OBSOLETE  * a code.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE #define	dst_multiply_256	((char) 0x73)
// OBSOLETE 
// OBSOLETE typedef struct
// OBSOLETE   {
// OBSOLETE     char code:5;
// OBSOLETE     char arg:3;
// OBSOLETE   }
// OBSOLETE dst_loc_header_t ALIGNED1;
// OBSOLETE 
// OBSOLETE typedef union
// OBSOLETE   {
// OBSOLETE     dst_loc_header_t header;
// OBSOLETE     char data;
// OBSOLETE   }
// OBSOLETE dst_loc_entry_t ALIGNED1;
// OBSOLETE 
// OBSOLETE #undef ALIGNED1
// OBSOLETE #endif /* apollo_dst_h */
