#!/bin/sh
# vim: ts=4 sw=4 ts

. "${top_srcdir}/test/test-common.sh"

# setup
prepTestDir
# 1016 plus seven (7) characters 'input//',
# plus one (1) null terminator character, makes 1024.
longdir=`makeName 1016`
mkdir -p "${test_dir}/input/${longdir}"
tar czf "${test_dir}/input.tgz" -C "${test_dir}" input

# run test
TEST_COMMAND="${targz_sizes} --log-level 5"
runTest

# clean output
/bin/echo -E "$0, clean output: cut -d' ' -f2 < ${test_dir}/rawout > ${test_dir}/output"
cut -d' ' -f2 < "${test_dir}/rawout" > "${test_dir}/output"

# check output
checkFilenames

