#include <sys/types.h>
#include <sys/stat.h>

#include "debug.h"
#include "rtld.h"

void
_rtld_setup_pltgot(const Obj_Entry *obj)
{
	obj->pltgot[1] = (Elf_Addr) obj;
	obj->pltgot[2] = (Elf_Addr) &_rtld_bind_start;
}

int
_rtld_relocate_nonplt_object(obj, rela, dodebug)
	Obj_Entry *obj;
	const Elf_Rela *rela;
	bool dodebug;
{
	Elf_Addr        *where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
	const Elf_Sym   *def;
	const Obj_Entry *defobj;
	Elf_Addr         tmp;

	switch (ELF_R_TYPE(rela->r_info)) {

	case R_TYPE(NONE):
		break;

#if 1 /* XXX should not occur */
	case R_TYPE(PC24): {	/* word32 S - P + A */
		Elf32_Sword addend;

		/*
		 * Extract addend and sign-extend if needed.
		 */
		addend = *where;
		if (addend & 0x00800000)
			addend |= 0xff000000;

		def = _rtld_find_symdef(rela->r_info, obj, &defobj, false);
		if (def == NULL)
			return -1;
		tmp = (Elf_Addr)obj->relocbase + def->st_value
		    - (Elf_Addr)where + (addend << 2);
		if ((tmp & 0xfe000000) != 0xfe000000 &&
		    (tmp & 0xfe000000) != 0) {
			_rtld_error(
			"%s: R_ARM_PC24 relocation @ %p to %s failed "
			"(displacement %ld (%#lx) out of range)",
			    obj->path, where, defobj->strtab + def->st_name,
			    (long) tmp, (long) tmp);
			return -1;
		}
		tmp >>= 2;
		*where = (*where & 0xff000000) | (tmp & 0x00ffffff);
		rdbg(dodebug, ("PC24 %s in %s --> %p @ %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, where, defobj->path));
		break;
	}
#endif

	case R_TYPE(ABS32):	/* word32 B + S + A */
		def = _rtld_find_symdef(rela->r_info, obj, &defobj, false);
		if (def == NULL)
			return -1;
		*where += (Elf_Addr)defobj->relocbase + def->st_value;
		rdbg(dodebug, ("ABS32 %s in %s --> %p @ %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, where, defobj->path));
		break;

	case R_TYPE(GLOB_DAT):	/* word32 B + S */
		def = _rtld_find_symdef(rela->r_info, obj, &defobj, false);
		if (def == NULL)
			return -1;
		*where = (Elf_Addr)(defobj->relocbase + def->st_value);
		rdbg(dodebug, ("GLOB_DAT %s in %s --> %p @ %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, where, defobj->path));
		break;

	case R_TYPE(RELATIVE):	/* word32 B + A */
		*where += (Elf_Addr)obj->relocbase;
		rdbg(dodebug, ("RELATIVE in %s --> %p @ %p", obj->path,
		    (void *)*where, where));
		break;

	case R_TYPE(COPY):
		/*
		 * These are deferred until all other relocations have
		 * been done.  All we do here is make sure that the COPY
		 * relocation is not in a shared library.  They are allowed
		 * only in executable files.
		 */
		if (!obj->mainprog) {
			_rtld_error(
			"%s: Unexpected R_COPY relocation in shared library",
			    obj->path);
			return -1;
		}
		rdbg(dodebug, ("COPY (avoid in main)"));
		break;

	default:
		def = _rtld_find_symdef(rela->r_info, obj, &defobj, true);
		rdbg(dodebug, ("sym = %lu, type = %lu, offset = %p, "
		    "addend = %p, contents = %p, symbol = %s",
		    (u_long)ELF_R_SYM(rela->r_info),
		    (u_long)ELF_R_TYPE(rela->r_info),
		    (void *)rela->r_offset, (void *)rela->r_addend,
		    (void *)*where,
		    def ? defobj->strtab + def->st_name : "??"));
		_rtld_error("%s: Unsupported relocation type %ld "
		    "in non-PLT relocations\n",
		    obj->path, (u_long) ELF_R_TYPE(rela->r_info));
		return -1;
	}
	return 0;
}
