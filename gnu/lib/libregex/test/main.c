/* Main routine for running various tests.  Meant only to be linked with
   all the auxiliary test source files, with `test' undefined.  */

#ifndef lint
static char rcsid[] = "$Id: main.c,v 1.2 1993/08/02 17:24:20 mycroft Exp $";
#endif /* not lint */

#include "test.h"

test_type t = all_test;


/* Use this to run the tests we've thought of.  */

int
main ()
{
  switch (t)
  {
  case all_test:
    test_regress ();
    test_others ();
    test_posix_basic ();
    test_posix_extended ();
    test_posix_interface ();
    break;
    
  case other_test:
    test_others ();
    break;
  
  case posix_basic_test:
    test_posix_basic ();
    break;
  
  case posix_extended_test:
    test_posix_extended ();
    break;
  
  case posix_interface_test:
    test_posix_interface ();
    break;
  
  case regress_test:
    test_regress ();
    break;
    
  default:
    fprintf (stderr, "Unknown test %d.\n", t);
  }

  return 0;
}
