// $OpenLDAP: pkg/ldap/contrib/ldapc++/src/LDAPAddRequest.h,v 1.3.10.1 2008/04/14 23:09:26 quanah Exp $
/*
 * Copyright 2000, OpenLDAP Foundation, All Rights Reserved.
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

