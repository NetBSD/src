/*	$NetBSD: LDAPModDNRequest.h,v 1.2 2020/08/11 13:15:34 christos Exp $	*/

// $OpenLDAP$
/*
 * Copyright 2000-2020 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_MOD_DN_REQUEST_H
#define LDAP_MOD_DN_REQUEST_H

#include <LDAPRequest.h>

class LDAPModDNRequest : LDAPRequest {

    public:
        LDAPModDNRequest(const LDAPModDNRequest& req); 
        LDAPModDNRequest(const std::string& dn, const std::string& newRDN,
                bool deleteOld, const std::string& newParentDN,
                LDAPAsynConnection *connect, const LDAPConstraints *cons,
                bool isReferral=false, const LDAPRequest* parent=0); 
        virtual ~LDAPModDNRequest(); 
        
        virtual LDAPMessageQueue* sendRequest(); 
        virtual LDAPRequest* followReferral(LDAPMsg*  urls);
    
    private:
        std::string m_dn;
        std::string m_newRDN;
        std::string m_newParentDN;
        bool m_deleteOld;
};    

#endif // LDAP_MOD_DN_REQUEST_H

