#! /bin/bash

RISCV_OPC_FILE=$1
RISCV_FEATURE_DIR=$2

function gen_csr_xml ()
{
    bitsize=$1

    cat <<EOF
<?xml version="1.0"?>
<!-- Copyright (C) 2018-2019 Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  -->

<!DOCTYPE feature SYSTEM "gdb-target.dtd">
<feature name="org.gnu.gdb.riscv.csr">
EOF

    grep "^DECLARE_CSR(" ${RISCV_OPC_FILE} \
        | sed -e "s!DECLARE_CSR(\(.*\), .*!  <reg name=\"\1\" bitsize=\"$bitsize\"/>!"

    echo "</feature>"
}

gen_csr_xml 32 > ${RISCV_FEATURE_DIR}/32bit-csr.xml
gen_csr_xml 64 > ${RISCV_FEATURE_DIR}/64bit-csr.xml
