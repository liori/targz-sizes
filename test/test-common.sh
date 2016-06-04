#!/bin/sh
# vim: ts=4 sw=4 ts

test_id=$(echo $0 | sed -e 's/^[^a-z]*//' | sed -e 's/.sh$//')
test_dir="${test_id}-testdir"
targz_sizes=${top_srcdir}/src/targz_sizes

prepTestDir () {

    rm -rf ${test_dir}
    mkdir ${test_dir}

}

runTest () {

    # run the test
    echo "runTest(), run command: ${LOG_COMPILER} ${LOG_FLAGS} ${TEST_COMMAND} < ${test_dir}/input.tgz > ${test_dir}/rawout 2> ${test_dir}/rawerr"
    ${LOG_COMPILER} ${LOG_FLAGS} ${TEST_COMMAND} < ${test_dir}/input.tgz 1> ${test_dir}/rawout 2> ${test_dir}/rawerr 
    command_result=$?
    cat ${test_dir}/rawerr ${test_dir}/rawout
    echo "runTest(), command_result=$command_result"
    if [ "$command_result" != "0" ] ; then
        echo "runTest(), exiting"
        exit $command_result
    fi
    cp ${test_dir}/rawout ${test_dir}/output
}

checkFilenames () {

    echo "checkFilenames(), entered, test_dir=$test_dir"

    # create expected result
    tar tzf ${test_dir}/input.tgz | cut -c 1-1023 > ${test_dir}/expout

    echo "checkFilenames(), run diff: diff ${test_dir}/output ${test_dir}/expout"
    diff ${test_dir}/output ${test_dir}/expout
    diff_result=$?
    echo "checkFilenames(), diff_result=$diff_result"

    if [ "$diff_result" != "0" ] ; then
        echo "checkFilenames(), exiting"
        exit $diff_result
    fi
}


checkInErrOut () {

    echo "checkInErrOut(), entered, test_id=$test_dir"

    echo "checkInErrOut(), run egrep: egrep \"$1\" ${test_dir}/rawerr"
    egrep "$1" ${test_dir}/rawerr
    egrep_result=$?

    echo "checkInErrOut(), egrep_result=$egrep_result"

    if [ "$egrep_result" != "0" ] ; then
        echo "checkInErrOut(), exiting"
        exit $egrep_result
    fi
}

makeName () {

    count=$1

    for i in `seq 1 ${count}`; do

        if [ $i = 1 ] ; then
            echo -n '['
        elif [ $i = ${count} ] ; then
            echo -n ']'
        elif [ $((i%100)) = 0 ] ; then
            echo -n '/'
        elif [ $((i%10)) = 0 ] ; then
            echo -n $((i/10%10))
        elif [ $((i%5)) = 0 ] ; then
            echo -n ','
        else
            echo -n '.'
        fi

    done
}
