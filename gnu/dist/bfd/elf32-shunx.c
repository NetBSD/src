#define TARGET_BIG_SYM		bfd_elf32_shunx_vec
#define TARGET_BIG_NAME		"elf32-sh-unx"
#define TARGET_LITTLE_SYM	bfd_elf32_shlunx_vec
#define TARGET_LITTLE_NAME	"elf32-shl-unx"
#define ELF_MAXPAGESIZE		0x1000

#include "elf32-sh.c"
