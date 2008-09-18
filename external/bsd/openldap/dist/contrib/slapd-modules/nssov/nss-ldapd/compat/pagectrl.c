/*
   pagectrl.c - provide a replacement ldap_create_page_control() function.
   This file was part of the nss_ldap library which has been
   forked into the nss-ldapd library.

   Copyright (C) 2002 Max Caines
   This software is not subject to any license of the University
   of Wolverhampton.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <lber.h>
#include <ldap.h>

#include "pagectrl.h"

#ifndef LDAP_CONTROL_PAGE_OID
#define LDAP_CONTROL_PAGE_OID           "1.2.840.113556.1.4.319"
#endif

#ifndef HAVE_LDAP_CREATE_PAGE_CONTROL
/*---
   ldap_create_page_control

   Create and encode the Paged Results control.

   ld        (IN)  An LDAP session handle, as obtained from a call to
                                   ldap_init().

   pagesize  (IN)  The number of entries to return in each page

   cookiep   (IN)  Pointer to a berVal structure that the server uses to
                                   determine the current location in the
                                   result set (opaque). Set to NULL the
                                   first time.

   iscritical (IN) Is this control critical to the search?

   ctrlp     (OUT) A result parameter that will be assigned the address
                                   of an LDAPControl structure that contains the
                                   PagedResult control created by this function.
                                   The memory occupied by the LDAPControl structure
                                   SHOULD be freed when it is no longer in use by
                                   calling ldap_control_free().


   Ber encoding

   PageResult ::= SEQUENCE {
                pageSize     INTEGER
                cookie       OCTET STRING }


   Note:  The first time the Page control is created, the cookie
                  should be set to a zero-length string. The cookie obtained
                  from calling ldap_parse_page_control() should be used as
                  the cookie in the next ldap_create_page_control call.

 ---*/

int
ldap_create_page_control (LDAP * ld,
                          unsigned long pagesize,
                          struct berval *cookiep,
                          int iscritical, LDAPControl ** ctrlp)
{
  ber_tag_t tag;
  BerElement *ber;
  BerElement *ldap_alloc_ber_with_options (LDAP * ld);
  int rc;

  if ((ld == NULL) || (ctrlp == NULL))
    {
      return (LDAP_PARAM_ERROR);
    }

  if ((ber = ldap_alloc_ber_with_options (ld)) == NULL)
    {
      return (LDAP_NO_MEMORY);
    }

  tag = ber_printf (ber, "{i", pagesize);
  if (tag == LBER_ERROR)
    goto exit;

  if (cookiep == NULL)
    tag = ber_printf (ber, "o", "", 0);
  else
    tag = ber_printf (ber, "O", cookiep);
  if (tag == LBER_ERROR)
    goto exit;

  tag = ber_printf (ber, /*{ */ "N}");
  if (tag == LBER_ERROR)
    goto exit;

  rc = ldap_create_control (LDAP_CONTROL_PAGE_OID, ber, iscritical, ctrlp);

  ber_free (ber, 1);
  return (rc);

exit:
  ber_free (ber, 1);
  return (LDAP_ENCODING_ERROR);
}
#endif /* not HAVE_LDAP_CREATE_PAGE_CONTROL */

#ifndef HAVE_LDAP_PARSE_PAGE_CONTROL
/*---
   ldap_parse_page_control

   Decode the Virtual List View control return information.

   ld           (IN)   An LDAP session handle.

   ctrls        (IN)   The address of a NULL-terminated array of
                                           LDAPControl structures, typically obtained
                                           by a call to ldap_parse_result().

   list_countp  (OUT)  This result parameter is filled in with the number
                                           of entries returned in this page

   cookiep      (OUT)  This result parameter is filled in with the address
                                           of a struct berval that contains the server-
                                           generated cookie.
                                           The returned cookie SHOULD be used in the next call
                                           to create a Page sort control.  The struct berval
                                           returned SHOULD be disposed of by calling ber_bvfree()
                                           when it is no longer needed.

---*/
int
ldap_parse_page_control (LDAP * ld,
                         LDAPControl ** ctrls,
                         unsigned long *list_countp, struct berval **cookiep)
{
  BerElement *ber;
  LDAPControl *pControl;
  int i;
  unsigned long count;
  ber_tag_t tag;

  if (cookiep)
    {
      *cookiep = NULL;          /* Make sure we return a NULL if error occurs. */
    }

  if (ld == NULL)
    {
      return (LDAP_PARAM_ERROR);
    }

  if (ctrls == NULL)
    {
      return (LDAP_CONTROL_NOT_FOUND);
    }

  /* Search the list of control responses for a Page control. */
  for (i = 0; ctrls[i]; i++)
    {
      pControl = ctrls[i];
      if (!strcmp (LDAP_CONTROL_PAGE_OID, pControl->ldctl_oid))
        goto foundPageControl;
    }

  /* No page control was found. */
  return (LDAP_CONTROL_NOT_FOUND);

foundPageControl:
  /* Create a BerElement from the berval returned in the control. */
  ber = ber_init (&pControl->ldctl_value);

  if (ber == NULL)
    {
      return (LDAP_NO_MEMORY);
    }

  /* Extract the data returned in the control. */
  tag = ber_scanf (ber, "{iO" /*} */ , &count, cookiep);

  if (tag == LBER_ERROR)
    {
      ber_free (ber, 1);
      return (LDAP_DECODING_ERROR);
    }

  ber_free (ber, 1);

  /* Return data to the caller for items that were requested. */
  if (list_countp)
    {
      *list_countp = count;
    }

  return (LDAP_SUCCESS);
}
#endif /* not HAVE_LDAP_PARSE_PAGE_CONTROL */
