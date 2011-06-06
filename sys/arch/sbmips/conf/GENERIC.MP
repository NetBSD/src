
include "arch/sbmips/conf/GENERIC"

options 	MULTIPROCESSOR
options 	LOCKDEBUG

pseudo-device	lockstat

no options	SYMTAB_SPACE
options		SYMTAB_SPACE=640000

