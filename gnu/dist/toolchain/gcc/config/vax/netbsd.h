#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-D__vax__ -D__NetBSD__ -Asystem(unix) -Asystem(NetBSD) -Acpu(vax) -Amachine(vax)"

#undef CC1_SPEC
#define CC1_SPEC "\
	%{!fno-pic: \
		%{!mno-pic: %{!fpic: %{!fPIC:-fPIC}}} \
		%{!mindirect: %{!no-mindirect: -mno-indirect}}} \
	%{mno-pic: -fno-pic -mindirect} \
	%{fno-pic: \
		%{!mindirect: %{!no-mindirect: -mindirect}}}"

#define	CC1PLUS_SPEC CC1_SPEC

/* Make gcc agree with <machine/ansi.h> */

#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

#undef WCHAR_TYPE
#define WCHAR_TYPE "int"

#undef WCHAR_UNSIGNED
#define WCHAR_UNSIGNED 0

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 32

/* Until they use ELF or something that handles dwarf2 unwinds
   and initialization stuff better.  */
#undef DWARF2_UNWIND_INFO

#undef TARGET_DEFAULT
#define TARGET_DEFAULT 0

/* Function CSE screws up PLT .vs. GOT usage.
 */
#define NO_FUNCTION_CSE

/* This makes use of a hook in varasm.c to mark all external functions
   for us.  We use this to make sure that external functions are correctly
   referenced from the PLT.  */

#define	NO_EXTERNAL_INDIRECT_ADDRESS

/* Define this macro if references to a symbol must be treated
   differently depending on something about the variable or
   function named by the symbol (such as what section it is in).

   On the VAX, if using PIC, mark a SYMBOL_REF for a non-global
   symbol so that we may use indirect accesses with it.  */

#define ENCODE_SECTION_INFO(DECL)				\
do								\
  {								\
    if (flag_pic)						\
      {								\
	rtx rtl = (TREE_CODE_CLASS (TREE_CODE (DECL)) != 'd'	\
		   ? TREE_CST_RTL (DECL) : DECL_RTL (DECL));	\
								\
	if (GET_CODE (rtl) == MEM)				\
	  {							\
	    SYMBOL_REF_FLAG (XEXP (rtl, 0))			\
	      = (TREE_CODE_CLASS (TREE_CODE (DECL)) != 'd'	\
		 || ! TREE_PUBLIC (DECL));			\
	  }							\
      }								\
  }								\
while (0)

/* Put relocations in the constant pool in the writable data section.  */
#undef  SELECT_RTX_SECTION
#define SELECT_RTX_SECTION(MODE,RTX)			\
{							\
  if (flag_pic && vax_symbolic_operand ((RTX), (MODE)))	\
    data_section ();					\
  else							\
    readonly_data_section ();				\
}

/* Use sjlj exceptions. */
