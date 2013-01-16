#! /bin/bash

# Tool paths
if [ -d /System/Library/CoreServices ]; then
    Elftosb=./build/Debug/elftosb
    Sbtool=./build/Debug/sbtool
else
    Elftosb=./bld/linux/elftosb
    Sbtool=./bld/linux/sbtool
fi

# Create test output file directory
mkdir -p test_output

# simple.e
$Elftosb -Vdz -p bdfiles -p test_files -fmx28 -c simple.e -o test_output/output1.sb plugin_hello redboot_gcc.srec hostlink
$Sbtool -Vdz test_output/output1.sb > test_output/output1.txt

# habtest.bd
$Elftosb -Vdz -p bdfiles -p test_files -fmx28 -c habtest.bd -o test_output/output2.sb plugin_hello redboot_gcc.srec hostlink
$Sbtool -Vdz test_output/output2.sb > test_output/output2.txt

# basic_test_cmd.e
$Elftosb -Vdz -p bdfiles -p test_files -fmx28 -c basic_test_cmd.e -o test_output/output3.sb plugin_hello redboot_gcc.srec hostlink sd_player_gcc
$Sbtool -Vdz test_output/output3.sb > test_output/output3.txt

