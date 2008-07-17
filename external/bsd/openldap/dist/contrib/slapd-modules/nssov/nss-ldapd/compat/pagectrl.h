/*
   pagectrl.h - provide a replacement ldap_create_page_control() function.
   This file was part of the nss_ldap library which has been
   forked into the nss-ldapd library.

   Copyright (C) 1997-2005 Luke Howard

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

#ifndef _LDAP_NSS_LDAP_PAGECTRL_H
#define _LDAP_NSS_LDAP_PAGECTRL_H

#ifndef HAVE_LDAP_CREATE_PAGE_CONTROL
int
ldap_create_page_control( LDAP *ld,
    unsigned long pagesize,
    struct berval *cookiep,
    int iscritical,
    LDAPControl **ctrlp );
#endif /* not HAVE_LDAP_CREATE_PAGE_CONTROL */

#ifndef HAVE_LDAP_PARSE_PAGE_CONTROL
int
ldap_parse_page_control(
    LDAP           *ld,
    LDAPControl    **ctrls,
    unsigned long  *list_countp,
    struct berval  **cookiep );
#endif /* not HAVE_LDAP_PARSE_PAGE_CONTROL */

#endif /* _LDAP_NSS_LDAP_UTIL_H */
