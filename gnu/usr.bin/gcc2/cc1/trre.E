# 1 "/a/users/paul/gcc2/cc1/../common/tree.c"
 



















 













# 1 "/a/users/paul/gcc2/cc1/../common/config.h" 1
 






















 



 






 



 





 


# 1 "/a/users/paul/gcc2/cc1/../common/tm.h" 1
 

 


# 1 "/a/users/paul/gcc2/cc1/../common/i386/gstabs.h" 1
# 1 "/a/users/paul/gcc2/cc1/../common/i386/gas.h" 1
 


















 


# 1 "/a/users/paul/gcc2/cc1/../common/i386/i386.h" 1
 



















 














 



 













 

extern int target_flags;

 

 




 

 

 

  

 





 



 




 




 






# 119 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 



 

 
 



 
 


 

 


 


 





 


 



 


 


 




 



 






 



 

 


 



 





 

 




 

















 






 










 

# 258 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 













 




 











 






 










 


 

 

 


 


 


 



 





 


 


 



 




 



 























enum reg_class
{
  NO_REGS,
  AREG, DREG, CREG, BREG,
  Q_REGS,			 
  SIREG, DIREG,
  INDEX_REGS,			 
  GENERAL_REGS,			 
  FP_TOP_REG, FP_SECOND_REG,	 
  FLOAT_REGS,
  ALL_REGS, LIM_REG_CLASSES
};





 


# 406 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 




# 421 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 




extern enum reg_class regclass_map[17 ];


 












  








 


 

 


 

 




 


# 487 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 



















 







 







 







 











 






 

 





 

 



 





 





 







 


 





















 







 





 





 






 










 








 








 













 







 











 










 



# 728 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 



 





 























 


 




# 781 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 



 









 
















 












 



# 850 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 

 
 

 
 

 

 

















 














 






 

   









# 922 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"


 

















 










 










 









 






# 1002 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 





















# 1056 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 












 






 







# 1097 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 






 



 


 

 




 


 


 



 





 
 

 


 


 

 

 



 




 




 




 




 





 



# 1194 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"


 





# 1217 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 



















 






 


 









 



extern struct rtx_def *i386_compare_op0, *i386_compare_op1;
extern struct rtx_def *(*i386_compare_gen)(), *(*i386_compare_gen_eq)();

 

 


 



 



 







 















 


 


 











 







 




 






 





 

 

# 1359 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 





 





 








 









 






 
 






 














 




 





 





 





 







 





 








 

















 






extern char *hi_reg_name[];
extern char *qi_reg_name[];
extern char *qi_high_reg_name[];


# 1527 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"







 





# 1560 "/a/users/paul/gcc2/cc1/../common/i386/i386.h"

 



 






 

 




 







 




# 23 "/a/users/paul/gcc2/cc1/../common/i386/gas.h" 2







 
 














# 1 "/a/users/paul/gcc2/cc1/../common/i386/bsd.h" 1
 




















 
# 1 "/a/users/paul/gcc2/cc1/../common/i386/unix.h" 1
 


















 



 



 












 





 





 


 


 


 


 


 



 




  
 



 











 

 




 




 




 



 



 



 





 





 





 




# 23 "/a/users/paul/gcc2/cc1/../common/i386/bsd.h" 2


 



 

 








 






 





 



 

 





 

 







 







 






 












 










 







 


 


 




# 46 "/a/users/paul/gcc2/cc1/../common/i386/gas.h" 2


 



 



 




 



 



 











 







 









 








# 125 "/a/users/paul/gcc2/cc1/../common/i386/gas.h"

 








 


# 158 "/a/users/paul/gcc2/cc1/../common/i386/gas.h"

# 1 "/a/users/paul/gcc2/cc1/../common/i386/gstabs.h" 2


 



 


# 6 "/a/users/paul/gcc2/cc1/../common/tm.h" 2


 
# 1 "/a/users/paul/gcc2/cc1/../common/i386/perform.h" 1
 


















 

 



 



# 39 "/a/users/paul/gcc2/cc1/../common/i386/perform.h"


# 49 "/a/users/paul/gcc2/cc1/../common/i386/perform.h"


# 60 "/a/users/paul/gcc2/cc1/../common/i386/perform.h"


# 70 "/a/users/paul/gcc2/cc1/../common/i386/perform.h"


# 93 "/a/users/paul/gcc2/cc1/../common/i386/perform.h"

# 9 "/a/users/paul/gcc2/cc1/../common/tm.h" 2





 
















 



 


# 51 "/a/users/paul/gcc2/cc1/../common/tm.h"

 










 
 

 







 




 


# 48 "/a/users/paul/gcc2/cc1/../common/config.h" 2

# 35 "/a/users/paul/gcc2/cc1/../common/tree.c" 2

# 1 "/a/users/paul/gcc2/cc1/../common/flags.h" 1
 


















 
extern char *main_input_filename;

enum debug_info_type
{
  NO_DEBUG,	     
  DBX_DEBUG,	     
  SDB_DEBUG,	     
  DWARF_DEBUG,	     
  XCOFF_DEBUG	     
};

 
extern enum debug_info_type write_symbols;

enum debug_info_level
{
  DINFO_LEVEL_NONE,	 
  DINFO_LEVEL_TERSE,	 
  DINFO_LEVEL_NORMAL,	 
  DINFO_LEVEL_VERBOSE	 
};

 
extern enum debug_info_level debug_info_level;

 

extern int use_gnu_debug_info_extensions;

 

extern int optimize;

 


extern int obey_regdecls;

 


extern int quiet_flag;

 

extern int inhibit_warnings;

 

extern int extra_warnings;

 

extern int warn_unused;

 

extern int warn_inline;

 

extern int warn_uninitialized;

 

extern int warn_shadow;

 

extern int warn_switch;

 


extern int warn_return_type;

 



extern int warn_cast_align;

 


extern int warn_template_debugging;

 


extern int warn_id_clash;
extern int id_clash_len;

 


extern int warn_aggregate_return;

 

extern int profile_flag;

 

extern int profile_block_flag;

 


extern int pedantic;

 


extern int in_system_header;

 


extern int flag_print_asm_name;

 

 

extern int flag_signed_char;

 

extern int flag_short_enums;

 



extern int flag_caller_saves;

 

extern int flag_pcc_struct_return;

 


extern int flag_force_mem;

 


extern int flag_force_addr;

 


extern int flag_defer_pop;

 


extern int flag_float_store;

 

extern int flag_strength_reduce;

 




extern int flag_unroll_loops;

 


extern int flag_unroll_all_loops;

 


extern int flag_cse_follow_jumps;

 


extern int flag_cse_skip_blocks;

 

extern int flag_expensive_optimizations;

 


extern int flag_writable_strings;

 



extern int flag_no_function_cse;

 


extern int flag_omit_frame_pointer;

 

extern int flag_no_peephole;

 

extern int flag_volatile;

 

extern int flag_volatile_global;

 




extern int flag_fast_math;

 


extern int flag_inline_functions;

 



extern int flag_keep_inline_functions;

 





extern int flag_no_inline;

 

extern int flag_syntax_only;

 

extern int flag_gen_aux_info;

 

extern int flag_shared_data;

 




extern int flag_schedule_insns;
extern int flag_schedule_insns_after_reload;

 

extern int flag_delayed_branch;

 



extern int flag_pretend_float;

 


extern int flag_pedantic_errors;

 


extern int flag_pic;

 

extern int flag_no_common;

 



extern int flag_inhibit_size_directive;

 




extern int flag_verbose_asm;

 

extern int flag_gnu_linker;

 

 



extern int frame_pointer_needed;

 


extern int can_reach_end;

 


extern int current_function_has_nonlocal_label;

# 36 "/a/users/paul/gcc2/cc1/../common/tree.c" 2

# 1 "/a/users/paul/gcc2/cc1/../common/tree.h" 1
 


















# 1 "/a/users/paul/gcc2/cc1/../common/machmode.h" 1
 



















 










 












 













 







 





 



enum machine_mode {
# 1 "/a/users/paul/gcc2/cc1/../common/machmode.def" 1
 




















 










 



























 

 VOIDmode, 

 QImode, 		 
 HImode, 
 


 PSImode, 
 SImode, 
 PDImode, 
 DImode, 
 TImode, 
 OImode, 

 QFmode, 
 HFmode, 
 SFmode, 
 DFmode, 
 XFmode,     
 TFmode, 

 
 SCmode, 
 DCmode, 
 XCmode, 
 TCmode, 

 CQImode, 
 CHImode, 
 CSImode, 
 CDImode, 
 CTImode, 
 COImode, 

 

 BLKmode, 

 





 CCmode, 

 


 





# 78 "/a/users/paul/gcc2/cc1/../common/machmode.h" 2



  CCFPEQmode ,

MAX_MACHINE_MODE };









 

extern char *mode_name[];


enum mode_class { MODE_RANDOM, MODE_INT, MODE_FLOAT, MODE_PARTIAL_INT, MODE_CC,
		  MODE_COMPLEX_INT, MODE_COMPLEX_FLOAT, MAX_MODE_CLASS};

 


extern enum mode_class mode_class[];


 

extern int mode_size[];


 

extern int mode_unit_size[];


 





 



 






 

extern enum machine_mode mode_wider_mode[];


 



extern enum machine_mode mode_for_size (unsigned int, enum mode_class, int) ;

 

extern enum machine_mode get_best_mode (int, int, int, enum machine_mode, int) ;

 





 

extern enum machine_mode class_narrowest_mode[];


 


extern enum machine_mode byte_mode;
extern enum machine_mode word_mode;


# 20 "/a/users/paul/gcc2/cc1/../common/tree.h" 2


 



enum tree_code {
# 1 "/a/users/paul/gcc2/cc1/../common/tree.def" 1
 



















 
 












 



 




  ERROR_MARK, 

 



  IDENTIFIER_NODE, 

 


  OP_IDENTIFIER, 

 
 



  TREE_LIST, 

 
  TREE_VEC, 

 

















  BLOCK, 

 

 


































  VOID_TYPE, 	 

 







  INTEGER_TYPE, 

 

  REAL_TYPE, 

 

  COMPLEX_TYPE, 

 




 





  ENUMERAL_TYPE, 

 

  BOOLEAN_TYPE, 

 

  CHAR_TYPE, 

 

  POINTER_TYPE, 

 



  OFFSET_TYPE, 

 

  REFERENCE_TYPE, 

 




  METHOD_TYPE, 

 
  FILE_TYPE, 

 







 
  ARRAY_TYPE, 

 

  SET_TYPE, 

 

  STRING_TYPE, 

 
 


 

  RECORD_TYPE, 

 

 

  UNION_TYPE, 	 

 



  QUAL_UNION_TYPE, 

 





  FUNCTION_TYPE, 

 



  LANG_TYPE, 

 

 

 




  INTEGER_CST, 

 
  REAL_CST, 

 


  COMPLEX_CST, 

 

  STRING_CST, 

 

























































  FUNCTION_DECL, 
  LABEL_DECL, 
  CONST_DECL, 
  TYPE_DECL, 
  VAR_DECL, 
  PARM_DECL, 
  RESULT_DECL, 
  FIELD_DECL, 

 

 


  COMPONENT_REF, 

 






  BIT_FIELD_REF, 
   
 
  INDIRECT_REF, 

 



  OFFSET_REF, 

 
  BUFFER_REF, 

 


  ARRAY_REF, 

 





  CONSTRUCTOR, 

 




 

  COMPOUND_EXPR, 

 
  MODIFY_EXPR, 

 

  INIT_EXPR, 

 


  TARGET_EXPR, 

 



  COND_EXPR, 

 




















  BIND_EXPR, 

 




  CALL_EXPR, 

 


  METHOD_CALL_EXPR, 

 







  WITH_CLEANUP_EXPR, 

 

  PLUS_EXPR, 
  MINUS_EXPR, 
  MULT_EXPR, 

 
 



  TRUNC_DIV_EXPR, 

 
  CEIL_DIV_EXPR, 

 
  FLOOR_DIV_EXPR, 

 
  ROUND_DIV_EXPR, 

 
  TRUNC_MOD_EXPR, 
  CEIL_MOD_EXPR, 
  FLOOR_MOD_EXPR, 
  ROUND_MOD_EXPR, 

 



  RDIV_EXPR, 

 

  EXACT_DIV_EXPR, 

 




  FIX_TRUNC_EXPR, 
  FIX_CEIL_EXPR, 
  FIX_FLOOR_EXPR, 
  FIX_ROUND_EXPR, 

 
  FLOAT_EXPR, 

 

  EXPON_EXPR, 

 
  NEGATE_EXPR, 

  MIN_EXPR, 
  MAX_EXPR, 
  ABS_EXPR, 
  FFS_EXPR, 

 





  LSHIFT_EXPR, 
  RSHIFT_EXPR, 
  LROTATE_EXPR, 
  RROTATE_EXPR, 

 
  BIT_IOR_EXPR, 
  BIT_XOR_EXPR, 
  BIT_AND_EXPR, 
  BIT_ANDTC_EXPR, 
  BIT_NOT_EXPR, 

 




  TRUTH_ANDIF_EXPR, 
  TRUTH_ORIF_EXPR, 
  TRUTH_AND_EXPR, 
  TRUTH_OR_EXPR, 
  TRUTH_XOR_EXPR, 
  TRUTH_NOT_EXPR, 

 





  LT_EXPR, 
  LE_EXPR, 
  GT_EXPR, 
  GE_EXPR, 
  EQ_EXPR, 
  NE_EXPR, 

 
  IN_EXPR, 
  SET_LE_EXPR, 
  CARD_EXPR, 
  RANGE_EXPR, 

 


  CONVERT_EXPR, 

 
  NOP_EXPR, 

 
  NON_LVALUE_EXPR, 

 



  SAVE_EXPR, 

 



  RTL_EXPR, 

 

  ADDR_EXPR, 

 
  REFERENCE_EXPR, 

 

  ENTRY_VALUE_EXPR, 

 

  COMPLEX_EXPR, 

 

  CONJ_EXPR, 

 

  REALPART_EXPR, 
  IMAGPART_EXPR, 

 


  PREDECREMENT_EXPR, 
  PREINCREMENT_EXPR, 
  POSTDECREMENT_EXPR, 
  POSTINCREMENT_EXPR, 

 


 


  LABEL_EXPR, 

 

  GOTO_EXPR, 

 




  RETURN_EXPR, 

 

  EXIT_EXPR, 

 


  LOOP_EXPR, 

 





# 27 "/a/users/paul/gcc2/cc1/../common/tree.h" 2


  LAST_AND_UNUSED_TREE_CODE	 

};



 


 






extern char **tree_code_type;


 

extern int *tree_code_length;

 

extern char **tree_code_name;

 


enum built_in_function
{
  NOT_BUILT_IN,
  BUILT_IN_ALLOCA,
  BUILT_IN_ABS,
  BUILT_IN_FABS,
  BUILT_IN_LABS,
  BUILT_IN_FFS,
  BUILT_IN_DIV,
  BUILT_IN_LDIV,
  BUILT_IN_FFLOOR,
  BUILT_IN_FCEIL,
  BUILT_IN_FMOD,
  BUILT_IN_FREM,
  BUILT_IN_MEMCPY,
  BUILT_IN_MEMCMP,
  BUILT_IN_MEMSET,
  BUILT_IN_STRCPY,
  BUILT_IN_STRCMP,
  BUILT_IN_STRLEN,
  BUILT_IN_FSQRT,
  BUILT_IN_SIN,
  BUILT_IN_COS,
  BUILT_IN_GETEXP,
  BUILT_IN_GETMAN,
  BUILT_IN_SAVEREGS,
  BUILT_IN_CLASSIFY_TYPE,
  BUILT_IN_NEXT_ARG,
  BUILT_IN_ARGS_INFO,
  BUILT_IN_CONSTANT_P,
  BUILT_IN_FRAME_ADDRESS,
  BUILT_IN_RETURN_ADDRESS,
  BUILT_IN_CALLER_RETURN_ADDRESS,
  BUILT_IN_APPLY_ARGS,
  BUILT_IN_APPLY,
  BUILT_IN_RETURN,

   
  BUILT_IN_NEW,
  BUILT_IN_VEC_NEW,
  BUILT_IN_DELETE,
  BUILT_IN_VEC_DELETE
};

 

 













 

typedef union tree_node *tree;

 




struct tree_common
{
  union tree_node *chain;
  union tree_node *type;



  enum tree_code code : 8;


  unsigned side_effects_flag : 1;
  unsigned constant_flag : 1;
  unsigned permanent_flag : 1;
  unsigned addressable_flag : 1;
  unsigned volatile_flag : 1;
  unsigned readonly_flag : 1;
  unsigned unsigned_flag : 1;
  unsigned asm_written_flag: 1;

  unsigned used_flag : 1;
  unsigned raises_flag : 1;
  unsigned static_flag : 1;
  unsigned public_flag : 1;
  unsigned private_flag : 1;
  unsigned protected_flag : 1;

  unsigned lang_flag_0 : 1;
  unsigned lang_flag_1 : 1;
  unsigned lang_flag_2 : 1;
  unsigned lang_flag_3 : 1;
  unsigned lang_flag_4 : 1;
  unsigned lang_flag_5 : 1;
  unsigned lang_flag_6 : 1;
   
};

 


 




 




 










 










 









 






 







 

 













 




 



 


 



 






 




 


 





 






 











 






 





 




 




 








 





 



 

 



 








 

 




















struct tree_int_cst
{
  char common[sizeof (struct tree_common)];
  int  int_cst_low;
  int  int_cst_high;
};

 





 
 





# 1 "/a/users/paul/gcc2/cc1/../common/real.h" 1
 





















 





 













 






# 141 "/a/users/paul/gcc2/cc1/../common/real.h"


 
# 163 "/a/users/paul/gcc2/cc1/../common/real.h"




 










 









 



















 




 




 




 




 




 




 





 


extern double ldexp ();


 


 
double  ereal_atof ();

# 267 "/a/users/paul/gcc2/cc1/../common/real.h"



 




 



 
extern double  real_value_truncate ();







 




 




 




 




 

extern double  dconst0;
extern double  dconst1;
extern double  dconst2;
extern double  dconstm1;

 


union real_extract 
{
  double  d;
  int  i[sizeof (double ) / sizeof (int )];
};

 









 

 




 

double  real_value_from_int_cst ();

 






 



 







# 396 "/a/users/paul/gcc2/cc1/../common/tree.h" 2


struct tree_real_cst
{
  char common[sizeof (struct tree_common)];
  struct rtx_def *rtl;	 

  double  real_cst;
};

 



struct tree_string
{
  char common[sizeof (struct tree_common)];
  struct rtx_def *rtl;	 

  int length;
  char *pointer;
};

 



struct tree_complex
{
  char common[sizeof (struct tree_common)];
  struct rtx_def *rtl;	 

  union tree_node *real;
  union tree_node *imag;
};

 




struct tree_identifier
{
  char common[sizeof (struct tree_common)];
  int length;
  char *pointer;
};

 



struct tree_list
{
  char common[sizeof (struct tree_common)];
  union tree_node *purpose;
  union tree_node *value;
};

 




struct tree_vec
{
  char common[sizeof (struct tree_common)];
  int length;
  union tree_node *a[1];
};

 

 



 



 


 


 



struct tree_exp
{
  char common[sizeof (struct tree_common)];
  int complexity;
  union tree_node *operands[1];
};

 




 






 



struct tree_block
{
  char common[sizeof (struct tree_common)];

  unsigned handler_block_flag : 1;
  unsigned abstract_flag : 1;

  union tree_node *vars;
  union tree_node *type_tags;
  union tree_node *subblocks;
  union tree_node *supercontext;
  union tree_node *abstract_origin;
  struct rtx_def *end_note;
};

 

 




























 





 




 


 


 








struct tree_type
{
  char common[sizeof (struct tree_common)];
  union tree_node *values;
  union tree_node *size;
  unsigned uid;




  enum machine_mode mode : 8;

  unsigned char precision;

  unsigned no_force_blk_flag : 1;
  unsigned lang_flag_0 : 1;
  unsigned lang_flag_1 : 1;
  unsigned lang_flag_2 : 1;
  unsigned lang_flag_3 : 1;
  unsigned lang_flag_4 : 1;
  unsigned lang_flag_5 : 1;
  unsigned lang_flag_6 : 1;

  unsigned int align;
  union tree_node *pointer_to;
  union tree_node *reference_to;
  int parse_info;
  int symtab_address;
  union tree_node *name;
  union tree_node *minval;
  union tree_node *maxval;
  union tree_node *next_variant;
  union tree_node *main_variant;
  union tree_node *binfo;
  union tree_node *noncopied_parts;
  union tree_node *context;
   
  struct lang_type *lang_specific;
};

 














 


 




   




 






 





 












 





 



 












 

 


 

 




 






 


 



 
 


 

 

 





 


 


 


 


 

 



 





 


 

 


 





 


 







 




 


 




 




 



 







 



 


 




 



 


 








 



 




 


 


 


 



 









struct tree_decl
{
  char common[sizeof (struct tree_common)];
  char *filename;
  int linenum;
  union tree_node *size;
  unsigned int uid;



  enum machine_mode mode : 8;


  unsigned external_flag : 1;
  unsigned nonlocal_flag : 1;
  unsigned regdecl_flag : 1;
  unsigned inline_flag : 1;
  unsigned bit_field_flag : 1;
  unsigned virtual_flag : 1;
  unsigned ignored_flag : 1;
  unsigned abstract_flag : 1;

  unsigned in_system_header_flag : 1;
   

  unsigned lang_flag_0 : 1;
  unsigned lang_flag_1 : 1;
  unsigned lang_flag_2 : 1;
  unsigned lang_flag_3 : 1;
  unsigned lang_flag_4 : 1;
  unsigned lang_flag_5 : 1;
  unsigned lang_flag_6 : 1;
  unsigned lang_flag_7 : 1;

  union tree_node *name;
  union tree_node *context;
  union tree_node *arguments;
  union tree_node *result;
  union tree_node *initial;
  union tree_node *abstract_origin;
   
  char *print_name;
  union tree_node *assembler_name;
  struct rtx_def *rtl;	 

   


  int frame_size;
   


  union {
    struct rtx_def *r;
    int i;
  } saved_insns;
  union tree_node *vindex;
   
  struct lang_decl *lang_specific;
};

 



union tree_node
{
  struct tree_common common;
  struct tree_int_cst int_cst;
  struct tree_real_cst real_cst;
  struct tree_string string;
  struct tree_complex complex;
  struct tree_identifier identifier;
  struct tree_decl decl;
  struct tree_type type;
  struct tree_list list;
  struct tree_vec vec;
  struct tree_exp exp;
  struct tree_block block;
 };

 











 

















 












 















extern char *xmalloc ();
extern char *xrealloc ();


extern char *oballoc			(int) ;
extern char *permalloc			(int) ;
extern char *savealloc			(int) ;
extern void free			(void *) ;

 



extern tree make_node			(enum tree_code) ;

 



extern tree copy_node			(tree) ;

 

extern tree copy_list			(tree) ;

 

extern tree make_tree_vec		(int) ;

 


extern tree get_identifier		(char *) ;

 




# 1065 "/a/users/paul/gcc2/cc1/../common/tree.h"

extern tree build ();
extern tree build_nt ();
extern tree build_parse_node ();


extern tree build_int_2_wide		(int , int ) ;
extern tree build_real			(tree, double ) ;
extern tree build_real_from_int_cst 	(tree, tree) ;
extern tree build_complex		(tree, tree) ;
extern tree build_string		(int, char *) ;
extern tree build1			(enum tree_code, tree, tree) ;
extern tree build_tree_list		(tree, tree) ;
extern tree build_decl_list		(tree, tree) ;
extern tree build_decl			(enum tree_code, tree, tree) ;
extern tree build_block			(tree, tree, tree, tree, tree) ;

 

extern tree make_signed_type		(int) ;
extern tree make_unsigned_type		(int) ;
extern tree signed_or_unsigned_type 	(int, tree) ;
extern void fixup_unsigned_type		(tree) ;
extern tree build_pointer_type		(tree) ;
extern tree build_reference_type 	(tree) ;
extern tree build_index_type		(tree) ;
extern tree build_index_2_type		(tree, tree) ;
extern tree build_array_type		(tree, tree) ;
extern tree build_function_type		(tree, tree) ;
extern tree build_method_type		(tree, tree) ;
extern tree build_offset_type		(tree, tree) ;
extern tree build_complex_type		(tree) ;
extern tree array_type_nelts		(tree) ;

extern tree value_member		(tree, tree) ;
extern tree purpose_member		(tree, tree) ;
extern tree binfo_member		(tree, tree) ;
extern int tree_int_cst_equal		(tree, tree) ;
extern int tree_int_cst_lt		(tree, tree) ;
extern int index_type_equal		(tree, tree) ;

 



extern tree make_tree ();

 






extern tree build_type_variant		(tree, int, int) ;

 

extern tree build_type_copy		(tree) ;

 




extern void layout_type			(tree) ;

 




extern tree type_hash_canon		(int, tree) ;

 







extern void layout_decl			(tree, unsigned) ;

 





extern tree fold			(tree) ;

 

extern tree non_lvalue			(tree) ;

extern tree convert			(tree, tree) ;
extern tree size_in_bytes		(tree) ;
extern int int_size_in_bytes		(tree) ;
extern tree size_binop			(enum tree_code, tree, tree) ;
extern tree size_int			(unsigned) ;
extern tree round_up			(tree, int) ;
extern tree get_pending_sizes		(void) ;

 

extern tree sizetype;

 



extern tree chainon			(tree, tree) ;

 

extern tree tree_cons			(tree, tree, tree) ;
extern tree perm_tree_cons		(tree, tree, tree) ;
extern tree temp_tree_cons		(tree, tree, tree) ;
extern tree saveable_tree_cons		(tree, tree, tree) ;
extern tree decl_tree_cons		(tree, tree, tree) ;

 

extern tree tree_last			(tree) ;

 

extern tree nreverse			(tree) ;

 


extern int list_length			(tree) ;

 

extern int integer_zerop		(tree) ;

 

extern int integer_onep			(tree) ;

 


extern int integer_all_onesp		(tree) ;

 


extern int integer_pow2p		(tree) ;

 


extern int staticp			(tree) ;

 


extern int lvalue_or_else		(tree, char *) ;

 



extern tree save_expr			(tree) ;

 





extern tree variable_size		(tree) ;

 



extern tree stabilize_reference		(tree) ;

 




extern tree get_unwidened		(tree, tree) ;

 




extern tree get_narrower		(tree, int *) ;

 




extern tree type_for_mode		(enum machine_mode, int) ;

 




extern tree type_for_size		(unsigned, int) ;

 




extern tree unsigned_type		(tree) ;

 




extern tree signed_type			(tree) ;

 



extern tree maybe_build_cleanup		(tree) ;

 



extern tree get_inner_reference		(tree, int *, int *, tree *, enum machine_mode *, int *, int *) ;

 

extern tree decl_function_context 	(tree) ;

 

extern tree decl_type_context		(tree) ;

 




extern char *function_cannot_inline_p 	(tree) ;

 
extern int real_zerop (tree) ;

 

 
extern tree integer_zero_node;

 
extern tree integer_one_node;

 
extern tree size_zero_node;

 
extern tree size_one_node;

 
extern tree null_pointer_node;

 
extern tree error_mark_node;

 
extern tree void_type_node;

 
extern tree integer_type_node;

 
extern tree unsigned_type_node;

 
extern tree char_type_node;

 

extern char *input_filename;

 
extern int lineno;

 

extern int pedantic;

 


extern int immediate_size_expand;

 

extern tree current_function_decl;

 

extern int current_function_calls_setjmp;

 

extern int current_function_calls_longjmp;

 

extern int all_types_permanent;

 

extern char *(*decl_printable_name) ();

 


extern void (*incomplete_decl_finalize_hook) ();

 
extern char *perm_calloc			(int, long) ;

 

extern tree expand_start_stmt_expr		(void) ;
extern tree expand_end_stmt_expr		(tree) ;
extern void expand_expr_stmt			(tree) ;
extern void expand_decl_init			(tree) ;
extern void clear_last_expr			(void) ;
extern void expand_label			(tree) ;
extern void expand_goto				(tree) ;
extern void expand_asm				(tree) ;
extern void expand_start_cond			(tree, int) ;
extern void expand_end_cond			(void) ;
extern void expand_start_else			(void) ;
extern void expand_start_elseif			(tree) ;
extern struct nesting *expand_start_loop 	(int) ;
extern struct nesting *expand_start_loop_continue_elsewhere 	(int) ;
extern void expand_loop_continue_here		(void) ;
extern void expand_end_loop			(void) ;
extern int expand_continue_loop			(struct nesting *) ;
extern int expand_exit_loop			(struct nesting *) ;
extern int expand_exit_loop_if_false		(struct nesting *, tree) ;
extern int expand_exit_something		(void) ;

extern void expand_null_return			(void) ;
extern void expand_return			(tree) ;
extern void expand_start_bindings		(int) ;
extern void expand_end_bindings			(tree, int, int) ;
extern tree last_cleanup_this_contour		(void) ;
extern void expand_start_case			(int, tree, tree, char *) ;
extern void expand_end_case			(tree) ;
extern int pushcase				(tree, tree, tree *) ;
extern int pushcase_range			(tree, tree, tree, tree *) ;

 

extern tree invert_truthvalue			(tree) ;

 

 
extern void init_lex				(void) ;
 
extern void init_decl_processing		(void) ;

 

extern void lang_init				(void) ;
extern void lang_finish				(void) ;

 
extern char *lang_identify			(void) ;

 
extern int yyparse				(void) ;
 


extern int lang_decode_option			(char *) ;

 
 

extern void pushlevel				(int) ;
 




extern tree poplevel				(int, int, int) ;
 
extern void set_block				(tree) ;
 



extern tree pushdecl				(tree) ;
 
extern tree getdecls				(void) ;
 
extern tree gettags				(void) ;

extern tree build_range_type (tree, tree, tree) ;

 


extern int suspend_momentary (void) ;

extern int allocation_temporary_p (void) ;

 



extern void resume_momentary (int) ;

 
extern void rest_of_type_compilation (tree, int) ;

 
extern void push_obstacks_nochange (void) ;

extern void push_momentary (void) ;

extern void clear_momentary (void) ;

extern void pop_momentary (void) ;

extern void end_temporary_allocation (void) ;

 
extern void pop_obstacks (void) ;
# 37 "/a/users/paul/gcc2/cc1/../common/tree.c" 2

# 1 "/a/users/paul/gcc2/cc1/../common/function.h" 1
 


























struct var_refs_queue
  {
    int *  modified;
    enum machine_mode promoted_mode;
    int unsignedp;
    struct var_refs_queue *next;
  };

 




struct sequence_stack
{
   
  int *  first, last;
  struct sequence_stack *next;
};

extern struct sequence_stack *sequence_stack;

 


struct function
{
  struct function *next;

   
  char *name;
  tree decl;
  int pops_args;
  int returns_struct;
  int returns_pcc_struct;
  int needs_context;
  int calls_setjmp;
  int calls_longjmp;
  int calls_alloca;
  int has_nonlocal_label;
  int *  nonlocal_goto_handler_slot;
  int *  nonlocal_goto_stack_level;
  tree nonlocal_labels;
  int args_size;
  int pretend_args_size;
  int *  arg_offset_rtx;
  int max_parm_reg;
  int *  *parm_reg_stack_loc;
  int outgoing_args_size;
  int *  return_rtx;
  int *  cleanup_label;
  int *  return_label;
  int *  save_expr_regs;
  int *  stack_slot_list;
  int *  parm_birth_insn;
  int frame_offset;
  int *  tail_recursion_label;
  int *  tail_recursion_reentry;
  int *  internal_arg_pointer;
  int *  arg_pointer_save_area;
  tree rtl_expr_chain;
  int *  last_parm_insn;
  tree context_display;
  tree trampoline_list;
  int function_call_count;
  struct temp_slot *temp_slots;
  int temp_slot_level;
   

  struct var_refs_queue *fixup_var_refs_queue;

   
  struct nesting *block_stack;
  struct nesting *stack_block_stack;
  struct nesting *cond_stack;
  struct nesting *loop_stack;
  struct nesting *case_stack;
  struct nesting *nesting_stack;
  int nesting_depth;
  int block_start_count;
  tree last_expr_type;
  int *  last_expr_value;
  int expr_stmts_for_value;
  char *emit_filename;
  int emit_lineno;
  struct goto_fixup *goto_fixup_chain;

   
  int pending_stack_adjust;
  int inhibit_defer_pop;
  tree cleanups_this_call;
  int *  saveregs_value;
  int *  apply_args_value;
  int *  forced_labels;

   
  int reg_rtx_no;
  int first_label_num;
  int *  first_insn;
  int *  last_insn;
  struct sequence_stack *sequence_stack;
  int cur_insn_uid;
  int last_linenum;
  char *last_filename;
  char *regno_pointer_flag;
  int regno_pointer_flag_length;
  int *  *regno_reg_rtx;

   
  tree permanent_type_chain;
  tree temporary_type_chain;
  tree permanent_type_end;
  tree temporary_type_end;
  tree pending_sizes;
  int immediate_size_expand;

   
  int all_types_permanent;
  struct momentary_level *momentary_stack;
  char *maybepermanent_firstobj;
  char *temporary_firstobj;
  char *momentary_firstobj;
  struct obstack *current_obstack;
  struct obstack *function_obstack;
  struct obstack *function_maybepermanent_obstack;
  struct obstack *expression_obstack;
  struct obstack *saveable_obstack;
  struct obstack *rtl_obstack;

   
  int uses_const_pool;

   
  int uses_pic_offset_table;

   
  int *  epilogue_delay_list;

   
  struct constant_descriptor **const_rtx_hash_table;
  struct pool_sym **const_rtx_sym_hash_table;
  struct pool_constant *first_pool, *last_pool;
  int pool_offset;
};

 
extern tree inline_function_decl;

 


extern int *  return_label;

 

extern int *  stack_slot_list;

 

struct function *find_function_data (tree) ;

 
extern struct function *outer_function_chain;

 


tree *identify_blocks (tree, int * ) ;








# 38 "/a/users/paul/gcc2/cc1/../common/tree.c" 2

# 1 "/a/users/paul/gcc2/cc1/../common/obstack.h" 1
 
















 


















































































 




 











 




# 133 "/a/users/paul/gcc2/cc1/../common/obstack.h"

# 1 "/usr/include/stddef.h" 1
 





































# 1 "/usr/include/machine/ansi.h" 1
 






































 


















# 39 "/usr/include/stddef.h" 2


typedef	int				ptrdiff_t;


typedef	unsigned int			size_t;




typedef	unsigned short			wchar_t;










# 134 "/a/users/paul/gcc2/cc1/../common/obstack.h" 2










struct _obstack_chunk		 
{
  char  *limit;			 
  struct _obstack_chunk *prev;	 
  char	contents[4];		 
};

struct obstack		 
{
  long	chunk_size;		 
  struct _obstack_chunk* chunk;	 
  char	*object_base;		 
  char	*next_free;		 
  char	*chunk_limit;		 
  ptrdiff_t  temp;		 
  int   alignment_mask;		 
  struct _obstack_chunk *(*chunkfun) ();  
  void (*freefun) ();		 
  char *extra_arg;		 
  unsigned use_extra_arg:1;	 
  unsigned maybe_empty_object:1; 



};

 


extern void _obstack_newchunk (struct obstack *, int);
extern void _obstack_free (struct obstack *, void *);
extern void _obstack_begin (struct obstack *, int, int,
			    void *(*) (), void (*) ());
extern void _obstack_begin_1 (struct obstack *, int, int,
			      void *(*) (), void (*) (), void *);









 


void obstack_init (struct obstack *obstack);

void * obstack_alloc (struct obstack *obstack, int size);

void * obstack_copy (struct obstack *obstack, void *address, int size);
void * obstack_copy0 (struct obstack *obstack, void *address, int size);

void obstack_free (struct obstack *obstack, void *block);

void obstack_blank (struct obstack *obstack, int size);

void obstack_grow (struct obstack *obstack, void *data, int size);
void obstack_grow0 (struct obstack *obstack, void *data, int size);

void obstack_1grow (struct obstack *obstack, int data_char);
void obstack_ptr_grow (struct obstack *obstack, void *data);
void obstack_int_grow (struct obstack *obstack, int data);

void * obstack_finish (struct obstack *obstack);

int obstack_object_size (struct obstack *obstack);

int obstack_room (struct obstack *obstack);
void obstack_1grow_fast (struct obstack *obstack, int data_char);
void obstack_ptr_grow_fast (struct obstack *obstack, void *data);
void obstack_int_grow_fast (struct obstack *obstack, int data);
void obstack_blank_fast (struct obstack *obstack, int size);

void * obstack_base (struct obstack *obstack);
void * obstack_next_free (struct obstack *obstack);
int obstack_alignment_mask (struct obstack *obstack);
int obstack_chunk_size (struct obstack *obstack);



 


 





 



 



 




























 














 





# 301 "/a/users/paul/gcc2/cc1/../common/obstack.h"


# 312 "/a/users/paul/gcc2/cc1/../common/obstack.h"









 


   














































 


# 387 "/a/users/paul/gcc2/cc1/../common/obstack.h"









# 482 "/a/users/paul/gcc2/cc1/../common/obstack.h"



# 39 "/a/users/paul/gcc2/cc1/../common/tree.c" 2

# 1 "/a/users/paul/gcc2/cc1/../common/gvarargs.h" 1




 



































# 57 "/a/users/paul/gcc2/cc1/../common/gvarargs.h"


 








 



 


 






typedef void *__gnuc_va_list;











 
























 





# 140 "/a/users/paul/gcc2/cc1/../common/gvarargs.h"


 




 

 






typedef __gnuc_va_list va_list;






 






# 40 "/a/users/paul/gcc2/cc1/../common/tree.c" 2

# 1 "/usr/include/stdio.h" 1
 








































# 1 "/usr/include/sys/cdefs.h" 1
 














































 











# 77 "/usr/include/sys/cdefs.h"


 
















# 42 "/usr/include/stdio.h" 2


# 1 "/usr/include/machine/ansi.h" 1
 



































# 58 "/usr/include/machine/ansi.h"

# 44 "/usr/include/stdio.h" 2










typedef long fpos_t;		 



 





 
struct __sbuf {
	unsigned char *_base;
	int	_size;
};

 























typedef	struct __sFILE {
	unsigned char *_p;	 
	int	_r;		 
	int	_w;		 
	short	_flags;		 
	short	_file;		 
	struct	__sbuf _bf;	 
	int	_lbfsize;	 

	 
	void	*_cookie;	 
	int	(*_close) (void *)		;
	int	(*_read)  (void *, char *, int)		;
	fpos_t	(*_seek)  (void *, fpos_t, int)		;
	int	(*_write) (void *, const char *, int)		;

	 
	struct	__sbuf _ub;	 
	unsigned char *_up;	 
	int	_ur;		 

	 
	unsigned char _ubuf[3];	 
	unsigned char _nbuf[1];	 

	 
	struct	__sbuf _lb;	 

	 
	int	_blksize;	 
	int	_offset;	 
} FILE;

 
extern FILE __sF[];
 





	 











 















 







 




















 


 
void	 clearerr (FILE *)		;
int	 fclose (FILE *)		;
int	 feof (FILE *)		;
int	 ferror (FILE *)		;
int	 fflush (FILE *)		;
int	 fgetc (FILE *)		;
int	 fgetpos (FILE *, fpos_t *)		;
char	*fgets (char *, size_t, FILE *)		;
FILE	*fopen (const char *, const char *)		;
int	 fprintf (FILE *, const char *, ...)		;
int	 fputc (int, FILE *)		;
int	 fputs (const char *, FILE *)		;
int	 fread (void *, size_t, size_t, FILE *)		;
FILE	*freopen (const char *, const char *, FILE *)		;
int	 fscanf (FILE *, const char *, ...)		;
int	 fseek (FILE *, long, int)		;
int	 fsetpos (FILE *, const fpos_t *)		;
long	 ftell (const FILE *)		;
int	 fwrite (const void *, size_t, size_t, FILE *)		;
int	 getc (FILE *)		;
int	 getchar (void)		;
char	*gets (char *)		;

extern int sys_nerr;			 
extern char *sys_errlist[];

void	 perror (const char *)		;
int	 printf (const char *, ...)		;
int	 putc (int, FILE *)		;
int	 putchar (int)		;
int	 puts (const char *)		;
int	 remove (const char *)		;
int	 rename  (const char *, const char *)		;
void	 rewind (FILE *)		;
int	 scanf (const char *, ...)		;
void	 setbuf (FILE *, char *)		;
int	 setvbuf (FILE *, char *, int, size_t)		;
int	 sprintf (char *, const char *, ...)		;
int	 sscanf (char *, const char *, ...)		;
FILE	*tmpfile (void)		;
char	*tmpnam (char *)		;
int	 ungetc (int, FILE *)		;
int	 vfprintf (FILE *, const char *, char *			)		;
int	 vprintf (const char *, char *			)		;
int	 vsprintf (char *, const char *, char *			)		;
 

 






 
char	*ctermid (char *)		;
FILE	*fdopen (int, const char *)		;
int	 fileno (FILE *)		;
 


 



 
char	*fgetline (FILE *, size_t *)		;
int	 fpurge (FILE *)		;
int	 getw (FILE *)		;
int	 pclose (FILE *)		;
FILE	*popen (const char *, const char *)		;
int	 putw (int, FILE *)		;
void	 setbuffer (FILE *, char *, int)		;
int	 setlinebuf (FILE *)		;
char	*tempnam (const char *, const char *)		;
int	 snprintf (char *, size_t, const char *, ...)		;
int	 vsnprintf (char *, size_t, const char *, char *			)		;
int	 vscanf (const char *, char *			)		;
int	 vsscanf (const char *, const char *, char *			)		;
 

 






 


 
FILE	*funopen (const void *,
		int (*)(void *, char *, int),
		int (*)(void *, const char *, int),
		fpos_t (*)(void *, fpos_t, int),
		int (*)(void *))		;
 




 


 
int	__srget (FILE *)		;
int	__svfscanf (FILE *, const char *, char *			)		;
int	__swbuf (int, FILE *)		;
 

 





static __inline int __sputc(int _c, FILE *_p) {
	if (--_p->_w >= 0 || (_p->_w >= _p->_lbfsize && (char)_c != '\n'))
		return (*_p->_p++ = _c);
	else
		return (__swbuf(_c, _p));
}
# 331 "/usr/include/stdio.h"























# 41 "/a/users/paul/gcc2/cc1/../common/tree.c" 2





 



struct obstack permanent_obstack;

 





struct obstack *function_maybepermanent_obstack;

 

struct obstack maybepermanent_obstack;

 




struct obstack *function_obstack;

 

struct obstack temporary_obstack;

 


struct obstack momentary_obstack;

 



static struct obstack temp_decl_obstack;

 


struct obstack *saveable_obstack;

 



struct obstack *rtl_obstack;

 

struct obstack *current_obstack;

 


struct obstack *expression_obstack;

 

struct obstack_stack
{
  struct obstack_stack *next;
  struct obstack *current;
  struct obstack *saveable;
  struct obstack *expression;
  struct obstack *rtl;
};

struct obstack_stack *obstack_stack;

 

static struct obstack obstack_stack_obstack;

 

char *maybepermanent_firstobj;
char *temporary_firstobj;
char *momentary_firstobj;
char *temp_decl_firstobj;

 

int all_types_permanent;

 
   
struct momentary_level
{
   
  struct momentary_level *prev;
   
  char *base;
   
  struct obstack *obstack;
};

struct momentary_level *momentary_stack;

 





char *standard_tree_code_type[] = {
# 1 "/a/users/paul/gcc2/cc1/../common/tree.def" 1
 



















 
 












 



 




 "x", 

 



 "x", 

 


 "x", 

 
 



 "x", 

 
 "x", 

 

















 "b", 

 

 


































 "t", 	 

 







 "t", 

 

 "t", 

 

 "t", 

 




 





 "t", 

 

 "t", 

 

 "t", 

 

 "t", 

 



 "t", 

 

 "t", 

 




 "t", 

 
 "t", 

 







 
 "t", 

 

 "t", 

 

 "t", 

 
 


 

 "t", 

 

 

 "t", 	 

 



 "t", 

 





 "t", 

 



 "t", 

 

 

 




 "c", 

 
 "c", 

 


 "c", 

 

 "c", 

 

























































 "d", 
 "d", 
 "d", 
 "d", 
 "d", 
 "d", 
 "d", 
 "d", 

 

 


 "r", 

 






 "r", 
   
 
 "r", 

 



 "r", 

 
 "r", 

 


 "r", 

 





 "e", 

 




 

 "e", 

 
 "e", 

 

 "e", 

 


 "e", 

 



 "e", 

 




















 "e", 

 




 "e", 

 


 "e", 

 







 "e", 

 

 "2", 
 "2", 
 "2", 

 
 



 "2", 

 
 "2", 

 
 "2", 

 
 "2", 

 
 "2", 
 "2", 
 "2", 
 "2", 

 



 "2", 

 

 "2", 

 




 "1", 
 "1", 
 "1", 
 "1", 

 
 "1", 

 

 "2", 

 
 "1", 

 "2", 
 "2", 
 "1", 
 "1", 

 





 "2", 
 "2", 
 "2", 
 "2", 

 
 "2", 
 "2", 
 "2", 
 "2", 
 "1", 

 




 "e", 
 "e", 
 "2", 
 "2", 
 "2", 
 "e", 

 





 "<", 
 "<", 
 "<", 
 "<", 
 "<", 
 "<", 

 
 "2", 
 "<", 
 "1", 
 "2", 

 


 "1", 

 
 "1", 

 
 "1", 

 



 "e", 

 



 "e", 

 

 "e", 

 
 "e", 

 

 "e", 

 

 "2", 

 

 "1", 

 

 "1", 
 "1", 

 


 "e", 
 "e", 
 "e", 
 "e", 

 


 


 "s", 

 

 "s", 

 




 "s", 

 

 "s", 

 


 "s", 

 





# 155 "/a/users/paul/gcc2/cc1/../common/tree.c" 2

};


 





int standard_tree_code_length[] = {
# 1 "/a/users/paul/gcc2/cc1/../common/tree.def" 1
 



















 
 












 



 




 0, 

 



 -1, 

 


 2, 

 
 



 2, 

 
 2, 

 

















 0, 

 

 


































 0, 	 

 







 0, 

 

 0, 

 

 0, 

 




 





 0, 

 

 0, 

 

 0, 

 

 0, 

 



 0, 

 

 0, 

 




 0, 

 
 0, 

 







 
 0, 

 

 0, 

 

 0, 

 
 


 

 0, 

 

 

 0, 	 

 



 0, 

 





 0, 

 



 0, 

 

 

 




 2, 

 
 3, 

 


 3, 

 

 3, 

 

























































 0, 
 0, 
 0, 
 0, 
 0, 
 0, 
 0, 
 0, 

 

 


 2, 

 






 3, 
   
 
 1, 

 



 2, 

 
 1, 

 


 2, 

 





 2, 

 




 

 2, 

 
 2, 

 

 2, 

 


 3, 

 



 3, 

 




















 3, 

 




 3, 

 


 4, 

 







 3, 

 

 2, 
 2, 
 2, 

 
 



 2, 

 
 2, 

 
 2, 

 
 2, 

 
 2, 
 2, 
 2, 
 2, 

 



 2, 

 

 2, 

 




 1, 
 1, 
 1, 
 1, 

 
 1, 

 

 2, 

 
 1, 

 2, 
 2, 
 1, 
 1, 

 





 2, 
 2, 
 2, 
 2, 

 
 2, 
 2, 
 2, 
 2, 
 1, 

 




 2, 
 2, 
 2, 
 2, 
 2, 
 1, 

 





 2, 
 2, 
 2, 
 2, 
 2, 
 2, 

 
 2, 
 2, 
 1, 
 2, 

 


 1, 

 
 1, 

 
 1, 

 



 3, 

 



 2, 

 

 1, 

 
 1, 

 

 1, 

 

 2, 

 

 1, 

 

 1, 
 1, 

 


 2, 
 2, 
 2, 
 2, 

 


 


 1, 

 

 1, 

 




 1, 

 

 1, 

 


 1, 

 





# 166 "/a/users/paul/gcc2/cc1/../common/tree.c" 2

};


 



char *standard_tree_code_name[] = {
# 1 "/a/users/paul/gcc2/cc1/../common/tree.def" 1
 



















 
 












 



 




 "error_mark", 

 



 "identifier_node", 

 


 "op_identifier", 

 
 



 "tree_list", 

 
 "tree_vec", 

 

















 "block", 

 

 


































 "void_type", 	 

 







 "integer_type", 

 

 "real_type", 

 

 "complex_type", 

 




 





 "enumeral_type", 

 

 "boolean_type", 

 

 "char_type", 

 

 "pointer_type", 

 



 "offset_type", 

 

 "reference_type", 

 




 "method_type", 

 
 "file_type", 

 







 
 "array_type", 

 

 "set_type", 

 

 "string_type", 

 
 


 

 "record_type", 

 

 

 "union_type", 	 

 



 "qual_union_type", 

 





 "function_type", 

 



 "lang_type", 

 

 

 




 "integer_cst", 

 
 "real_cst", 

 


 "complex_cst", 

 

 "string_cst", 

 

























































 "function_decl", 
 "label_decl", 
 "const_decl", 
 "type_decl", 
 "var_decl", 
 "parm_decl", 
 "result_decl", 
 "field_decl", 

 

 


 "component_ref", 

 






 "bit_field_ref", 
   
 
 "indirect_ref", 

 



 "offset_ref", 

 
 "buffer_ref", 

 


 "array_ref", 

 





 "constructor", 

 




 

 "compound_expr", 

 
 "modify_expr", 

 

 "init_expr", 

 


 "target_expr", 

 



 "cond_expr", 

 




















 "bind_expr", 

 




 "call_expr", 

 


 "method_call_expr", 

 







 "with_cleanup_expr", 

 

 "plus_expr", 
 "minus_expr", 
 "mult_expr", 

 
 



 "trunc_div_expr", 

 
 "ceil_div_expr", 

 
 "floor_div_expr", 

 
 "round_div_expr", 

 
 "trunc_mod_expr", 
 "ceil_mod_expr", 
 "floor_mod_expr", 
 "round_mod_expr", 

 



 "rdiv_expr", 

 

 "exact_div_expr", 

 




 "fix_trunc_expr", 
 "fix_ceil_expr", 
 "fix_floor_expr", 
 "fix_round_expr", 

 
 "float_expr", 

 

 "expon_expr", 

 
 "negate_expr", 

 "min_expr", 
 "max_expr", 
 "abs_expr", 
 "ffs_expr", 

 





 "alshift_expr", 
 "arshift_expr", 
 "lrotate_expr", 
 "rrotate_expr", 

 
 "bit_ior_expr", 
 "bit_xor_expr", 
 "bit_and_expr", 
 "bit_andtc_expr", 
 "bit_not_expr", 

 




 "truth_andif_expr", 
 "truth_orif_expr", 
 "truth_and_expr", 
 "truth_or_expr", 
 "truth_xor_expr", 
 "truth_not_expr", 

 





 "lt_expr", 
 "le_expr", 
 "gt_expr", 
 "ge_expr", 
 "eq_expr", 
 "ne_expr", 

 
 "in_expr", 
 "set_le_expr", 
 "card_expr", 
 "range_expr", 

 


 "convert_expr", 

 
 "nop_expr", 

 
 "non_lvalue_expr", 

 



 "save_expr", 

 



 "rtl_expr", 

 

 "addr_expr", 

 
 "reference_expr", 

 

 "entry_value_expr", 

 

 "complex_expr", 

 

 "conj_expr", 

 

 "realpart_expr", 
 "imagpart_expr", 

 


 "predecrement_expr", 
 "preincrement_expr", 
 "postdecrement_expr", 
 "postincrement_expr", 

 


 


 "label_expr", 

 

 "goto_expr", 

 




 "return_expr", 

 

 "exit_expr", 

 


 "loop_expr", 

 





# 175 "/a/users/paul/gcc2/cc1/../common/tree.c" 2

};


 



char **tree_code_type;

 



int *tree_code_length;

 

char **tree_code_name;

 
typedef enum
{
  d_kind,
  t_kind,
  b_kind,
  s_kind,
  r_kind,
  e_kind,
  c_kind,
  id_kind,
  op_id_kind,
  perm_list_kind,
  temp_list_kind,
  vec_kind,
  x_kind,
  lang_decl,
  lang_type,
  all_kinds
} tree_node_kind;

int tree_node_counts[(int)all_kinds];
int tree_node_sizes[(int)all_kinds];
int id_string_size = 0;

char *tree_node_kind_names[] = {
  "decls",
  "types",
  "blocks",
  "stmts",
  "refs",
  "exprs",
  "constants",
  "identifiers",
  "op_identifiers",
  "perm_tree_lists",
  "temp_tree_lists",
  "vecs",
  "random kinds",
  "lang_decl kinds",
  "lang_type kinds"
};

 


static tree hash_table[1009 ];	 

 
static int do_identifier_warnings;

 
static int next_decl_uid;
 
static int next_type_uid = 1;

extern char *mode_name[];

void gcc_obstack_init ();
static tree stabilize_reference_1 ();

 

void
init_obstacks ()
{
  gcc_obstack_init (&obstack_stack_obstack);
  gcc_obstack_init (&permanent_obstack);

  gcc_obstack_init (&temporary_obstack);
  temporary_firstobj = (char *)  	({ struct obstack *__h = (&temporary_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( 0));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
  gcc_obstack_init (&momentary_obstack);
  momentary_firstobj = (char *)  	({ struct obstack *__h = (&momentary_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( 0));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
  gcc_obstack_init (&maybepermanent_obstack);
  maybepermanent_firstobj
    = (char *)  	({ struct obstack *__h = (&maybepermanent_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( 0));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
  gcc_obstack_init (&temp_decl_obstack);
  temp_decl_firstobj = (char *)  	({ struct obstack *__h = (&temp_decl_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( 0));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;

  function_obstack = &temporary_obstack;
  function_maybepermanent_obstack = &maybepermanent_obstack;
  current_obstack = &permanent_obstack;
  expression_obstack = &permanent_obstack;
  rtl_obstack = saveable_obstack = &permanent_obstack;

   
  bzero (hash_table, sizeof hash_table);
}

void
gcc_obstack_init (obstack)
     struct obstack *obstack;
{
   



   






  _obstack_begin (obstack, 0 , 0,
		  (void *(*) ()) xmalloc ,
		  (void (*) ()) free );
}

 


void
save_tree_status (p)
     struct function *p;
{
  p->all_types_permanent = all_types_permanent;
  p->momentary_stack = momentary_stack;
  p->maybepermanent_firstobj = maybepermanent_firstobj;
  p->momentary_firstobj = momentary_firstobj;
  p->function_obstack = function_obstack;
  p->function_maybepermanent_obstack = function_maybepermanent_obstack;
  p->current_obstack = current_obstack;
  p->expression_obstack = expression_obstack;
  p->saveable_obstack = saveable_obstack;
  p->rtl_obstack = rtl_obstack;

  function_obstack = (struct obstack *) xmalloc (sizeof (struct obstack));
  gcc_obstack_init (function_obstack);

  function_maybepermanent_obstack
    = (struct obstack *) xmalloc (sizeof (struct obstack));
  gcc_obstack_init (function_maybepermanent_obstack);

  current_obstack = &permanent_obstack;
  expression_obstack = &permanent_obstack;
  rtl_obstack = saveable_obstack = &permanent_obstack;

  momentary_firstobj = (char *)  	({ struct obstack *__o1 = (&momentary_obstack);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ;
  maybepermanent_firstobj
    = (char *)  	({ struct obstack *__o1 = (function_maybepermanent_obstack);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ;
}

 


void
restore_tree_status (p)
     struct function *p;
{
  all_types_permanent = p->all_types_permanent;
  momentary_stack = p->momentary_stack;

   	({ struct obstack *__o = (&momentary_obstack);	void *__obj = ( momentary_firstobj);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;
   	({ struct obstack *__o = (function_obstack);	void *__obj = ( 0);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;
   	({ struct obstack *__o = (function_maybepermanent_obstack);	void *__obj = ( 0);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;
  free (function_obstack);

  momentary_firstobj = p->momentary_firstobj;
  maybepermanent_firstobj = p->maybepermanent_firstobj;
  function_obstack = p->function_obstack;
  function_maybepermanent_obstack = p->function_maybepermanent_obstack;
  current_obstack = p->current_obstack;
  expression_obstack = p->expression_obstack;
  saveable_obstack = p->saveable_obstack;
  rtl_obstack = p->rtl_obstack;
}

 




void
temporary_allocation ()
{
   

  current_obstack = function_obstack;
  expression_obstack = function_obstack;
  rtl_obstack = saveable_obstack = function_maybepermanent_obstack;
  momentary_stack = 0;
}

 



void
end_temporary_allocation ()
{
  current_obstack = &permanent_obstack;
  expression_obstack = &permanent_obstack;
  rtl_obstack = saveable_obstack = &permanent_obstack;
}

 


void
resume_temporary_allocation ()
{
  current_obstack = function_obstack;
  expression_obstack = function_obstack;
  rtl_obstack = saveable_obstack = function_maybepermanent_obstack;
}

 




void
saveable_allocation ()
{
   

  expression_obstack = current_obstack = saveable_obstack;
}

 



void
push_obstacks (current, saveable)
     struct obstack *current, *saveable;
{
  struct obstack_stack *p
    = (struct obstack_stack *)  	({ struct obstack *__h = (&obstack_stack_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( (
					      (sizeof (struct obstack_stack))));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;

  p->current = current_obstack;
  p->saveable = saveable_obstack;
  p->expression = expression_obstack;
  p->rtl = rtl_obstack;
  p->next = obstack_stack;
  obstack_stack = p;

  current_obstack = current;
  expression_obstack = current;
  rtl_obstack = saveable_obstack = saveable;
}

 

void
push_obstacks_nochange ()
{
  struct obstack_stack *p
    = (struct obstack_stack *)  	({ struct obstack *__h = (&obstack_stack_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( (
					      (sizeof (struct obstack_stack))));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;

  p->current = current_obstack;
  p->saveable = saveable_obstack;
  p->expression = expression_obstack;
  p->rtl = rtl_obstack;
  p->next = obstack_stack;
  obstack_stack = p;
}

 

void
pop_obstacks ()
{
  struct obstack_stack *p = obstack_stack;
  obstack_stack = p->next;

  current_obstack = p->current;
  saveable_obstack = p->saveable;
  expression_obstack = p->expression;
  rtl_obstack = p->rtl;

   	({ struct obstack *__o = (&obstack_stack_obstack);	void *__obj = ( p);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;
}

 


int
allocation_temporary_p ()
{
  return current_obstack != &permanent_obstack;
}

 



void
permanent_allocation ()
{
   
   	({ struct obstack *__o = (&temporary_obstack);	void *__obj = ( temporary_firstobj);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;
   	({ struct obstack *__o = (&momentary_obstack);	void *__obj = ( momentary_firstobj);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;
   	({ struct obstack *__o = (&maybepermanent_obstack);	void *__obj = ( maybepermanent_firstobj);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;
   	({ struct obstack *__o = (&temp_decl_obstack);	void *__obj = ( temp_decl_firstobj);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;

  current_obstack = &permanent_obstack;
  expression_obstack = &permanent_obstack;
  rtl_obstack = saveable_obstack = &permanent_obstack;
}

 

void
preserve_data ()
{
  maybepermanent_firstobj
    = (char *)  	({ struct obstack *__h = (function_maybepermanent_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( 0));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
}

void
preserve_initializer ()
{
  temporary_firstobj
    = (char *)  	({ struct obstack *__h = (&temporary_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( 0));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
  momentary_firstobj
    = (char *)  	({ struct obstack *__h = (&momentary_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( 0));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
  maybepermanent_firstobj
    = (char *)  	({ struct obstack *__h = (function_maybepermanent_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( 0));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
}

 



void
rtl_in_current_obstack ()
{
  rtl_obstack = current_obstack;
}

 


void
rtl_in_saveable_obstack ()
{
  rtl_obstack = saveable_obstack;
}

 



char *
oballoc (size)
     int size;
{
  return (char *)  	({ struct obstack *__h = (current_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( size));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
}

 



void
obfree (ptr)
     char *ptr;
{
   	({ struct obstack *__o = (current_obstack);	void *__obj = ( ptr);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;
}

 


char *
permalloc (size)
     int size;
{
  return (char *)  	({ struct obstack *__h = (&permanent_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( size));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
}

 



char *
perm_calloc (nelem, size)
     int nelem;
     long size;
{
  char *rval = (char *)  	({ struct obstack *__h = (&permanent_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( nelem * size));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
  bzero (rval, nelem * size);
  return rval;
}

 


char *
savealloc (size)
     int size;
{
  return (char *)  	({ struct obstack *__h = (saveable_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( size));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
}

 

void
debug_obstack (object)
     char *object;
{
  struct obstack *obstack = 0 ;
  char *obstack_name = 0 ;
  struct function *p;

  for (p = outer_function_chain; p; p = p->next)
    {
      if (_obstack_allocated_p (p->function_obstack, object))
	{
	  obstack = p->function_obstack;
	  obstack_name = "containing function obstack";
	}
      if (_obstack_allocated_p (p->function_maybepermanent_obstack, object))
	{
	  obstack = p->function_maybepermanent_obstack;
	  obstack_name = "containing function maybepermanent obstack";
	}
    }

  if (_obstack_allocated_p (&obstack_stack_obstack, object))
    {
      obstack = &obstack_stack_obstack;
      obstack_name = "obstack_stack_obstack";
    }
  else if (_obstack_allocated_p (function_obstack, object))
    {
      obstack = function_obstack;
      obstack_name = "function obstack";
    }
  else if (_obstack_allocated_p (&permanent_obstack, object))
    {
      obstack = &permanent_obstack;
      obstack_name = "permanent_obstack";
    }
  else if (_obstack_allocated_p (&momentary_obstack, object))
    {
      obstack = &momentary_obstack;
      obstack_name = "momentary_obstack";
    }
  else if (_obstack_allocated_p (function_maybepermanent_obstack, object))
    {
      obstack = function_maybepermanent_obstack;
      obstack_name = "function maybepermanent obstack";
    }
  else if (_obstack_allocated_p (&temp_decl_obstack, object))
    {
      obstack = &temp_decl_obstack;
      obstack_name = "temp_decl_obstack";
    }

   
  if (obstack != 0 )
    {
      if (object >= obstack->next_free
	  && object < obstack->chunk_limit)
	fprintf ((&__sF[2]) , "object in free portion of obstack %s.\n",
		 obstack_name);
      else
	fprintf ((&__sF[2]) , "object allocated from %s.\n", obstack_name);
    }
  else
    fprintf ((&__sF[2]) , "object not allocated from any obstack.\n");
}

 



int
object_permanent_p (obj)
     tree obj;
{
  return _obstack_allocated_p (&permanent_obstack, obj);
}

 




void
push_momentary ()
{
  struct momentary_level *tem
    = (struct momentary_level *)  	({ struct obstack *__h = (&momentary_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( (
						sizeof (struct momentary_level)));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
  tem->prev = momentary_stack;
  tem->base = (char *) ((&momentary_obstack)->object_base) ;
  tem->obstack = expression_obstack;
  momentary_stack = tem;
  expression_obstack = &momentary_obstack;
}

 


void
clear_momentary ()
{
   	({ struct obstack *__o = (&momentary_obstack);	void *__obj = ( momentary_stack->base);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;
}

 




void
pop_momentary ()
{
  struct momentary_level *tem = momentary_stack;
  momentary_stack = tem->prev;
  expression_obstack = tem->obstack;
   	({ struct obstack *__o = (&momentary_obstack);	void *__obj = ( tem);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;
}

 



int
suspend_momentary ()
{
  register int tem = expression_obstack == &momentary_obstack;
  expression_obstack = saveable_obstack;
  return tem;
}

 




void
resume_momentary (yes)
     int yes;
{
  if (yes)
    expression_obstack = &momentary_obstack;
}

 


void
init_tree_codes ()
{
  tree_code_type = (char **) xmalloc (sizeof (standard_tree_code_type));
  tree_code_length = (int *) xmalloc (sizeof (standard_tree_code_length));
  tree_code_name = (char **) xmalloc (sizeof (standard_tree_code_name));
  bcopy (standard_tree_code_type, tree_code_type,
	 sizeof (standard_tree_code_type));
  bcopy (standard_tree_code_length, tree_code_length,
	 sizeof (standard_tree_code_length));
  bcopy (standard_tree_code_name, tree_code_name,
	 sizeof (standard_tree_code_name));
}

 






tree
make_node (code)
     enum tree_code code;
{
  register tree t;
  register int type = (*tree_code_type[(int) (code)]) ;
  register int length;
  register struct obstack *obstack = current_obstack;
  register int i;
  register tree_node_kind kind;

  switch (type)
    {
    case 'd':   



      length = sizeof (struct tree_decl);
       
      if (obstack != &permanent_obstack)
	obstack = saveable_obstack;
       


      else if (code == PARM_DECL)
	obstack = function_maybepermanent_obstack;
      break;

    case 't':   



      length = sizeof (struct tree_type);
       
      if (obstack != &permanent_obstack)
	obstack = all_types_permanent ? &permanent_obstack : saveable_obstack;
      break;

    case 'b':   



      length = sizeof (struct tree_block);
       
      if (obstack != &permanent_obstack)
	obstack = saveable_obstack;
      break;

    case 's':   




    case 'r':   




    case 'e':   
    case '<':   
    case '1':   
    case '2':   




      obstack = expression_obstack;
       
      if (code == BIND_EXPR && obstack != &permanent_obstack)
	obstack = saveable_obstack;
      length = sizeof (struct tree_exp)
	+ (tree_code_length[(int) code] - 1) * sizeof (char *);
      break;

    case 'c':   



      obstack = expression_obstack;

       





      if (code == INTEGER_CST)
	length = sizeof (struct tree_int_cst);
      else if (code == REAL_CST)
	length = sizeof (struct tree_real_cst);
      else
	length = sizeof (struct tree_common)
	  + tree_code_length[(int) code] * sizeof (char *);
      break;

    case 'x':   
# 869 "/a/users/paul/gcc2/cc1/../common/tree.c"

      length = sizeof (struct tree_common)
	+ tree_code_length[(int) code] * sizeof (char *);
       

      if (code == IDENTIFIER_NODE) obstack = &permanent_obstack;
    }

  t = (tree)  	({ struct obstack *__h = (obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( length));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;






   
  for (i = (length / sizeof (int)) - 1; i >= 0; i--)
    ((int *) t)[i] = 0;
   
  for (i = length / sizeof (int) * sizeof (int); i < length; i++)
    ((char *) t)[i] = 0;

  ((t)->common.code = (int) ( code)) ;
  if (obstack == &permanent_obstack)
    ((t)->common.permanent_flag)  = 1;

  switch (type)
    {
    case 's':
      ((t)->common.side_effects_flag)  = 1;
      ((t)->common.type)  = void_type_node;
      break;

    case 'd':
      if (code != FUNCTION_DECL)
	((t)->decl.frame_size)  = 1;
      ((t)->decl.in_system_header_flag) 
	= in_system_header && (obstack == &permanent_obstack);
      ((t)->decl.linenum)  = lineno;
      ((t)->decl.filename)  = (input_filename) ? input_filename : "<built-in>";
      ((t)->decl.uid)  = next_decl_uid++;
      break;

    case 't':
      ((t)->type.uid)  = next_type_uid++;
      ((t)->type.align)  = 1;
      ((t)->type.main_variant)  = t;
      break;

    case 'c':
      ((t)->common.constant_flag)  = 1;
      break;
    }

  return t;
}

 


tree
copy_node (node)
     tree node;
{
  register tree t;
  register enum tree_code code = ((enum tree_code) (node)->common.code) ;
  register int length;
  register int i;

  switch ((*tree_code_type[(int) (code)]) )
    {
    case 'd':   
      length = sizeof (struct tree_decl);
      break;

    case 't':   
      length = sizeof (struct tree_type);
      break;

    case 'b':   
      length = sizeof (struct tree_block);
      break;

    case 'r':   
    case 'e':   
    case 's':   
    case '<':   
    case '1':   
    case '2':   
      length = sizeof (struct tree_exp)
	+ (tree_code_length[(int) code] - 1) * sizeof (char *);
      break;

    case 'c':   
       

      if (code == REAL_CST)
	{
	  length = sizeof (struct tree_real_cst);
	  break;
	}

    case 'x':   
      length = sizeof (struct tree_common)
	+ tree_code_length[(int) code] * sizeof (char *);
      if (code == TREE_VEC)
	length += (((node)->vec.length)  - 1) * sizeof (char *);
    }

  t = (tree)  	({ struct obstack *__h = (current_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( length));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;

  for (i = (length / sizeof (int)) - 1; i >= 0; i--)
    ((int *) t)[i] = ((int *) node)[i];
   
  for (i = length / sizeof (int) * sizeof (int); i < length; i++)
    ((char *) t)[i] = ((char *) node)[i];

  ((t)->common.chain)  = 0;

  if ((*tree_code_type[(int) (code)])  == 'd')
    ((t)->decl.uid)  = next_decl_uid++;
  else if ((*tree_code_type[(int) (code)])  == 't')
    ((t)->type.uid)  = next_type_uid++;

  ((t)->common.permanent_flag)  = (current_obstack == &permanent_obstack);

  return t;
}

 


tree
copy_list (list)
     tree list;
{
  tree head;
  register tree prev, next;

  if (list == 0)
    return 0;

  head = prev = copy_node (list);
  next = ((list)->common.chain) ;
  while (next)
    {
      ((prev)->common.chain)  = copy_node (next);
      prev = ((prev)->common.chain) ;
      next = ((next)->common.chain) ;
    }
  return head;
}



 



tree
get_identifier (text)
     register char *text;
{
  register int hi;
  register int i;
  register tree idp;
  register int len, hash_len;

   
  for (len = 0; text[len]; len++);

   
  hash_len = len;
  if (warn_id_clash && len > id_clash_len)
    hash_len = id_clash_len;

   
  hi = hash_len * 613 + (unsigned)text[0];
  for (i = 1; i < hash_len; i += 2)
    hi = ((hi * 613) + (unsigned)(text[i]));

  hi &= (1 << 30 ) - 1;
  hi %= 1009 ;
  
   
  for (idp = hash_table[hi]; idp; idp = ((idp)->common.chain) )
    if (((idp)->identifier.length)  == len
	&& ((idp)->identifier.pointer) [0] == text[0]
	&& !bcmp (((idp)->identifier.pointer) , text, len))
      return idp;		 

   
  if (warn_id_clash && do_identifier_warnings && len >= id_clash_len)
    for (idp = hash_table[hi]; idp; idp = ((idp)->common.chain) )
      if (!strncmp (((idp)->identifier.pointer) , text, id_clash_len))
	{
	  warning ("`%s' and `%s' identical in first %d characters",
		   ((idp)->identifier.pointer) , text, id_clash_len);
	  break;
	}

  if (tree_code_length[(int) IDENTIFIER_NODE] < 0)
    abort ();			 

   
  idp = make_node (IDENTIFIER_NODE);
  ((idp)->identifier.length)  = len;




  ((idp)->identifier.pointer)  =  	({ struct obstack *__h = (&permanent_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( len));	((__o->next_free + __len + 1 > __o->chunk_limit)	? (_obstack_newchunk (__o, __len + 1), 0) : 0),	bcopy ( ( text), __o->next_free, __len),	__o->next_free += __len,	*(__o->next_free)++ = 0;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;

  ((idp)->common.chain)  = hash_table[hi];
  hash_table[hi] = idp;
  return idp;			 
}

 


void
start_identifier_warnings ()
{
  do_identifier_warnings = 1;
}

 




void
set_identifier_size (size)
     int size;
{
  tree_code_length[(int) IDENTIFIER_NODE]
    = (size - sizeof (struct tree_common)) / sizeof (tree);
}

 





tree
build_int_2_wide (low, hi)
     int  low, hi;
{
  register tree t = make_node (INTEGER_CST);
  ((t)->int_cst.int_cst_low)  = low;
  ((t)->int_cst.int_cst_high)  = hi;
  ((t)->common.type)  = integer_type_node;
  return t;
}

 

tree
build_real (type, d)
     tree type;
     double  d;
{
  tree v;

   





  v = make_node (REAL_CST);
  ((v)->common.type)  = type;
  ((v)->real_cst.real_cst)  = d;
  return v;
}

 




double 
real_value_from_int_cst (i)
     tree i;
{
  double  d;
  double  e;
   








  if (((i)->int_cst.int_cst_high)  < 0 && ! ((((i)->common.type) )->common.unsigned_flag) )
    {
      d = (double) (~ ((i)->int_cst.int_cst_high) );
      e = ((double) ((int ) 1 << (32   / 2))
	    * (double) ((int ) 1 << (32   / 2)));
      d *= e;
      e = (double) (unsigned int ) (~ ((i)->int_cst.int_cst_low) );
      d += e;
      d = (- d - 1.0);
    }
  else
    {
      d = (double) (unsigned int ) ((i)->int_cst.int_cst_high) ;
      e = ((double) ((int ) 1 << (32   / 2))
	    * (double) ((int ) 1 << (32   / 2)));
      d *= e;
      e = (double) (unsigned int ) ((i)->int_cst.int_cst_low) ;
      d += e;
    }

  return d;
}

 


tree
build_real_from_int_cst (type, i)
     tree type;
     tree i;
{
  tree v;
  double  d;

  v = make_node (REAL_CST);
  ((v)->common.type)  = type;

  d = ( (8  * mode_size[(int)(((type)->type.mode) )])  == sizeof (float) * 8 	? (float) ( real_value_from_int_cst (i)) : ( real_value_from_int_cst (i))) ;
   





  ((v)->real_cst.real_cst)  = d;
  return v;
}



 



tree
build_string (len, str)
     int len;
     char *str;
{
  register tree s = make_node (STRING_CST);
  ((s)->string.length)  = len;
  ((s)->string.pointer)  =  	({ struct obstack *__h = (saveable_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( len));	((__o->next_free + __len + 1 > __o->chunk_limit)	? (_obstack_newchunk (__o, __len + 1), 0) : 0),	bcopy ( ( str), __o->next_free, __len),	__o->next_free += __len,	*(__o->next_free)++ = 0;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;
  return s;
}

 




tree
build_complex (real, imag)
     tree real, imag;
{
  register tree t = make_node (COMPLEX_CST);
  ((t)->complex.real)  = real;
  ((t)->complex.imag)  = imag;
  ((t)->common.type)  = build_complex_type (((real)->common.type) );
  return t;
}

 
tree
make_tree_vec (len)
     int len;
{
  register tree t;
  register int length = (len-1) * sizeof (tree) + sizeof (struct tree_vec);
  register struct obstack *obstack = current_obstack;
  register int i;






  t = (tree)  	({ struct obstack *__h = (obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( length));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;

  for (i = (length / sizeof (int)) - 1; i >= 0; i--)
    ((int *) t)[i] = 0;

  ((t)->common.code = (int) ( TREE_VEC)) ;
  ((t)->vec.length)  = len;
  if (obstack == &permanent_obstack)
    ((t)->common.permanent_flag)  = 1;

  return t;
}

 

int
integer_zerop (expr)
     tree expr;
{
  while ((((enum tree_code) (expr)->common.code)  == NOP_EXPR	|| ((enum tree_code) (expr)->common.code)  == CONVERT_EXPR	|| ((enum tree_code) (expr)->common.code)  == NON_LVALUE_EXPR)	&& (((((expr)->common.type) )->type.mode) 	== ((((((expr)->exp.operands[ 0]) )->common.type) )->type.mode) ))	(expr) = ((expr)->exp.operands[ 0]) ; ;

  return (((enum tree_code) (expr)->common.code)  == INTEGER_CST
	  && ((expr)->int_cst.int_cst_low)  == 0
	  && ((expr)->int_cst.int_cst_high)  == 0);
}

 

int
integer_onep (expr)
     tree expr;
{
  while ((((enum tree_code) (expr)->common.code)  == NOP_EXPR	|| ((enum tree_code) (expr)->common.code)  == CONVERT_EXPR	|| ((enum tree_code) (expr)->common.code)  == NON_LVALUE_EXPR)	&& (((((expr)->common.type) )->type.mode) 	== ((((((expr)->exp.operands[ 0]) )->common.type) )->type.mode) ))	(expr) = ((expr)->exp.operands[ 0]) ; ;

  return (((enum tree_code) (expr)->common.code)  == INTEGER_CST
	  && ((expr)->int_cst.int_cst_low)  == 1
	  && ((expr)->int_cst.int_cst_high)  == 0);
}

 


int
integer_all_onesp (expr)
     tree expr;
{
  register int prec;
  register int uns;

  while ((((enum tree_code) (expr)->common.code)  == NOP_EXPR	|| ((enum tree_code) (expr)->common.code)  == CONVERT_EXPR	|| ((enum tree_code) (expr)->common.code)  == NON_LVALUE_EXPR)	&& (((((expr)->common.type) )->type.mode) 	== ((((((expr)->exp.operands[ 0]) )->common.type) )->type.mode) ))	(expr) = ((expr)->exp.operands[ 0]) ; ;

  if (((enum tree_code) (expr)->common.code)  != INTEGER_CST)
    return 0;

  uns = ((((expr)->common.type) )->common.unsigned_flag) ;
  if (!uns)
    return ((expr)->int_cst.int_cst_low)  == -1 && ((expr)->int_cst.int_cst_high)  == -1;

  prec = ((((expr)->common.type) )->type.precision) ;
  if (prec >= 32  )
    {
      int high_value, shift_amount;

      shift_amount = prec - 32  ;

      if (shift_amount > 32  )
	 
	abort ();
      else if (shift_amount == 32  )
	 

	high_value = -1;
      else
	high_value = ((int ) 1 << shift_amount) - 1;

      return ((expr)->int_cst.int_cst_low)  == -1
	&& ((expr)->int_cst.int_cst_high)  == high_value;
    }
  else
    return ((expr)->int_cst.int_cst_low)  == ((int ) 1 << prec) - 1;
}

 


int
integer_pow2p (expr)
     tree expr;
{
  int  high, low;

  while ((((enum tree_code) (expr)->common.code)  == NOP_EXPR	|| ((enum tree_code) (expr)->common.code)  == CONVERT_EXPR	|| ((enum tree_code) (expr)->common.code)  == NON_LVALUE_EXPR)	&& (((((expr)->common.type) )->type.mode) 	== ((((((expr)->exp.operands[ 0]) )->common.type) )->type.mode) ))	(expr) = ((expr)->exp.operands[ 0]) ; ;

  if (((enum tree_code) (expr)->common.code)  != INTEGER_CST)
    return 0;

  high = ((expr)->int_cst.int_cst_high) ;
  low = ((expr)->int_cst.int_cst_low) ;

  if (high == 0 && low == 0)
    return 0;

  return ((high == 0 && (low & (low - 1)) == 0)
	  || (low == 0 && (high & (high - 1)) == 0));
}

 

int
real_zerop (expr)
     tree expr;
{
  while ((((enum tree_code) (expr)->common.code)  == NOP_EXPR	|| ((enum tree_code) (expr)->common.code)  == CONVERT_EXPR	|| ((enum tree_code) (expr)->common.code)  == NON_LVALUE_EXPR)	&& (((((expr)->common.type) )->type.mode) 	== ((((((expr)->exp.operands[ 0]) )->common.type) )->type.mode) ))	(expr) = ((expr)->exp.operands[ 0]) ; ;

  return (((enum tree_code) (expr)->common.code)  == REAL_CST
	  && ((((expr)->real_cst.real_cst) ) == ( dconst0)) );
}

 

int
real_onep (expr)
     tree expr;
{
  while ((((enum tree_code) (expr)->common.code)  == NOP_EXPR	|| ((enum tree_code) (expr)->common.code)  == CONVERT_EXPR	|| ((enum tree_code) (expr)->common.code)  == NON_LVALUE_EXPR)	&& (((((expr)->common.type) )->type.mode) 	== ((((((expr)->exp.operands[ 0]) )->common.type) )->type.mode) ))	(expr) = ((expr)->exp.operands[ 0]) ; ;

  return (((enum tree_code) (expr)->common.code)  == REAL_CST
	  && ((((expr)->real_cst.real_cst) ) == ( dconst1)) );
}

 

int
real_twop (expr)
     tree expr;
{
  while ((((enum tree_code) (expr)->common.code)  == NOP_EXPR	|| ((enum tree_code) (expr)->common.code)  == CONVERT_EXPR	|| ((enum tree_code) (expr)->common.code)  == NON_LVALUE_EXPR)	&& (((((expr)->common.type) )->type.mode) 	== ((((((expr)->exp.operands[ 0]) )->common.type) )->type.mode) ))	(expr) = ((expr)->exp.operands[ 0]) ; ;

  return (((enum tree_code) (expr)->common.code)  == REAL_CST
	  && ((((expr)->real_cst.real_cst) ) == ( dconst2)) );
}

 
 
int
really_constant_p (exp)
     tree exp;
{
   
  while (((enum tree_code) (exp)->common.code)  == NOP_EXPR
	 || ((enum tree_code) (exp)->common.code)  == CONVERT_EXPR
	 || ((enum tree_code) (exp)->common.code)  == NON_LVALUE_EXPR)
    exp = ((exp)->exp.operands[ 0]) ;
  return ((exp)->common.constant_flag) ;
}

 


tree
value_member (elem, list)
     tree elem, list;
{
  while (list)
    {
      if (elem == ((list)->list.value) )
	return list;
      list = ((list)->common.chain) ;
    }
  return (tree) 0  ;
}

 


tree
purpose_member (elem, list)
     tree elem, list;
{
  while (list)
    {
      if (elem == ((list)->list.purpose) )
	return list;
      list = ((list)->common.chain) ;
    }
  return (tree) 0  ;
}

 


tree
binfo_member (elem, list)
     tree elem, list;
{
  while (list)
    {
      if (elem == ((list)->common.type)  )
	return list;
      list = ((list)->common.chain) ;
    }
  return (tree) 0  ;
}

 

int
chain_member (elem, chain)
     tree elem, chain;
{
  while (chain)
    {
      if (elem == chain)
	return 1;
      chain = ((chain)->common.chain) ;
    }

  return 0;
}

 



int
list_length (t)
     tree t;
{
  register tree tail;
  register int len = 0;

  for (tail = t; tail; tail = ((tail)->common.chain) )
    len++;

  return len;
}

 



tree
chainon (op1, op2)
     tree op1, op2;
{
  tree t;

  if (op1)
    {
      for (t = op1; ((t)->common.chain) ; t = ((t)->common.chain) )
	if (t == op2) abort ();	 
      if (t == op2) abort ();	 
      ((t)->common.chain)  = op2;
      return op1;
    }
  else return op2;
}

 

tree
tree_last (chain)
     register tree chain;
{
  register tree next;
  if (chain)
    while (next = ((chain)->common.chain) )
      chain = next;
  return chain;
}

 


tree
nreverse (t)
     tree t;
{
  register tree prev = 0, decl, next;
  for (decl = t; decl; decl = next)
    {
      next = ((decl)->common.chain) ;
      ((decl)->common.chain)  = prev;
      prev = decl;
    }
  return prev;
}

 


tree
listify (chain)
     tree chain;
{
  tree result = (tree) 0  ;
  tree in_tail = chain;
  tree out_tail = (tree) 0  ;

  while (in_tail)
    {
      tree next = tree_cons ((tree) 0  , in_tail, (tree) 0  );
      if (out_tail)
	((out_tail)->common.chain)  = next;
      else
	result = next;
      out_tail = next;
      in_tail = ((in_tail)->common.chain) ;
    }

  return result;
}

 


tree
build_tree_list (parm, value)
     tree parm, value;
{
  register tree t = make_node (TREE_LIST);
  ((t)->list.purpose)  = parm;
  ((t)->list.value)  = value;
  return t;
}

 

tree
build_decl_list (parm, value)
     tree parm, value;
{
  register tree node;
  register struct obstack *ambient_obstack = current_obstack;
  current_obstack = &temp_decl_obstack;
  node = build_tree_list (parm, value);
  current_obstack = ambient_obstack;
  return node;
}

 



tree
tree_cons (purpose, value, chain)
     tree purpose, value, chain;
{



  register int i;
  register tree node = (tree)  	({ struct obstack *__h = (current_obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( sizeof (struct tree_list)));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;





  for (i = (sizeof (struct tree_common) / sizeof (int)) - 1; i >= 0; i--)
    ((int *) node)[i] = 0;

  ((node)->common.code = (int) ( TREE_LIST)) ;
  if (current_obstack == &permanent_obstack)
    ((node)->common.permanent_flag)  = 1;


  ((node)->common.chain)  = chain;
  ((node)->list.purpose)  = purpose;
  ((node)->list.value)  = value;
  return node;
}

 

tree
decl_tree_cons (purpose, value, chain)
     tree purpose, value, chain;
{
  register tree node;
  register struct obstack *ambient_obstack = current_obstack;
  current_obstack = &temp_decl_obstack;
  node = tree_cons (purpose, value, chain);
  current_obstack = ambient_obstack;
  return node;
}

 

tree
perm_tree_cons (purpose, value, chain)
     tree purpose, value, chain;
{
  register tree node;
  register struct obstack *ambient_obstack = current_obstack;
  current_obstack = &permanent_obstack;

  node = tree_cons (purpose, value, chain);
  current_obstack = ambient_obstack;
  return node;
}

 

tree
temp_tree_cons (purpose, value, chain)
     tree purpose, value, chain;
{
  register tree node;
  register struct obstack *ambient_obstack = current_obstack;
  current_obstack = &temporary_obstack;

  node = tree_cons (purpose, value, chain);
  current_obstack = ambient_obstack;
  return node;
}

 

tree
saveable_tree_cons (purpose, value, chain)
     tree purpose, value, chain;
{
  register tree node;
  register struct obstack *ambient_obstack = current_obstack;
  current_obstack = saveable_obstack;

  node = tree_cons (purpose, value, chain);
  current_obstack = ambient_obstack;
  return node;
}

 





tree
size_in_bytes (type)
     tree type;
{
  tree t;

  if (type == error_mark_node)
    return integer_zero_node;
  type = ((type)->type.main_variant) ;
  if (((type)->type.size)  == 0)
    {
      incomplete_type_error ((tree) 0  , type);
      return integer_zero_node;
    }
  t = size_binop (CEIL_DIV_EXPR, ((type)->type.size) ,
		  size_int (8 ));
  if (((enum tree_code) (t)->common.code)  == INTEGER_CST)
    force_fit_type (t, 0);
  return t;
}

 


int
int_size_in_bytes (type)
     tree type;
{
  unsigned int size;
  if (type == error_mark_node)
    return 0;
  type = ((type)->type.main_variant) ;
  if (((type)->type.size)  == 0)
    return -1;
  if (((enum tree_code) (((type)->type.size) )->common.code)  != INTEGER_CST)
    return -1;
  if (((((type)->type.size) )->int_cst.int_cst_high)  != 0)
    {
      tree t = size_binop (CEIL_DIV_EXPR, ((type)->type.size) ,
			   size_int (8 ));
      return ((t)->int_cst.int_cst_low) ;
    }
  size = ((((type)->type.size) )->int_cst.int_cst_low) ;
  return (size + 8  - 1) / 8 ;
}

 



tree
array_type_nelts (type)
     tree type;
{
  tree index_type = ((type)->type.values) ;
  return (tree_int_cst_equal (((index_type)->type.minval) , integer_zero_node)
	  ? ((index_type)->type.maxval) 
	  : fold (build (MINUS_EXPR, integer_type_node,
			 ((index_type)->type.maxval) ,
			 ((index_type)->type.minval) )));
}

 


int
staticp (arg)
     tree arg;
{
  switch (((enum tree_code) (arg)->common.code) )
    {
    case VAR_DECL:
    case FUNCTION_DECL:
    case CONSTRUCTOR:
      return ((arg)->common.static_flag)  || ((arg)->decl.external_flag) ;

    case STRING_CST:
      return 1;

    case COMPONENT_REF:
    case BIT_FIELD_REF:
      return staticp (((arg)->exp.operands[ 0]) );

    case INDIRECT_REF:
      return ((((arg)->exp.operands[ 0]) )->common.constant_flag) ;

    case ARRAY_REF:
      if (((enum tree_code) (((((arg)->common.type) )->type.size) )->common.code)  == INTEGER_CST
	  && ((enum tree_code) (((arg)->exp.operands[ 1]) )->common.code)  == INTEGER_CST)
	return staticp (((arg)->exp.operands[ 0]) );
    }

  return 0;
}

 




tree
save_expr (expr)
     tree expr;
{
  register tree t = fold (expr);

   

  while (((enum tree_code) (t)->common.code)  == NON_LVALUE_EXPR)
    t = ((t)->exp.operands[ 0]) ;

   





  if (((t)->common.constant_flag)  || (((t)->common.readonly_flag)  && ! ((t)->common.side_effects_flag) )
      || ((enum tree_code) (t)->common.code)  == SAVE_EXPR)
    return t;

  t = build (SAVE_EXPR, ((expr)->common.type) , t, current_function_decl, (tree) 0  );

   


  ((t)->common.side_effects_flag)  = 1;
  return t;
}

 






tree
stabilize_reference (ref)
     tree ref;
{
  register tree result;
  register enum tree_code code = ((enum tree_code) (ref)->common.code) ;

  switch (code)
    {
    case VAR_DECL:
    case PARM_DECL:
    case RESULT_DECL:
       
      return ref;

    case NOP_EXPR:
    case CONVERT_EXPR:
    case FLOAT_EXPR:
    case FIX_TRUNC_EXPR:
    case FIX_FLOOR_EXPR:
    case FIX_ROUND_EXPR:
    case FIX_CEIL_EXPR:
      result = build_nt (code, stabilize_reference (((ref)->exp.operands[ 0]) ));
      break;

    case INDIRECT_REF:
      result = build_nt (INDIRECT_REF,
			 stabilize_reference_1 (((ref)->exp.operands[ 0]) ));
      break;

    case COMPONENT_REF:
      result = build_nt (COMPONENT_REF,
			 stabilize_reference (((ref)->exp.operands[ 0]) ),
			 ((ref)->exp.operands[ 1]) );
      break;

    case BIT_FIELD_REF:
      result = build_nt (BIT_FIELD_REF,
			 stabilize_reference (((ref)->exp.operands[ 0]) ),
			 stabilize_reference_1 (((ref)->exp.operands[ 1]) ),
			 stabilize_reference_1 (((ref)->exp.operands[ 2]) ));
      break;

    case ARRAY_REF:
      result = build_nt (ARRAY_REF,
			 stabilize_reference (((ref)->exp.operands[ 0]) ),
			 stabilize_reference_1 (((ref)->exp.operands[ 1]) ));
      break;

       

    default:
      return ref;

    case ERROR_MARK:
      return error_mark_node;
    }

  ((result)->common.type)  = ((ref)->common.type) ;
  ((result)->common.readonly_flag)  = ((ref)->common.readonly_flag) ;
  ((result)->common.side_effects_flag)  = ((ref)->common.side_effects_flag) ;
  ((result)->common.volatile_flag)  = ((ref)->common.volatile_flag) ;
  ((result)->common.raises_flag)  = ((ref)->common.raises_flag) ;

  return result;
}

 












static tree
stabilize_reference_1 (e)
     tree e;
{
  register tree result;
  register int length;
  register enum tree_code code = ((enum tree_code) (e)->common.code) ;

   




  if (((e)->common.constant_flag)  || code == SAVE_EXPR)
    return e;

  switch ((*tree_code_type[(int) (code)]) )
    {
    case 'x':
    case 't':
    case 'd':
    case 'b':
    case '<':
    case 's':
    case 'e':
    case 'r':
       

       

      if (((e)->common.side_effects_flag) )
	return save_expr (e);
      return e;

    case 'c':
       

      return e;
      
    case '2':
       


      if (code == TRUNC_DIV_EXPR || code == TRUNC_MOD_EXPR
	  || code == FLOOR_DIV_EXPR || code == FLOOR_MOD_EXPR
	  || code == CEIL_DIV_EXPR || code == CEIL_MOD_EXPR
	  || code == ROUND_DIV_EXPR || code == ROUND_MOD_EXPR)
	return save_expr (e);
       
      result = build_nt (code, stabilize_reference_1 (((e)->exp.operands[ 0]) ),
			 stabilize_reference_1 (((e)->exp.operands[ 1]) ));
      break;

    case '1':
       
      result = build_nt (code, stabilize_reference_1 (((e)->exp.operands[ 0]) ));
      break;
    }
  
  ((result)->common.type)  = ((e)->common.type) ;
  ((result)->common.readonly_flag)  = ((e)->common.readonly_flag) ;
  ((result)->common.side_effects_flag)  = ((e)->common.side_effects_flag) ;
  ((result)->common.volatile_flag)  = ((e)->common.volatile_flag) ;
  ((result)->common.raises_flag)  = ((e)->common.raises_flag) ;

  return result;
}

 

 




tree
build ( __builtin_va_alist )
        int __builtin_va_alist;   
{
  va_list p;
  enum tree_code code;
  register tree t;
  register int length;
  register int i;

   p=(char *) &__builtin_va_alist ;

  code = (p = (__gnuc_va_list) ((char *) (p) + (((sizeof ( enum tree_code) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ),	*(( enum tree_code *) (void *) ((char *) (p) - (((sizeof ( enum tree_code) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ))) ;
  t = make_node (code);
  length = tree_code_length[(int) code];
  ((t)->common.type)  = (p = (__gnuc_va_list) ((char *) (p) + (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ),	*(( tree *) (void *) ((char *) (p) - (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ))) ;

  if (length == 2)
    {
       
      register tree arg0 = (p = (__gnuc_va_list) ((char *) (p) + (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ),	*(( tree *) (void *) ((char *) (p) - (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ))) ;
      register tree arg1 = (p = (__gnuc_va_list) ((char *) (p) + (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ),	*(( tree *) (void *) ((char *) (p) - (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ))) ;
      ((t)->exp.operands[ 0])  = arg0;
      ((t)->exp.operands[ 1])  = arg1;
      if ((arg0 && ((arg0)->common.side_effects_flag) )
	  || (arg1 && ((arg1)->common.side_effects_flag) ))
	((t)->common.side_effects_flag)  = 1;
      ((t)->common.raises_flag) 
	= (arg0 && ((arg0)->common.raises_flag) ) || (arg1 && ((arg1)->common.raises_flag) );
    }
  else if (length == 1)
    {
      register tree arg0 = (p = (__gnuc_va_list) ((char *) (p) + (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ),	*(( tree *) (void *) ((char *) (p) - (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ))) ;

       
      if ((*tree_code_type[(int) (code)])  != 's')
	abort ();
      ((t)->exp.operands[ 0])  = arg0;
      if (arg0 && ((arg0)->common.side_effects_flag) )
	((t)->common.side_effects_flag)  = 1;
      ((t)->common.raises_flag)  = (arg0 && ((arg0)->common.raises_flag) );
    }
  else
    {
      for (i = 0; i < length; i++)
	{
	  register tree operand = (p = (__gnuc_va_list) ((char *) (p) + (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ),	*(( tree *) (void *) ((char *) (p) - (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ))) ;
	  ((t)->exp.operands[ i])  = operand;
	  if (operand)
	    {
	      if (((operand)->common.side_effects_flag) )
		((t)->common.side_effects_flag)  = 1;
	      if (((operand)->common.raises_flag) )
		((t)->common.raises_flag)  = 1;
	    }
	}
    }
   ;
  return t;
}

 


tree
build1 (code, type, node)
     enum tree_code code;
     tree type;
     tree node;
{
  register struct obstack *obstack = current_obstack;
  register int i, length;
  register tree_node_kind kind;
  register tree t;








  obstack = expression_obstack;
  length = sizeof (struct tree_exp);

  t = (tree)  	({ struct obstack *__h = (obstack);	 	({ struct obstack *__o = (__h);	int __len = ( ( length));	((__o->chunk_limit - __o->next_free < __len)	? (_obstack_newchunk (__o, __len), 0) : 0);	__o->next_free += __len;	(void) 0; }) ;	 	({ struct obstack *__o1 = (__h);	void *value = (void *) __o1->object_base;	if (__o1->next_free == value)	__o1->maybe_empty_object = 1;	__o1->next_free	= (((((__o1->next_free) - (char *)0) +__o1->alignment_mask)	& ~ (__o1->alignment_mask)) + (char *)0) ;	((__o1->next_free - (char *)__o1->chunk	> __o1->chunk_limit - (char *)__o1->chunk)	? (__o1->next_free = __o1->chunk_limit) : 0);	__o1->object_base = __o1->next_free;	value; }) ; }) ;






  for (i = (length / sizeof (int)) - 1; i >= 0; i--)
    ((int *) t)[i] = 0;

  ((t)->common.type)  = type;
  ((t)->common.code = (int) ( code)) ;

  if (obstack == &permanent_obstack)
    ((t)->common.permanent_flag)  = 1;

  ((t)->exp.operands[ 0])  = node;
  if (node)
    {
      if (((node)->common.side_effects_flag) )
	((t)->common.side_effects_flag)  = 1;
      if (((node)->common.raises_flag) )
	((t)->common.raises_flag)  = 1;
    }

  return t;
}

 




tree
build_nt ( __builtin_va_alist )
        int __builtin_va_alist;   
{
  va_list p;
  register enum tree_code code;
  register tree t;
  register int length;
  register int i;

   p=(char *) &__builtin_va_alist ;

  code = (p = (__gnuc_va_list) ((char *) (p) + (((sizeof ( enum tree_code) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ),	*(( enum tree_code *) (void *) ((char *) (p) - (((sizeof ( enum tree_code) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ))) ;
  t = make_node (code);
  length = tree_code_length[(int) code];

  for (i = 0; i < length; i++)
    ((t)->exp.operands[ i])  = (p = (__gnuc_va_list) ((char *) (p) + (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ),	*(( tree *) (void *) ((char *) (p) - (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ))) ;

   ;
  return t;
}

 


tree
build_parse_node ( __builtin_va_alist )
        int __builtin_va_alist;   
{
  register struct obstack *ambient_obstack = expression_obstack;
  va_list p;
  register enum tree_code code;
  register tree t;
  register int length;
  register int i;

  expression_obstack = &temp_decl_obstack;

   p=(char *) &__builtin_va_alist ;

  code = (p = (__gnuc_va_list) ((char *) (p) + (((sizeof ( enum tree_code) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ),	*(( enum tree_code *) (void *) ((char *) (p) - (((sizeof ( enum tree_code) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ))) ;
  t = make_node (code);
  length = tree_code_length[(int) code];

  for (i = 0; i < length; i++)
    ((t)->exp.operands[ i])  = (p = (__gnuc_va_list) ((char *) (p) + (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ),	*(( tree *) (void *) ((char *) (p) - (((sizeof ( tree) + sizeof (int) - 1) / sizeof (int)) * sizeof (int)) ))) ;

   ;
  expression_obstack = ambient_obstack;
  return t;
}

# 2175 "/a/users/paul/gcc2/cc1/../common/tree.c"


 





tree
build_decl (code, name, type)
     enum tree_code code;
     tree name, type;
{
  register tree t;

  t = make_node (code);

 

 


  ((t)->decl.name)  = name;
  ((t)->decl.assembler_name)  = name;
  ((t)->common.type)  = type;

  if (code == VAR_DECL || code == PARM_DECL || code == RESULT_DECL)
    layout_decl (t, 0);
  else if (code == FUNCTION_DECL)
    ((t)->decl.mode)  = QImode ;

  return t;
}

 



tree
build_block (vars, tags, subblocks, supercontext, chain)
     tree vars, tags, subblocks, supercontext, chain;
{
  register tree block = make_node (BLOCK);
  ((block)->block.vars)  = vars;
  ((block)->block.type_tags)  = tags;
  ((block)->block.subblocks)  = subblocks;
  ((block)->block.supercontext)  = supercontext;
  ((block)->common.chain)   = chain;
  return block;
}

 












tree
build_type_variant (type, constp, volatilep)
     tree type;
     int constp, volatilep;
{
  register tree t, m = ((type)->type.main_variant) ;
  register struct obstack *ambient_obstack = current_obstack;

   
  constp = !!constp;
  volatilep = !!volatilep;

   








  if (!flag_gen_aux_info)
    for (t = m; t; t = ((t)->type.next_variant) )
      if (constp == ((t)->common.readonly_flag)  && volatilep == ((t)->common.volatile_flag) )
        return t;

   
  current_obstack
    = ((type)->common.permanent_flag)  ? &permanent_obstack : saveable_obstack;

  t = copy_node (type);
  ((t)->common.readonly_flag)  = constp;
  ((t)->common.volatile_flag)  = volatilep;
  ((t)->type.pointer_to)  = 0;
  ((t)->type.reference_to)  = 0;

   
  ((t)->type.next_variant)  = ((m)->type.next_variant) ;
  ((m)->type.next_variant)  = t;

  current_obstack = ambient_obstack;
  return t;
}

 



tree
change_main_variant (type, new_main)
     tree type, new_main;
{
  tree t;
  tree omain = ((type)->type.main_variant) ;

   
  if (((omain)->type.next_variant)  == type)
    ((omain)->type.next_variant)  = ((type)->type.next_variant) ;
  else
    for (t = ((omain)->type.next_variant) ; t && ((t)->type.next_variant) ;
	 t = ((t)->type.next_variant) )
      if (((t)->type.next_variant)  == type)
	{
	  ((t)->type.next_variant)  = ((type)->type.next_variant) ;
	  break;
	}

  ((type)->type.main_variant)  = new_main;
  ((type)->type.next_variant)  = ((new_main)->type.next_variant) ;
  ((new_main)->type.next_variant)  = type;
}

 


tree
build_type_copy (type)
     tree type;
{
  register tree t, m = ((type)->type.main_variant) ;
  register struct obstack *ambient_obstack = current_obstack;

  current_obstack
    = ((type)->common.permanent_flag)  ? &permanent_obstack : saveable_obstack;

  t = copy_node (type);
  ((t)->type.pointer_to)  = 0;
  ((t)->type.reference_to)  = 0;

   
  ((t)->type.next_variant)  = ((m)->type.next_variant) ;
  ((m)->type.next_variant)  = t;

  current_obstack = ambient_obstack;
  return t;
}

 


 


struct type_hash
{
  struct type_hash *next;	 
  int hashcode;			 
  tree type;			 
};

 







struct type_hash *type_hash_table[59 ];

 



 



int
type_hash_list (list)
     tree list;
{
  register int hashcode;
  register tree tail;
  for (hashcode = 0, tail = list; tail; tail = ((tail)->common.chain) )
    hashcode += ((int ) (((tail)->list.value) ) & 0777777) ;
  return hashcode;
}

 


tree
type_hash_lookup (hashcode, type)
     int hashcode;
     tree type;
{
  register struct type_hash *h;
  for (h = type_hash_table[hashcode % 59 ]; h; h = h->next)
    if (h->hashcode == hashcode
	&& ((enum tree_code) (h->type)->common.code)  == ((enum tree_code) (type)->common.code) 
	&& ((h->type)->common.type)  == ((type)->common.type) 
	&& (((h->type)->type.maxval)  == ((type)->type.maxval) 
	    || tree_int_cst_equal (((h->type)->type.maxval) ,
				   ((type)->type.maxval) ))
	&& (((h->type)->type.minval)  == ((type)->type.minval) 
	    || tree_int_cst_equal (((h->type)->type.minval) ,
				   ((type)->type.minval) ))
	&& (((h->type)->type.values)  == ((type)->type.values) 
	    || (((h->type)->type.values) 
		&& ((enum tree_code) (((h->type)->type.values) )->common.code)  == TREE_LIST
		&& ((type)->type.values) 
		&& ((enum tree_code) (((type)->type.values) )->common.code)  == TREE_LIST
		&& type_list_equal (((h->type)->type.values) , ((type)->type.values) ))))
      return h->type;
  return 0;
}

 


void
type_hash_add (hashcode, type)
     int hashcode;
     tree type;
{
  register struct type_hash *h;

  h = (struct type_hash *) oballoc (sizeof (struct type_hash));
  h->hashcode = hashcode;
  h->type = type;
  h->next = type_hash_table[hashcode % 59 ];
  type_hash_table[hashcode % 59 ] = h;
}

 










 
int debug_no_type_hash = 0;

tree
type_hash_canon (hashcode, type)
     int hashcode;
     tree type;
{
  tree t1;

  if (debug_no_type_hash)
    return type;

  t1 = type_hash_lookup (hashcode, type);
  if (t1 != 0)
    {
      struct obstack *o
	= ((type)->common.permanent_flag)  ? &permanent_obstack : saveable_obstack;
       	({ struct obstack *__o = (o);	void *__obj = ( type);	if (__obj > (void *)__o->chunk && __obj < (void *)__o->chunk_limit) __o->next_free = __o->object_base = __obj;	else (obstack_free) (__o, __obj); }) ;




      return t1;
    }

   
  if (current_obstack == &permanent_obstack)
    type_hash_add (hashcode, type);

  return type;
}

 




int
type_list_equal (l1, l2)
     tree l1, l2;
{
  register tree t1, t2;
  for (t1 = l1, t2 = l2; t1 && t2; t1 = ((t1)->common.chain) , t2 = ((t2)->common.chain) )
    {
      if (((t1)->list.value)  != ((t2)->list.value) )
	return 0;
      if (((t1)->list.purpose)  != ((t2)->list.purpose) )
	{
	  int cmp = simple_cst_equal (((t1)->list.purpose) , ((t2)->list.purpose) );
	  if (cmp < 0)
	    abort ();
	  if (cmp == 0)
	    return 0;
	}
    }

  return t1 == t2;
}

 


int
tree_int_cst_equal (t1, t2)
     tree t1, t2;
{
  if (t1 == t2)
    return 1;
  if (t1 == 0 || t2 == 0)
    return 0;
  if (((enum tree_code) (t1)->common.code)  == INTEGER_CST
      && ((enum tree_code) (t2)->common.code)  == INTEGER_CST
      && ((t1)->int_cst.int_cst_low)  == ((t2)->int_cst.int_cst_low) 
      && ((t1)->int_cst.int_cst_high)  == ((t2)->int_cst.int_cst_high) )
    return 1;
  return 0;
}

 


int
tree_int_cst_lt (t1, t2)
     tree t1, t2;
{
  if (t1 == t2)
    return 0;

  if (!((((t1)->common.type) )->common.unsigned_flag) )
    return (((t1)->int_cst.int_cst_high)  < (( t2)->int_cst.int_cst_high) 	|| (((t1)->int_cst.int_cst_high)  == (( t2)->int_cst.int_cst_high) 	&& ((unsigned int ) ((t1)->int_cst.int_cst_low) 	< (unsigned int ) (( t2)->int_cst.int_cst_low) ))) ;
  return (((unsigned int ) ((t1)->int_cst.int_cst_high) 	< (unsigned int ) (( t2)->int_cst.int_cst_high) )	|| (((unsigned int ) ((t1)->int_cst.int_cst_high) 	== (unsigned int  ) (( t2)->int_cst.int_cst_high) ) && (((unsigned int ) ((t1)->int_cst.int_cst_low) 	< (unsigned int ) (( t2)->int_cst.int_cst_low) )))) ;
}

 
int
simple_cst_list_equal (l1, l2)
     tree l1, l2;
{
  while (l1 != (tree) 0   && l2 != (tree) 0  )
    {
      int cmp = simple_cst_equal (((l1)->list.value) , ((l2)->list.value) );
      if (cmp < 0)
	abort ();
      if (cmp == 0)
	return 0;
      l1 = ((l1)->common.chain) ;
      l2 = ((l2)->common.chain) ;
    }
  return (l1 == l2);
}

 





int
simple_cst_equal (t1, t2)
     tree t1, t2;
{
  register enum tree_code code1, code2;
  int cmp;

  if (t1 == t2)
    return 1;
  if (t1 == 0 || t2 == 0)
    return 0;

  code1 = ((enum tree_code) (t1)->common.code) ;
  code2 = ((enum tree_code) (t2)->common.code) ;

  if (code1 == NOP_EXPR || code1 == CONVERT_EXPR || code1 == NON_LVALUE_EXPR)
    if (code2 == NOP_EXPR || code2 == CONVERT_EXPR || code2 == NON_LVALUE_EXPR)
      return simple_cst_equal (((t1)->exp.operands[ 0]) , ((t2)->exp.operands[ 0]) );
    else
      return simple_cst_equal (((t1)->exp.operands[ 0]) , t2);
  else if (code2 == NOP_EXPR || code2 == CONVERT_EXPR
	   || code2 == NON_LVALUE_EXPR)
    return simple_cst_equal (t1, ((t2)->exp.operands[ 0]) );

  if (code1 != code2)
    return 0;

  switch (code1)
    {
    case INTEGER_CST:
      return ((t1)->int_cst.int_cst_low)  == ((t2)->int_cst.int_cst_low) 
	&& ((t1)->int_cst.int_cst_high)  == ((t2)->int_cst.int_cst_high) ;

    case REAL_CST:
      return ((((t1)->real_cst.real_cst) ) == ( ((t2)->real_cst.real_cst) )) ;

    case STRING_CST:
      return ((t1)->string.length)  == ((t2)->string.length) 
	&& !bcmp (((t1)->string.pointer) , ((t2)->string.pointer) ,
		  ((t1)->string.length) );

    case CONSTRUCTOR:
      abort ();

    case SAVE_EXPR:
      return simple_cst_equal (((t1)->exp.operands[ 0]) , ((t2)->exp.operands[ 0]) );

    case CALL_EXPR:
      cmp = simple_cst_equal (((t1)->exp.operands[ 0]) , ((t2)->exp.operands[ 0]) );
      if (cmp <= 0)
	return cmp;
      return simple_cst_list_equal (((t1)->exp.operands[ 1]) , ((t2)->exp.operands[ 1]) );

    case TARGET_EXPR:
       



      if ((((enum tree_code) (((t1)->exp.operands[ 0]) )->common.code)  == VAR_DECL
	   && ((((t1)->exp.operands[ 0]) )->decl.name)  == (tree) 0  
	   && ((((t1)->exp.operands[ 0]) )->decl.rtl)  == 0)
	  || (((enum tree_code) (((t2)->exp.operands[ 0]) )->common.code)  == VAR_DECL
	      && ((((t2)->exp.operands[ 0]) )->decl.name)  == (tree) 0  
	      && ((((t2)->exp.operands[ 0]) )->decl.rtl)  == 0))
	cmp = 1;
      else
	cmp = simple_cst_equal (((t1)->exp.operands[ 0]) , ((t2)->exp.operands[ 0]) );
      if (cmp <= 0)
	return cmp;
      return simple_cst_equal (((t1)->exp.operands[ 1]) , ((t2)->exp.operands[ 1]) );

    case WITH_CLEANUP_EXPR:
      cmp = simple_cst_equal (((t1)->exp.operands[ 0]) , ((t2)->exp.operands[ 0]) );
      if (cmp <= 0)
	return cmp;
      return simple_cst_equal (((t1)->exp.operands[ 2]) , ((t1)->exp.operands[ 2]) );

    case COMPONENT_REF:
      if (((t1)->exp.operands[ 1])  == ((t2)->exp.operands[ 1]) )
	return simple_cst_equal (((t1)->exp.operands[ 0]) , ((t2)->exp.operands[ 0]) );
      return 0;

    case VAR_DECL:
    case PARM_DECL:
    case CONST_DECL:
    case FUNCTION_DECL:
      return 0;
    }

   


  switch ((*tree_code_type[(int) (code1)]) )
    {
      int i;
    case '1':
    case '2':
    case '<':
    case 'e':
    case 'r':
    case 's':
      cmp = 1;
      for (i=0; i<tree_code_length[(int) code1]; ++i)
	{
	  cmp = simple_cst_equal (((t1)->exp.operands[ i]) , ((t2)->exp.operands[ i]) );
	  if (cmp <= 0)
	    return cmp;
	}
      return cmp;
    }

  return -1;
}

 



 


tree
build_pointer_type (to_type)
     tree to_type;
{
  register tree t = ((to_type)->type.pointer_to) ;
  register struct obstack *ambient_obstack = current_obstack;
  register struct obstack *ambient_saveable_obstack = saveable_obstack;

   

  if (t)
    return t;

   
  if (((to_type)->common.permanent_flag) )
    {
      current_obstack = &permanent_obstack;
      saveable_obstack = &permanent_obstack;
    }

  t = make_node (POINTER_TYPE);
  ((t)->common.type)  = to_type;

   
  ((to_type)->type.pointer_to)  = t;

   


  layout_type (t);

  current_obstack = ambient_obstack;
  saveable_obstack = ambient_saveable_obstack;
  return t;
}

 



tree
build_index_type (maxval)
     tree maxval;
{
  register tree itype = make_node (INTEGER_TYPE);
  ((itype)->type.precision)  = ((sizetype)->type.precision) ;
  ((itype)->type.minval)  = build_int_2_wide ((int ) (0), (int ) ( 0)) ;
  ((((itype)->type.minval) )->common.type)  = sizetype;
  ((itype)->type.maxval)  = convert (sizetype, maxval);
  ((itype)->type.mode)  = ((sizetype)->type.mode) ;
  ((itype)->type.size)  = ((sizetype)->type.size) ;
  ((itype)->type.align)  = ((sizetype)->type.align) ;
  if (((enum tree_code) (maxval)->common.code)  == INTEGER_CST)
    {
      int maxint = (int) ((maxval)->int_cst.int_cst_low) ;
       

      if ((((maxval)->int_cst.int_cst_high)  < (( integer_zero_node)->int_cst.int_cst_high) 	|| (((maxval)->int_cst.int_cst_high)  == (( integer_zero_node)->int_cst.int_cst_high) 	&& ((unsigned int ) ((maxval)->int_cst.int_cst_low) 	< (unsigned int ) (( integer_zero_node)->int_cst.int_cst_low) ))) )
	{
	  ((itype)->type.maxval)  = build_int_2_wide ((int ) (-1), (int ) ( -1)) ;
	  ((((itype)->type.maxval) )->common.type)  = sizetype;
	}
      return type_hash_canon (maxint < 0 ? ~maxint : maxint, itype);
    }
  else
    return itype;
}

 




tree
build_range_type (type, lowval, highval)
     tree type, lowval, highval;
{
  register tree itype = make_node (INTEGER_TYPE);
  ((itype)->common.type)  = type;
  if (type == (tree) 0  )
    type = sizetype;
  ((itype)->type.precision)  = ((type)->type.precision) ;
  ((itype)->type.minval)  = convert (type, lowval);
  ((itype)->type.maxval)  = convert (type, highval);
  ((itype)->type.mode)  = ((type)->type.mode) ;
  ((itype)->type.size)  = ((type)->type.size) ;
  ((itype)->type.align)  = ((type)->type.align) ;
  if ((((enum tree_code) (lowval)->common.code)  == INTEGER_CST)
      && (((enum tree_code) (highval)->common.code)  == INTEGER_CST))
    {
      int  highint = ((highval)->int_cst.int_cst_low) ;
      int  lowint = ((lowval)->int_cst.int_cst_low) ;
      int maxint = (int) (highint - lowint);
      return type_hash_canon (maxint < 0 ? ~maxint : maxint, itype);
    }
  else
    return itype;
}

 


tree
build_index_2_type (lowval,highval)
     tree lowval, highval;
{
  return build_range_type ((tree) 0  , lowval, highval);
}

 




int
index_type_equal (itype1, itype2)
     tree itype1, itype2;
{
  if (((enum tree_code) (itype1)->common.code)  != ((enum tree_code) (itype2)->common.code) )
    return 0;
  if (((enum tree_code) (itype1)->common.code)  == INTEGER_TYPE)
    {
      if (((itype1)->type.precision)  != ((itype2)->type.precision) 
	  || ((itype1)->type.mode)  != ((itype2)->type.mode) 
	  || ! simple_cst_equal (((itype1)->type.size) , ((itype2)->type.size) )
	  || ((itype1)->type.align)  != ((itype2)->type.align) )
	return 0;
      if (simple_cst_equal (((itype1)->type.minval) , ((itype2)->type.minval) )
	  && simple_cst_equal (((itype1)->type.maxval) , ((itype2)->type.maxval) ))
	return 1;
    }
  return 0;
}

 



tree
build_array_type (elt_type, index_type)
     tree elt_type, index_type;
{
  register tree t;
  int hashcode;

  if (((enum tree_code) (elt_type)->common.code)  == FUNCTION_TYPE)
    {
      error ("arrays of functions are not meaningful");
      elt_type = integer_type_node;
    }

   
  build_pointer_type (elt_type);

   

  t = make_node (ARRAY_TYPE);
  ((t)->common.type)  = elt_type;
  ((t)->type.values)  = index_type;

  if (index_type == 0)
    {
      return t;
    }

  hashcode = ((int ) (elt_type) & 0777777)  + ((int ) (index_type) & 0777777) ;
  t = type_hash_canon (hashcode, t);










  if (((t)->type.size)  == 0)
    layout_type (t);
  return t;
}

 






tree
build_function_type (value_type, arg_types)
     tree value_type, arg_types;
{
  register tree t;
  int hashcode;

  if (((enum tree_code) (value_type)->common.code)  == FUNCTION_TYPE
      || ((enum tree_code) (value_type)->common.code)  == ARRAY_TYPE)
    {
      error ("function return type cannot be function or array");
      value_type = integer_type_node;
    }

   
  t = make_node (FUNCTION_TYPE);
  ((t)->common.type)  = value_type;
  ((t)->type.values)  = arg_types;

   
  hashcode = ((int ) (value_type) & 0777777)  + type_hash_list (arg_types);
  t = type_hash_canon (hashcode, t);

  if (((t)->type.size)  == 0)
    layout_type (t);
  return t;
}

 

tree
build_reference_type (to_type)
     tree to_type;
{
  register tree t = ((to_type)->type.reference_to) ;
  register struct obstack *ambient_obstack = current_obstack;
  register struct obstack *ambient_saveable_obstack = saveable_obstack;

   

  if (t)
    return t;

   
  if (((to_type)->common.permanent_flag) )
    {
      current_obstack = &permanent_obstack;
      saveable_obstack = &permanent_obstack;
    }

  t = make_node (REFERENCE_TYPE);
  ((t)->common.type)  = to_type;

   
  ((to_type)->type.reference_to)  = t;

  layout_type (t);

  current_obstack = ambient_obstack;
  saveable_obstack = ambient_saveable_obstack;
  return t;
}

 




tree
build_method_type (basetype, type)
     tree basetype, type;
{
  register tree t;
  int hashcode;

   
  t = make_node (METHOD_TYPE);

  if (((enum tree_code) (type)->common.code)  != FUNCTION_TYPE)
    abort ();

  ((t)->type.maxval)  = ((basetype)->type.main_variant) ;
  ((t)->common.type)  = ((type)->common.type) ;

   


  ((t)->type.values) 
    = tree_cons ((tree) 0  ,
		 build_pointer_type (basetype), ((type)->type.values) );

   
  hashcode = ((int ) (basetype) & 0777777)  + ((int ) (type) & 0777777) ;
  t = type_hash_canon (hashcode, t);

  if (((t)->type.size)  == 0)
    layout_type (t);

  return t;
}

 



tree
build_offset_type (basetype, type)
     tree basetype, type;
{
  register tree t;
  int hashcode;

   
  t = make_node (OFFSET_TYPE);

  ((t)->type.maxval)  = ((basetype)->type.main_variant) ;
  ((t)->common.type)  = type;

   
  hashcode = ((int ) (basetype) & 0777777)  + ((int ) (type) & 0777777) ;
  t = type_hash_canon (hashcode, t);

  if (((t)->type.size)  == 0)
    layout_type (t);

  return t;
}

 

tree
build_complex_type (component_type)
     tree component_type;
{
  register tree t;
  int hashcode;

   
  t = make_node (COMPLEX_TYPE);

  ((t)->common.type)  = ((component_type)->type.main_variant) ;
  ((t)->common.volatile_flag)  = ((component_type)->common.volatile_flag) ;
  ((t)->common.readonly_flag)  = ((component_type)->common.readonly_flag) ;

   
  hashcode = ((int ) (component_type) & 0777777) ;
  t = type_hash_canon (hashcode, t);

  if (((t)->type.size)  == 0)
    layout_type (t);

  return t;
}

 






















tree
get_unwidened (op, for_type)
     register tree op;
     tree for_type;
{
   
   

  register tree type = ((op)->common.type) ;
  register unsigned final_prec
    = ((for_type != 0 ? for_type : type)->type.precision) ;
  register int uns
    = (for_type != 0 && for_type != type
       && final_prec > ((type)->type.precision) 
       && ((type)->common.unsigned_flag) );
  register tree win = op;

  while (((enum tree_code) (op)->common.code)  == NOP_EXPR)
    {
      register int bitschange
	= ((((op)->common.type) )->type.precision) 
	  - ((((((op)->exp.operands[ 0]) )->common.type) )->type.precision) ;

       

      if (bitschange < 0
	  && final_prec > ((((op)->common.type) )->type.precision) )
	break;

       

      op = ((op)->exp.operands[ 0]) ;

       






      if (bitschange > 0)
	{
	  if (! uns || final_prec <= ((((op)->common.type) )->type.precision) )
	    win = op;
	   


	  if ((uns || ((enum tree_code) (op)->common.code)  == NOP_EXPR)
	      && ((((op)->common.type) )->common.unsigned_flag) )
	    {
	      uns = 1;
	      win = op;
	    }
	}
    }

  if (((enum tree_code) (op)->common.code)  == COMPONENT_REF
       
      && ((enum tree_code) (type)->common.code)  != REAL_TYPE)
    {
      unsigned innerprec = ((((((op)->exp.operands[ 1]) )->decl.size) )->int_cst.int_cst_low) ;
      type = type_for_size (innerprec, ((((op)->exp.operands[ 1]) )->common.unsigned_flag) );

       





      if (innerprec < ((((op)->common.type) )->type.precision) 
	  && (for_type || ! ((((op)->exp.operands[ 1]) )->decl.bit_field_flag) )
	  && (! uns || final_prec <= innerprec
	      || ((((op)->exp.operands[ 1]) )->common.unsigned_flag) )
	  && type != 0)
	{
	  win = build (COMPONENT_REF, type, ((op)->exp.operands[ 0]) ,
		       ((op)->exp.operands[ 1]) );
	  ((win)->common.side_effects_flag)  = ((op)->common.side_effects_flag) ;
	  ((win)->common.volatile_flag)  = ((op)->common.volatile_flag) ;
	  ((win)->common.raises_flag)  = ((op)->common.raises_flag) ;
	}
    }
  return win;
}

 




tree
get_narrower (op, unsignedp_ptr)
     register tree op;
     int *unsignedp_ptr;
{
  register int uns = 0;
  int first = 1;
  register tree win = op;

  while (((enum tree_code) (op)->common.code)  == NOP_EXPR)
    {
      register int bitschange
	= ((((op)->common.type) )->type.precision) 
	  - ((((((op)->exp.operands[ 0]) )->common.type) )->type.precision) ;

       
      if (bitschange < 0)
	break;

       

      op = ((op)->exp.operands[ 0]) ;

      if (bitschange > 0)
	{
	   

	  if (first)
	    uns = ((((op)->common.type) )->common.unsigned_flag) ;
	   


	  else if (uns != ((((op)->common.type) )->common.unsigned_flag) )
	    break;
	  first = 0;
	}
       

      win = op;
    }

  if (((enum tree_code) (op)->common.code)  == COMPONENT_REF
       
      && ((enum tree_code) (((op)->common.type) )->common.code)  != REAL_TYPE)
    {
      unsigned innerprec = ((((((op)->exp.operands[ 1]) )->decl.size) )->int_cst.int_cst_low) ;
      tree type = type_for_size (innerprec, ((op)->common.unsigned_flag) );

       







      if (innerprec < ((((op)->common.type) )->type.precision) 
	  && ! ((((op)->exp.operands[ 1]) )->decl.bit_field_flag) 
	  && (first || uns == ((((op)->exp.operands[ 1]) )->common.unsigned_flag) )
	  && type != 0)
	{
	  if (first)
	    uns = ((((op)->exp.operands[ 1]) )->common.unsigned_flag) ;
	  win = build (COMPONENT_REF, type, ((op)->exp.operands[ 0]) ,
		       ((op)->exp.operands[ 1]) );
	  ((win)->common.side_effects_flag)  = ((op)->common.side_effects_flag) ;
	  ((win)->common.volatile_flag)  = ((op)->common.volatile_flag) ;
	  ((win)->common.raises_flag)  = ((op)->common.raises_flag) ;
	}
    }
  *unsignedp_ptr = uns;
  return win;
}

 




int
type_precision (type)
     register tree type;
{
  return ((((enum tree_code) (type)->common.code)  == INTEGER_TYPE
	   || ((enum tree_code) (type)->common.code)  == ENUMERAL_TYPE
	   || ((enum tree_code) (type)->common.code)  == REAL_TYPE)
	  ? ((type)->type.precision)  : 32 );
}

 


int
int_fits_type_p (c, type)
     tree c, type;
{
  if (((type)->common.unsigned_flag) )
    return (!(((unsigned int ) ((((type)->type.maxval) )->int_cst.int_cst_high) 	< (unsigned int ) (( c)->int_cst.int_cst_high) )	|| (((unsigned int ) ((((type)->type.maxval) )->int_cst.int_cst_high) 	== (unsigned int  ) (( c)->int_cst.int_cst_high) ) && (((unsigned int ) ((((type)->type.maxval) )->int_cst.int_cst_low) 	< (unsigned int ) (( c)->int_cst.int_cst_low) )))) 
	    && !(((unsigned int ) ((c)->int_cst.int_cst_high) 	< (unsigned int ) (( ((type)->type.minval) )->int_cst.int_cst_high) )	|| (((unsigned int ) ((c)->int_cst.int_cst_high) 	== (unsigned int  ) (( ((type)->type.minval) )->int_cst.int_cst_high) ) && (((unsigned int ) ((c)->int_cst.int_cst_low) 	< (unsigned int ) (( ((type)->type.minval) )->int_cst.int_cst_low) )))) 
	    && (((c)->int_cst.int_cst_high)  >= 0 || ((((c)->common.type) )->common.unsigned_flag) ));
  else
    return (!(((((type)->type.maxval) )->int_cst.int_cst_high)  < (( c)->int_cst.int_cst_high) 	|| (((((type)->type.maxval) )->int_cst.int_cst_high)  == (( c)->int_cst.int_cst_high) 	&& ((unsigned int ) ((((type)->type.maxval) )->int_cst.int_cst_low) 	< (unsigned int ) (( c)->int_cst.int_cst_low) ))) 
	    && !(((c)->int_cst.int_cst_high)  < (( ((type)->type.minval) )->int_cst.int_cst_high) 	|| (((c)->int_cst.int_cst_high)  == (( ((type)->type.minval) )->int_cst.int_cst_high) 	&& ((unsigned int ) ((c)->int_cst.int_cst_low) 	< (unsigned int ) (( ((type)->type.minval) )->int_cst.int_cst_low) ))) 
	    && (((c)->int_cst.int_cst_high)  >= 0 || !((((c)->common.type) )->common.unsigned_flag) ));
}

 


tree
decl_function_context (decl)
     tree decl;
{
  tree context;

  if (((enum tree_code) (decl)->common.code)  == ERROR_MARK)
    return 0;

  if (((enum tree_code) (decl)->common.code)  == SAVE_EXPR)
    context = ((decl)->exp.operands[ 1])  ;
  else
    context = ((decl)->decl.context) ;

  while (context && ((enum tree_code) (context)->common.code)  != FUNCTION_DECL)
    {
      if (((enum tree_code) (context)->common.code)  == RECORD_TYPE
	  || ((enum tree_code) (context)->common.code)  == UNION_TYPE)
	context = ((context)->type.context) ;
      else if (((enum tree_code) (context)->common.code)  == TYPE_DECL)
	context = ((context)->decl.context) ;
      else if (((enum tree_code) (context)->common.code)  == BLOCK)
	context = ((context)->block.supercontext) ;
      else
	 
	abort ();
    }

  return context;
}

 



tree
decl_type_context (decl)
     tree decl;
{
  tree context = ((decl)->decl.context) ;

  while (context)
    {
      if (((enum tree_code) (context)->common.code)  == RECORD_TYPE
	  || ((enum tree_code) (context)->common.code)  == UNION_TYPE)
	return context;
      if (((enum tree_code) (context)->common.code)  == TYPE_DECL
	  || ((enum tree_code) (context)->common.code)  == FUNCTION_DECL)
	context = ((context)->decl.context) ;
      else if (((enum tree_code) (context)->common.code)  == BLOCK)
	context = ((context)->block.supercontext) ;
      else
	 
	abort ();
    }
  return (tree) 0  ;
}

void
print_obstack_statistics (str, o)
     char *str;
     struct obstack *o;
{
  struct _obstack_chunk *chunk = o->chunk;
  int n_chunks = 0;
  int n_alloc = 0;

  while (chunk)
    {
      n_chunks += 1;
      n_alloc += chunk->limit - &chunk->contents[0];
      chunk = chunk->prev;
    }
  fprintf ((&__sF[2]) , "obstack %s: %d bytes, %d chunks\n",
	   str, n_alloc, n_chunks);
}
void
dump_tree_statistics ()
{
  int i;
  int total_nodes, total_bytes;

  fprintf ((&__sF[2]) , "\n??? tree nodes created\n\n");
# 3337 "/a/users/paul/gcc2/cc1/../common/tree.c"

  fprintf ((&__sF[2]) , "(No per-node statistics)\n");

  print_lang_statistics ();
}
