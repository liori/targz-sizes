#!/bin/sh
# vim: ts=4 sw=4 ts

. ${top_srcdir}/test/test-common.sh

# setup
prepTestDir
longdir=`makeName 100`
mkdir -p ${test_dir}/input/${longdir}
tar czf ${test_dir}/input.tgz -C ${test_dir} input

# run test
TEST_COMMAND="${targz_sizes} --log-level 5"
runTest

# clean output
/bin/echo -E "$0, clean output: cut -d' ' -f2 < ${test_dir}/rawout > ${test_dir}/output"
cut -d' ' -f2 < ${test_dir}/rawout > ${test_dir}/output

# check output
checkFilenames

