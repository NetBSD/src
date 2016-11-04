extern void abort (void);

/* Since GCC 5 folds symbol address comparison, assuming each symbol has
   different address,  &foo == &bar is always false for GCC 5.  Use
   check_ptr_eq to check if 2 addresses are the same.  */

void 
check_ptr_eq (void *p1, void *p2)
{
  if (p1 != p2)
    abort ();
}
