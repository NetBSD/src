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
	case R_TYPE(PC32):
		def = _rtld_find_symdef(rela->r_info, obj, &defobj, false);
		if (def == NULL)
			return -1;

		tmp = (Elf_Addr)(defobj->relocbase + def->st_value +
		    rela->r_addend) - (Elf_Addr)where;
		if (*where != tmp)
			*where = tmp;
		rdbg(dodebug, ("PC32 %s in %s --> %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, defobj->path));
		break;

	case R_TYPE(GOT32):
#endif
	case R_TYPE(32):
	case R_TYPE(GLOB_DAT):
		def = _rtld_find_symdef(rela->r_info, obj, &defobj, false);
		if (def == NULL)
			return -1;

		tmp = (Elf_Addr)(defobj->relocbase + def->st_value +
		    rela->r_addend);
		if (*where != tmp)
			*where = tmp;
		rdbg(dodebug, ("32/GLOB_DAT %s in %s --> %p in %s",
		    defobj->strtab + def->st_name, obj->path,
		    (void *)*where, defobj->path));
		break;

	case R_TYPE(RELATIVE):
		*where += (Elf_Addr)obj->relocbase;
		rdbg(dodebug, ("RELATIVE in %s --> %p", obj->path,
		    (void *)*where));
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
