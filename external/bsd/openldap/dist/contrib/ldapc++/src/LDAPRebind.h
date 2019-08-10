/*	$NetBSD: LDAPRebind.h,v 1.1.1.5.6.1 2019/08/10 06:17:07 martin Exp $	*/

// $OpenLDAP$
/*
 * Copyright 2000-2019 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_REBIND_H
#define LDAP_REBIND_H

#include <string>
#include <LDAPRebindAuth.h>

/**
 * Just an abstract class to provide a mechnism for rebind to another
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

