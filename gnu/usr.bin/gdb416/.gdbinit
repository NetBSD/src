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

dir ../../lib/bfd

dir ../../dist/bfd
dir ../../dist/opcodes
dir ../../dist/libiberty
dir ../../dist/include

set prompt (top-gdb) 
