/*	$NetBSD: LDAPDeleteRequest.h,v 1.1.1.4 2017/02/09 01:46:43 christos Exp $	*/

// $OpenLDAP$
/*
 * Copyright 2000-2016 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_DELETE_REQUEST_H
#define LDAP_DELETE_REQUEST_H

#include <LDAPRequest.h>
class LDAPMessageQueue;

class LDAPDeleteRequest : public LDAPRequest{
    public :
        LDAPDeleteRequest(const LDAPDeleteRequest& req);
        LDAPDeleteRequest(const std::string& dn, LDAPAsynConnection *connect,
                const LDAPConstraints *cons, bool isReferral=false, 
                const LDAPRequest* parent=0);
        virtual ~LDAPDeleteRequest();
        virtual LDAPMessageQueue* sendRequest();
        virtual LDAPRequest* followReferral(LDAPMsg* refs); 
	
    private :
		std::string m_dn;
};
#endif //LDAP_DELETE_REQUEST_H
