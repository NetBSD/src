#include "bfd.h"
#include "sysdep.h"
#include "libaout.h"

/* Return the list of objects needed by BFD.  */

/*ARGSUSED*/
struct bfd_link_needed_list *
bfd_netbsd_get_needed_list (abfd, info)
     bfd *abfd;
     struct bfd_link_info *info;
{
  if (aout_backend_info (abfd)->get_needed_list != NULL)
    {
      return (*aout_backend_info (abfd)->get_needed_list)
             (abfd, info);
    }

  return NULL;
}

/* Record an assignment made to a symbol by a linker script.  We need
   this in case some dynamic object refers to this symbol.  */

boolean
bfd_netbsd_record_link_assignment (output_bfd, info, name)
     bfd *output_bfd;
     struct bfd_link_info *info;
     const char *name;
{
  if (aout_backend_info (output_bfd)->record_link_assignment != NULL)
    {
      return (*aout_backend_info (output_bfd)->record_link_assignment)
             (output_bfd, info, name);
    }

  return true;
}

/* Set up the sizes and contents of the dynamic sections. This is called
   by the NetBSD linker emulation before_allocation routine.  */

boolean
bfd_netbsd_size_dynamic_sections (output_bfd, info, sdynptr, sneedptr,
				 srulesptr)
     bfd *output_bfd;
     struct bfd_link_info *info;
     asection **sdynptr;
     asection **sneedptr;
     asection **srulesptr;
{
  if (aout_backend_info (output_bfd)->size_dynamic_sections != NULL)
    {
      return (*aout_backend_info (output_bfd)->size_dynamic_sections)
             (output_bfd, info, sdynptr, sneedptr, srulesptr);
    }

  return true;
}
