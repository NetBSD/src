/* Make sure adding a non-root-visible type after adding a root-visible forward
   adds a new type rather than promoting and returning the existing one.  */

#include <ctf-api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main (int argc, char *argv[])
{
  ctf_dict_t *fp;
  ctf_id_t root, nonroot;
  int err;

  if ((fp = ctf_create (&err)) == NULL)
    {
      fprintf (stderr, "Cannot create: %s\n", ctf_errmsg (err));
      return 1;
    }

  if ((root = ctf_add_forward (fp, CTF_ADD_ROOT, "foo", CTF_K_ENUM)) == CTF_ERR)
    goto add_err;

  if ((nonroot = ctf_add_enum (fp, CTF_ADD_NONROOT, "foo")) == CTF_ERR)
    goto add_err;

  if (nonroot == root)
    fprintf (stderr, "Non-root addition should not promote root-visible forwards\n");
  else
    printf ("All done.\n");

  ctf_dict_close (fp);
  return 0;

 add_err:
  fprintf (stderr, "Cannot add: %s\n", ctf_errmsg (ctf_errno (fp)));
}
