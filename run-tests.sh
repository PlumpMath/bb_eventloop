#!/bin/sh

TESTRUNNER="$1"
if test -z "$TESTRUNNER" || ! test -x "$TESTRUNNER" ; then
    echo "Use \"make test\" to run unit tests" 1>&2
    exit 1
fi
if test -z "$USE_XML_OUTPUT" ; then
    # Jenkins defines WORKSPACE environment variable
    if test -n "$WORKSPACE" ; then
        export XML_OUTPUT="true"
    else
        unset XML_OUTPUT
    fi
else
    export XML_OUTPUT="true"
fi
if test "$USE_VALGRIND" != "false" ; then
    VALGRIND="$(which valgrind 2> /dev/null)"
    if test $? != 0 ; then
        unset VALGRIND
        echo "valgrind not found, valgrind disabled"
    else
        export VALGRIND
        echo "valgrind enabled"
        if test -z "$VALGRIND_SUPPRESSIONS" ; then
            export VALGRIND_SUPPRESSIONS="$(dirname "$TESTRUNNER")/tst/valgrind-suppressions.conf"
        fi
    fi
else
    unset VALGRIND
    echo "valgrind disabled"
fi
if test -z "$TEST" ; then
    if test "$XML_OUTPUT" = "true" ; then
        exec "$TESTRUNNER" --gtest_output=xml:testreport.xml
    else
        exec "$TESTRUNNER"
    fi
else
    if test "$XML_OUTPUT" = "true" ; then
        exec "$TESTRUNNER" --gtest_output=xml:testreport.xml --gtest_filter="$TEST"
    else
        exec "$TESTRUNNER" --gtest_filter="$TEST"
    fi
fi
