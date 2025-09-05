/*	$NetBSD: LDAPRebind.h,v 1.4 2025/09/05 21:16:14 christos Exp $	*/

// $OpenLDAP$
/*
 * Copyright 2000-2024 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_REBIND_H
#define LDAP_REBIND_H

#include <string>
#include <LDAPRebindAuth.h>

/**
 * Just an abstract class to provide a mechanism for rebind to another
 * server when chasing referrals. Clients have to implement a class
 * derived from this. To use authentication other than anonymous for
 * referral chasing
 */

class LDAPRebind{
    public:
        virtual ~LDAPRebind() {}
        virtual LDAPRebindAuth* getRebindAuth(const std::string& hostname, 
                int port) const = 0;
};
#endif //LDAP_REBIND_H

