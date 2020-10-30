#!/bin/bash
# Run clang-tidy on a diff between the given branch and the HEAD

THIS_DIR=$(dirname $(realpath "${BASH_SOURCE[0]}"))

# The branch to diff against (default: master)
BASE_BRANCH=${1:-master}

# Directories that support clang-tidy
DIRS=userspace
# Use a minimal set of build flags
BUILD_FLAGS=-std=gnu99
git diff origin/${BASE_BRANCH} --diff-filter=ACMRTUXB ${DIRS} | clang-tidy-diff -p1 -- ${BUILD_FLAGS}
exit $?
