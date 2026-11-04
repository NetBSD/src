/* Make sure that slices of slices are properly resolved.  If they're not, both
   population and lookup will fail.  */

#include <ctf-api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (void)
{
  ctf_dict_t *fp;
  ctf_id_t base;
  ctf_id_t slice;
  ctf_id_t slice2;
  ctf_encoding_t long_encoding = { CTF_INT_SIGNED, 0, sizeof (long) };
  ctf_encoding_t foo;
  int val;
  int err;

  if ((fp = ctf_create (&err)) == NULL)
    {
      fprintf (stderr, "Cannot create: %s\n", ctf_errmsg (err));
      return 1;
    }

  if ((base = ctf_add_enum_encoded (fp, CTF_ADD_ROOT, "enom", &long_encoding))
      == CTF_ERR)
    goto err;

  if (ctf_add_enumerator (fp, base, "a", 1) < 0 ||
      ctf_add_enumerator (fp, base, "b", 0) < 0)
    goto err;

  foo.cte_format = 0;
  foo.cte_bits = 4;
  foo.cte_offset = 4;
  if ((slice = ctf_add_slice (fp, CTF_ADD_ROOT, base, &foo)) == CTF_ERR)
    goto err;

  foo.cte_bits = 6;
  foo.cte_offset = 2;
  if ((slice2 = ctf_add_slice (fp, CTF_ADD_ROOT, slice, &foo)) == CTF_ERR)
    goto err;

  if (ctf_add_variable (fp, "foo", slice) < 0)
    goto err;

  if (ctf_enum_value (fp, slice, "a", &val) < 0)
    {
      fprintf (stderr, "Cannot look up value of sliced enum: %s\n", ctf_errmsg (ctf_errno (fp)));
      return 1;
    }
  if (val != 1)
    {
      fprintf (stderr, "sliced enum value is wrong\n");
      return 1;
    }

  if (ctf_enum_value (fp, slice2, "b", &val) < 0)
    {
      fprintf (stderr, "Cannot look up value of sliced sliced enum: %s\n", ctf_errmsg (ctf_errno (fp)));
      return 1;
    }
  if (val != 0)
    {
      fprintf (stderr, "sliced sliced enum value is wrong\n");
      return 1;
    }

  ctf_dict_close (fp);
  fprintf (stderr, "All done.\n");
  return 0;

 err:
  fprintf (stderr, "cannot populate: %s\n", ctf_errmsg (ctf_errno (fp)));
  return 1;
}
