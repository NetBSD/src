#!/bin/sh

# Run a program with the new shared libraries instead of the installed ones.

LD_LIBRARY_PATH=`pwd`/lib exec "$@"
