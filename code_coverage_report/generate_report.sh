#!/bin/bash
# check if we can find ../code_coverage_report
base_path=`pwd`
SOURCE_DIR=${base_path}/../
BUILD_DIR=${base_path}/build
#INFO_OUTPUT_DIR=${base_path}/info_output
OUTPUT_DIR=${base_path}/output
tmp=../
tmp2=../../C

if [ ! -d "$SOURCE_DIR"]; then
    exit 1
else
    echo "find source dir"
fi

echo "SOURCE_DIR: $SOURCE_DIR"
echo "BUILD_DIR: $BUILD_DIR"
echo "INFO_OUTPUT_DIR: $INFO_OUTPUT_DIR"
echo "OUTPUT_DIR: $OUTPUT_DIR"
echo "tmp: $tmp"
echo "tmp2: $tmp2"

#exit 0
#read -p "Press any key to continue." var

mkdir $BUILD_DIR
#mkdir $INFO_OUTPUT_DIR
mkdir $OUTPUT_DIR
cd $BUILD_DIR
# config options
cmake "$SOURCE_DIR" -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_C_OUTPUT_EXTENSION_REPLACE=1 -DCMAKE_CXX_OUTPUT_EXTENSION_REPLACE=1 -DCMAKE_BUILD_TYPE=Debug -DGENERATE_CODE_COVERAGE=1
# buil
make -j8
# Initial
lcov --capture --initial --base-directory $SOURCE_DIR  --directory . --output-file svt_av1_base.info

# run the unit tests
cp "$SOURCE_DIR"Bin/Debug/* .
./SvtAv1UnitTests
./SvtAv1ApiTests

# capture
lcov --capture --base-directory $SOURCE_DIR --directory . --output-file svt_av1_test.info

# merge the status
lcov --add-tracefile svt_av1_base.info --add-tracefile svt_av1_test.info --output-file svt_av1_total.info

# remove unwanted
lcov -r svt_av1_total.info "*third_party*" "*test*" "*/usr/*" -o svt_av1_final.info

genhtml svt_av1_final.info --output-directory $OUTPUT_DIR
