// $OpenLDAP: pkg/ldap/contrib/ldapc++/src/LDAPSearchRequest.h,v 1.4.10.1 2008/04/14 23:09:26 quanah Exp $
/*
 * Copyright 2000, OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef LDAP_SEARCH_REQUEST_H
#define LDAP_SEARCH_REQUEST_H

#include <queue>
#include <LDAPRequest.h>

class LDAPSearchReference;
class LDAPReferral;
class LDAPUrl;

class LDAPSearchRequest : public LDAPRequest{ 

    public :
        LDAPSearchRequest(const LDAPSearchRequest& req);

        LDAPSearchRequest(const std::string& base, int scope, const std::string& filter,
                          const StringList& attrs, bool attrsOnly, 
                          LDAPAsynConnection *connect,
                          const LDAPConstraints* cons, bool isReferral=false,
                          const LDAPRequest* parent=0);
        virtual ~LDAPSearchRequest();        
        virtual LDAPMessageQueue* sendRequest();
        virtual LDAPRequest* followReferral(LDAPMsg* ref);
        virtual bool equals(const LDAPRequest* req) const;
    
    private :
        std::string m_base;
        int m_scope;
        std::string m_filter;
        StringList m_attrs;
        bool m_attrsOnly;

        //no default constructor
        LDAPSearchRequest(){};
};

#endif //LDAP_SEARCH_REQUEST_H
