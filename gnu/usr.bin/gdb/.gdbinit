# $NetBSD: .gdbinit,v 1.4 2000/11/28 22:33:49 jmc Exp $

# .gdbinit for debugging gdb with itself

echo Setting up the environment for debugging gdb.\n

set complaints 1

b fatal

b info_command
commands
	silent
	return
end

dir ../../dist/gdb
dir ../../dist/readline

dir ../../lib/libbfd

dir ../../dist/bfd
dir ../../dist/opcodes
dir ../../dist/libiberty
dir ../../dist/include

set prompt (top-gdb) 
