// $OpenLDAP: pkg/ldap/contrib/ldapc++/src/LDAPControl.h,v 1.5.10.1 2008/04/14 23:09:26 quanah Exp $
/*
 * Copyright 2000, OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */


#ifndef LDAP_CONTROL_H
#define LDAP_CONTROL_H
#include <string>
#include <ldap.h>

/**
 * This class is used to store Controls. Controls are a mechanism to extend
 * and modify LDAP-Operations.
 */
class LDAPCtrl{
    public :
        /**
         * Copy-constructor
         */
        LDAPCtrl(const LDAPCtrl& c);

        /**
         * Constructor.
         * @param oid:  The Object Identifier of the Control
         * @param critical: "true" if the Control should be handled
         *                  critical by the server.
         * @param data: If there is data for the control, put it here.
         * @param length: The length of the data field
         */
        LDAPCtrl(const char *oid, bool critical, const char *data=0, 
                int length=0);

        /**
         * Constructor.
         * @param oid:  The Object Identifier of the Control
         * @param critical: "true" if the Control should be handled
         *                  critical by the server.
         * @param data: If there is data for the control, put it here.
         */
        LDAPCtrl(const std::string& oid, bool critical=false,
                const std::string& data=std::string());

        /**
         * Creates a copy of the Control that "ctrl is pointing to
         */
        LDAPCtrl(const LDAPControl* ctrl);

        /**
         * Destructor
         */
        ~LDAPCtrl();
       
        /**
         * @return The OID of the control
         */
        std::string getOID() const;

        /**
         * @return The Data of the control as a std::string-Objekt
         */
        std::string getData() const;

        /**
         * @return "true" if the control is critical
         */
        bool isCritical() const;

        /**
         * For internal use only.
         *
         * Translates the control to a LDAPControl-structure as needed by
         * the C-API
         */
        LDAPControl* getControlStruct() const;
	static void freeLDAPControlStruct(LDAPControl *ctrl);

    private :
        std::string m_oid;
        std::string m_data;
        bool m_isCritical;
};

#endif //LDAP_CONTROL_H
