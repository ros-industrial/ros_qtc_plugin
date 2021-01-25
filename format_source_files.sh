#!/bin/sh
find . -regex './src/.*\(cpp\|h\)' -exec clang-format -style=file -i {} \;

