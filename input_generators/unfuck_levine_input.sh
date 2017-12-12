#!/usr/bin/env bash
sed -e '1d' | gsed -r 's/a (.*) (.*) (.*)/echo "$((\1 - 1)) $((\2 - 1)) \3"/ge'
