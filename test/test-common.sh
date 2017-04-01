#!/bin/sh
# vim: ts=4 sw=4 ts

cmd_no_sh_suffix="${0%.sh}"
test_id="${cmd_no_sh_suffix#\"${cmd_no_sh_suffix%%[a-z]*}\"}"
test_dir="${test_id}-testdir"
targz_sizes="${top_srcdir}/src/targz_sizes"

prepTestDir () {

    rm -rf "${test_dir}"
    mkdir "${test_dir}"

}


checkCommand () {
    caller_name="$1"
    command_line="$2"
    stdout_file="$3"
    stderr_file="$4"
    stdout_redir=
    stderr_redir=

    if [ "$stdout_file" ] ; then
        stdout_redir="1> $stdout_file"
        if [ "$stderr_file" ] ; then
            stderr_redir="2> $stderr_file"
        fi
    fi

    echo "${caller_name}, run command: "${command_line}
    eval "${command_line}" "${stdout_redir}" "${stderr_redir}"
    command_result="$?"
    if [ "${stdout_file}" ] ; then
        cat "${stdout_file}"
    fi
    if [ "${stderr_file}" ] ; then
        cat "${stderr_file}"
    fi
    echo "${caller_name}, command_result=${command_result}"
    if [ "$command_result" != "0" ] ; then
        echo "${caller_name}, exiting"
        exit "$command_result"
    fi
}


runTest () {

    echo "runTest(), entered, test_dir=$test_dir"

    checkCommand "runTest()" "${LOG_COMPILER} ${LOG_FLAGS} ${TEST_COMMAND} < ${test_dir}/input.tgz" "${test_dir}/rawout" "${test_dir}/rawerr"

    cp "${test_dir}/rawout" "${test_dir}/output"

}

checkFilenames () {

    echo "checkFilenames(), entered, test_dir=$test_dir"

    # create expected result
    tar tzf ${test_dir}/input.tgz | cut -c 1-1023 > ${test_dir}/expout

    checkCommand "checkFilenames()" "diff ${test_dir}/output ${test_dir}/expout"
}


checkInErrOut () {

    echo "checkInErrOut(), entered, test_id=$test_dir"

    checkCommand "checkInErrOut()" "egrep \"$1\" ${test_dir}/rawerr"

}

makeName () {

    count="$1"

    for i in `seq 1 "${count}"`; do

        if [ "$i" = 1 ] ; then
            echo -n '['
        elif [ $i = "${count}" ] ; then
            echo -n ']'
        elif [ "$((i%100))" = 0 ] ; then
            echo -n '/'
        elif [ "$((i%10))" = 0 ] ; then
            echo -n "$((i/10%10))"
        elif [ "$((i%5))" = 0 ] ; then
            echo -n ','
        else
            echo -n '.'
        fi

    done
}
