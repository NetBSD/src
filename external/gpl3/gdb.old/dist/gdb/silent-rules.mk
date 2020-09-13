# If V is undefined or V=0 is specified, use the silent/verbose/compact mode.
V ?= 0
ifeq ($(V),0)
ECHO_CXX =    @echo "  CXX    $@";
ECHO_CXXLD =  @echo "  CXXLD  $@";
ECHO_REGDAT = @echo "  REGDAT $@";
ECHO_GEN =    @echo "  GEN    $@";
ECHO_GEN_XML_BUILTIN = \
              @echo "  GEN    xml-builtin.c";
ECHO_GEN_XML_BUILTIN_GENERATED = \
              @echo "  GEN    xml-builtin-generated.c";
ECHO_INIT_C =  echo "  GEN    init.c" ||
ECHO_SIGN =   @echo "  SIGN   gdb";
ECHO_YACC =   @echo "  YACC   $@";
ECHO_LEX  =   @echo "  LEX    $@";
SILENCE = @
endif
