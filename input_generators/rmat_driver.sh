#!/usr/bin/env bash

# Usage: bin n m

set -x

if hash gsed 2> /dev/null; then
	sed=gsed
else
	sed=sed
fi

echo "# RMAT driver ${1} ${2} ${3} $(date)"
echo $2 $3
$1 -nVertices $2 -nEdges $3 -memUsage 0.9 -noEdgeToSelf -noDuplicateEdges 1>&2
${sed} -r 's/^([0-9]+)[ \t]*([0-9]+)/\1 \2 1/' out.txt
