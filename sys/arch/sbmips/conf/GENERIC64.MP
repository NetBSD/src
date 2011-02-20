
include "arch/sbmips/conf/GENERIC64"

options 	MULTIPROCESSOR
options 	LOCKDEBUG

pseudo-device	lockstat

no options	SYMTAB_SPACE
options		SYMTAB_SPACE=640000

