#!/bin/bash
#exit 1
# git diff --name-only <commit compare1> <compare2>
# 

# cpp lint check
##git checkout -f ${sha1};
commit_msg=$(git log --oneline -1);
# commit_msg="5cd6d9f Merge c91688f9c04c2ada26f31eebf71fd812f2634beb into 43aedf77ce81a0121f767e02107f4b13779a9dfe";
# commit_msg="08e0f34 Merge 0c3381161d17f98a5aa5f0eb2242d3c3fdbd8c45 into d0cd02167802bc39ec5efd470a9b672a032ead4d";

tmp=${commit_msg#*Merge};
commit_src=${tmp%into*};
commit_target=${commit_msg##*into};

echo "commit_msg: $commit_msg";
echo "commit_src: $commit_src";
git log $commit_src -1;
echo "commit_target: $commit_target";
git log $commit_target -1;

#checkFiles=$(git diff --name-only $commit_src $commit_target | grep -E 'test/(.*)(\.c|\.h|\.cpp|\.cc)')
checkFiles=$(git diff --name-only $commit_src $commit_target test/*.h test/*.cc test/*.c test/*.cpp);
#checkFiles=$(git diff --name-only HEAD test/*.h test/*.cc test/*.c test/*.cpp)
echo "checkFiles: $checkFiles";
if [ ! -n "$checkFiles" ]; then
	echo "No files need to check."
	exit 0
fi

echo "=========== cpplint check start ==========="
# # cpplint check file type
CPPLINT_EXTENS=cc,cpp,h,c

# # cpplint filter;  -xxx remove, +xxx add
CPPLINT_FILTER=-whitespace/indent,-whitespace/line_length,-build/include_what_you_use,-readability/todo,-build/include,-build/header_guard
# cpplint --extensions=$CPPLINT_EXTENS --filter=$CPPLINT_FILTER $checkFiles 2>&1 | tee cpplint-result.xml
cpplint --extensions=$CPPLINT_EXTENS --filter=$CPPLINT_FILTER $checkFiles 2>&1
if [ $? -ne 0 ]; then 
    echo "cpplint check found error"
#    exit 1
fi

echo "=========== clang-format check start ==========="
# clang-format check
# clang-format -i $checkFiles

# loop file
errorFileCount=0
for checkFile in $checkFiles
do
    echo "===> clang-format check file $checkFile"
    clang-format -style=file -output-replacements-xml $checkFile | grep "<replacement "
    if [ $? -ne 1 ]; then 
        echo "$checkFile not match clang-format"
        let errorFileCount++
##        exit 1
    fi
done

if [ $errorFileCount -ne 0 ]; then
    echo "$errorFileCount files did not match clang-format"
    exit 1
fi

echo "All files match clang-format"

