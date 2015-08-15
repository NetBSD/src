#!/bin/sh -u

# Register protocol definitions for GDB, the GNU debugger.
# Copyright (C) 2001-2014 Free Software Foundation, Inc.
#
# This file is part of GDB.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

move_if_change ()
{
    file=$1
    if test -r ${file} && cmp -s "${file}" new-"${file}"
    then
	echo "${file} unchanged." 1>&2
    else
	mv new-"${file}" "${file}"
	echo "${file} updated." 1>&2
    fi
}

# Format of the input files
read="type entry"

do_read ()
{
    type=""
    entry=""
    while read line
    do
	if test "${line}" = ""
	then
	    continue
	elif test "${line}" = "#" -a "${comment}" = ""
	then
	    continue
	elif expr "${line}" : "#" > /dev/null
	then
	    comment="${comment}
${line}"
	else

	    # The semantics of IFS varies between different SH's.  Some
	    # treat ``::' as three fields while some treat it as just too.
	    # Work around this by eliminating ``::'' ....
	    line="`echo "${line}" | sed -e 's/::/: :/g' -e 's/::/: :/g'`"

	    OFS="${IFS}" ; IFS="[:]"
	    eval read ${read} <<EOF
${line}
EOF
	    IFS="${OFS}"

	    # .... and then going back through each field and strip out those
	    # that ended up with just that space character.
	    for r in ${read}
	    do
		if eval test \"\${${r}}\" = \"\ \"
		then
		    eval ${r}=""
		fi
	    done

	    break
	fi
    done
    if [ -n "${type}" ]
    then
	true
    else
	false
    fi
}

if test ! -r $1; then
  echo "$0: Could not open $1." 1>&2
  exit 1
fi

copyright ()
{
cat <<EOF
/* *INDENT-OFF* */ /* THIS FILE IS GENERATED */

/* A register protocol for GDB, the GNU debugger.
   Copyright (C) 2001-2013 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This file was created with the aid of \`\`regdat.sh'' and \`\`$1''.  */

EOF
}


exec > new-$2
copyright $1
echo '#include "server.h"'
echo '#include "regdef.h"'
echo '#include "tdesc.h"'
echo
offset=0
i=0
name=x
xmltarget=x
xmlarch=x
xmlosabi=x
expedite=x
exec < $1
while do_read
do
  if test "${type}" = "name"; then
    name="${entry}"
    echo "static struct reg regs_${name}[] = {"
    continue
  elif test "${type}" = "xmltarget"; then
    xmltarget="${entry}"
    continue
  elif test "${type}" = "xmlarch"; then
    xmlarch="${entry}"
    continue
  elif test "${type}" = "osabi"; then
    xmlosabi="${entry}"
    continue
  elif test "${type}" = "expedite"; then
    expedite="${entry}"
    continue
  elif test "${name}" = x; then
    echo "$0: $1 does not specify \`\`name''." 1>&2
    exit 1
  else
    echo "  { \"${entry}\", ${offset}, ${type} },"
    offset=`expr ${offset} + ${type}`
    i=`expr $i + 1`
  fi
done

echo "};"
echo
echo "static const char *expedite_regs_${name}[] = { \"`echo ${expedite} | sed 's/,/", "/g'`\", 0 };"
if test "${xmltarget}" = x; then
  if test "${xmlarch}" = x && test "${xmlosabi}" = x; then
    echo "static const char *xmltarget_${name} = 0;"
  else
    echo "static const char *xmltarget_${name} = \"@<target>\\"
    if test "${xmlarch}" != x; then
      echo "<architecture>${xmlarch}</architecture>\\"
    fi
    if test "${xmlosabi}" != x; then
      echo "<osabi>${xmlosabi}</osabi>\\"
    fi
    echo "</target>\";"
  fi
else
  echo "static const char *xmltarget_${name} = \"${xmltarget}\";"
fi
echo

cat <<EOF
const struct target_desc *tdesc_${name};

void
init_registers_${name} (void)
{
  static struct target_desc tdesc_${name}_s;
  struct target_desc *result = &tdesc_${name}_s;

  result->reg_defs = regs_${name};
  result->num_registers = sizeof (regs_${name}) / sizeof (regs_${name}[0]);
  result->expedite_regs = expedite_regs_${name};
  result->xmltarget = xmltarget_${name};

  init_target_desc (result);

  tdesc_${name} = result;
}
EOF

# close things off
exec 1>&2
move_if_change $2
