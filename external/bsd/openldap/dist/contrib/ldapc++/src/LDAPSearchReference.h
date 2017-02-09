/*	$NetBSD: LDAPSearchReference.h,v 1.1.1.4 2017/02/09 01:46:43 christos Exp $	*/

// $OpenLDAP$
/*
 * Copyright 2000-2016 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_SEARCH_REFERENCE_H
#define LDAP_SEARCH_REFERENCE_H 

#include <LDAPMessage.h>
#include <LDAPUrlList.h>

class LDAPRequest;
class LDAPUrl;

/**
 * This class is used to represent Continuation References that were
 * returned during a SEARCH-Operation.
 */
class LDAPSearchReference : public LDAPMsg{

    public :
        /**
         * Constructor that create an object from the C-API structures
         */
        LDAPSearchReference(const LDAPRequest* req, LDAPMessage* msg);

        /**
         * The Destructor
         */
        ~LDAPSearchReference();

        /**
         * @returns The destination URLs that were send with this message
         */
        const LDAPUrlList& getUrls() const;

    private :
        LDAPUrlList m_urlList;
        LDAPSearchReference();
};



#endif //LDAP_SEARCH_REFERENCE_H
