define(`confMIME_FORMAT_ERRORS', False)dnl

FEATURE(redirect)dnl

# All mail for netbsd.org machines is sent to mail.netbsd.org for processing.
# Outgoing mail is from netbsd.org.

MASQUERADE_AS(`netbsd.org')dnl
FEATURE(allmasquerade)dnl
EXPOSED_USER(root)dnl

define(`MAIL_HUB', `mail.netbsd.org')dnl
define(`LOCAL_RELAY', `mail.netbsd.org')dnl
define(`SMART_HOST', `mail.netbsd.org')dnl
