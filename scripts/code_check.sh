#!/bin/bash

excludes=""
while read -r submodule
do
    if [[ $submodule == path* ]]; then
        path=$(echo $submodule | cut -d' ' -f3)
        excludes="$excludes ! -path './$path/*'"
    fi
done < .gitmodules

files=$(eval "find . -name '*.cc' -o -name '*.h' $excludes")
exit_status=0

for file in $files; do
    diff_output=$(diff -u --color=always <(cat $file) <(clang-format $file))
    if [ $? -ne 0 ]; then
        echo "$file requires clang-format:"
        echo "$diff_output"
        exit_status=1
    fi
done

exit $exit_status
