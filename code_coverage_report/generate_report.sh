#!/bin/bash
# check if we can find ../Cidana-SVT-AV1
SOURCE_DIR = ../Cidana-SVT-AV1

if [ ! -d "$SOURCE_DIR"]; then
    exit 1
else
    echo "find source dir"
fi

# config options
cmake ../Cidana-SVT-AV1/ -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_C_OUTPUT_EXTENSION_REPLACE=1 -DCMAKE_CXX_OUTPUT_EXTENSION_REPLACE=1 -DCMAKE_BUILD_TYPE=Debug -DGENERATE_CODE_COVERAGE=1
# build
make -j8
# Initial
lcov --no-external --capture --initial --base-directory ../Cidana-SVT-AV1/ --directory . --output-file svt_av1_base.info

# run the unit tests
cp ../Cidana-SVT-AV1/Bin/Debug/* .
./SvtAv1UnitTests
./SvtAv1ApiTests

# capture
lcov --no-external --capture --base-directory ../Cidana-SVT-AV1/ --directory . --output-file svt_av1_test.info

# merge the status
lcov --add-tracefile ./svt_av1_base.info --add-tracefile ./svt_av1_test.info --output-file ./svt_av1_total.info

# remove unwanted
lcov -r svt_av1_total.info "*third_party*" "test" -o svt_av1_final.info

genhtml svt_av1_final.info --output-directory ../Cidana-SVT-AV1/code_coverage_report
