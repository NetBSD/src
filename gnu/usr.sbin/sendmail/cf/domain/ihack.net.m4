define(`confAUTO_REBUILD', True)dnl
define(`confMIME_FORMAT_ERRORS', False)dnl
define(`confTRUSTED_USERS', mycroft)dnl

FEATURE(redirect)dnl

define(`MAIL_HUB', `mail.ihack.net.')dnl
define(`UUCP_RELAY', `life.ai.mit.edu.')dnl
define(`BITNET_RELAY', `mitvma.mit.edu.')dnl

# All mail for ihack.net machines is sent to mail.ihack.net for processing.
# Outgoing mail is from ihack.net.

MASQUERADE_AS(`ihack.net')dnl
MASQUERADE_DOMAIN(`ihack.net')dnl
MASQUERADE_DOMAIN(`ihack.org')dnl
MASQUERADE_DOMAIN(`ihack.com')dnl
FEATURE(allmasquerade)dnl
FEATURE(always_add_domain)dnl
FEATURE(masquerade_envelope)dnl
FEATURE(masquerade_entire_domain)dnl
