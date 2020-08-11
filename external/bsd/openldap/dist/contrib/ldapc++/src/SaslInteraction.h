/*	$NetBSD: SaslInteraction.h,v 1.2 2020/08/11 13:15:34 christos Exp $	*/

// $OpenLDAP$
/*
 * Copyright 2007-2020 The OpenLDAP Foundation, All Rights Reserved.
 * COPYING RESTRICTIONS APPLY, see COPYRIGHT file
 */

#ifndef SASL_INTERACTION_H
#define SASL_INTERACTION_H

#include <string>
#include <sasl/sasl.h>

class SaslInteraction {
    public:
        SaslInteraction( sasl_interact_t *interact );
        ~SaslInteraction();
        unsigned long getId() const;
        const std::string getPrompt() const;
        const std::string getChallenge() const;
        const std::string getDefaultResult() const;

        void setResult(const std::string &res);

    private:
        sasl_interact_t *m_interact;
        std::string m_result;

};
#endif /* SASL_INTERACTION_H */
