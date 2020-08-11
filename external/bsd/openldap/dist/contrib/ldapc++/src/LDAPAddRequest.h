/*	$NetBSD: LDAPAddRequest.h,v 1.1.1.7 2020/08/11 13:12:03 christos Exp $	*/

// $OpenLDAP$
/*
 * Copyright 2000-2020 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_ADD_REQUEST_H
#define  LDAP_ADD_REQUEST_H

#include <LDAPRequest.h>
#include <LDAPEntry.h>

class LDAPMessageQueue;

class LDAPAddRequest : LDAPRequest {
    public:
        LDAPAddRequest(const LDAPAddRequest& req);
        LDAPAddRequest(const LDAPEntry* entry, 
                LDAPAsynConnection *connect,
                const LDAPConstraints *cons, bool isReferral=false, 
                const LDAPRequest* parent=0);
        virtual ~LDAPAddRequest();
        virtual LDAPMessageQueue* sendRequest();
        virtual LDAPRequest* followReferral(LDAPMsg* refs);
    private:
        LDAPEntry* m_entry;

};
#endif // LDAP_ADD_REQUEST_H

