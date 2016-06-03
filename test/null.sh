#!/bin/sh

. ${top_srcdir}/test/test-common.sh

# setup
prepTestDir
mkdir -p ${test_dir}/input/one/two/three
tar czf ${test_dir}/input.tgz -C ${test_dir} input

# run test
TEST_COMMAND="${targz_sizes} --log-level 5 --null"
runTest

# clean output
/bin/echo -E "$0: clean output: tr '\0' '\n' < ${test_dir}/rawout | cut -d' ' -f2 > ${test_dir}/output"
tr '\0' '\n' < ${test_dir}/rawout | cut -d' ' -f2 > ${test_dir}/output

# check output
checkFilenames

checkInErrOut "null separator enabled"
