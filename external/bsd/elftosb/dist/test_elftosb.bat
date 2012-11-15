
mkdir test_output

.\elftosb2\Debug\elftosb -Vdz -p bdfiles -p test_files -fmx28 -c simple.e -o test_output\output1.sb plugin_hello redboot_gcc.srec hostlink
.\sbtool\Debug\sbtool -Vdz test_output\output1.sb > test_output\output1.txt

.\elftosb2\Debug\elftosb -Vdz -p bdfiles -p test_files -fmx28 -c habtest.bd -o test_output\output2.sb plugin_hello redboot_gcc.srec hostlink
.\sbtool\Debug\sbtool -Vdz test_output\output2.sb > test_output\output2.txt

.\elftosb2\Debug\elftosb -Vdz -p bdfiles -p test_files -fmx28 -c basic_test_cmd.e -o test_output\output3.sb plugin_hello redboot_gcc.srec hostlink sd_player_gcc
.\sbtool\Debug\sbtool -Vdz test_output\output2.sb > test_output\output3.txt

