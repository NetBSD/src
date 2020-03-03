/*
 *  Copyright (C) 2014-2018 Yubico AB - See COPYING
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* These #defines must be present according to PAM documentation. */
#define PAM_SM_AUTH

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
#ifdef HAVE_SECURITY_PAM_MODULES_H
#include <security/pam_modules.h>
#endif

int
main (int argc, const char **argv)
{
  pam_handle_t *pamh = NULL;
  int rc;

  rc = pam_sm_authenticate (pamh, 0, 1, argv);

  printf ("rc %d\n", rc);

  return 0;
}
