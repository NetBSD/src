#! /bin/sh
# Provision of this shell script should not be taken to imply that use of
# GNU eqn with groff -Tascii|-Tlatin1|-Tutf8|-Tcp1047 is supported.

@GROFF_BIN_PATH_SETUP@
PATH="$GROFF_RUNTIME$PATH"
export PATH
exec @g@eqn -Tascii ${1+"$@"}

# eof
