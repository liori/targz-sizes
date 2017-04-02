#!/bin/sh

. "${top_srcdir}/test/test-common.sh"

# setup
prepTestDir
mkdir -p "${test_dir}/input/one/two/three"
tar czf "${test_dir}/input.tgz" -C "${test_dir}" input

# run test
TEST_COMMAND="${targz_sizes}  --log-level 5"
runTest

# clean output
echo "$0: clean output: cut -d' ' -f2 ${test_dir}/rawout > ${test_dir}/output"
cut -d' ' -f2 "${test_dir}/rawout" > "${test_dir}/output"

# check output
checkFilenames

checkInErrOut "logging level 5 enabled"
