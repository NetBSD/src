/*	$NetBSD: LDAPModifyRequest.h,v 1.1.1.3.10.1 2017/04/21 16:52:22 bouyer Exp $	*/

// $OpenLDAP$
/*
 * Copyright 2000-2016 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_MODIFY_REQUEST_H
#define LDAP_MODIFY_REQUEST_H

#include <LDAPRequest.h>

class LDAPMessageQueue;

class LDAPModifyRequest : LDAPRequest {
    private :
        std::string m_dn;
        LDAPModList *m_modList;

    public:
        LDAPModifyRequest(const LDAPModifyRequest& mod);
        LDAPModifyRequest(const std::string& dn, const LDAPModList *modList,
                LDAPAsynConnection *connect, const LDAPConstraints *cons,
                bool isReferral=false, const LDAPRequest* req=0);
        virtual ~LDAPModifyRequest();
        virtual LDAPMessageQueue* sendRequest();
        virtual LDAPRequest* followReferral(LDAPMsg* refs);
};

#endif // LDAP_MODIFY_REQUEST_H

