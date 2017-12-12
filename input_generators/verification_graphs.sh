#!/usr/bin/env bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/.."

vertices=8000

${DIR}/utils/generate.py 'barbell_graph(N // 50, N // 50)' ${vertices} > $1/barbell.in
${DIR}/utils/generate.py 'grid_2d_graph(N // 50, N // 50)' ${vertices} > $1/grid.in
${DIR}/utils/generate.py 'lollipop_graph(N // 20, N // 10)' ${vertices} > $1/lolli.in
${DIR}/utils/generate.py 'star_graph(N)' ${vertices} > $1/star.in
${DIR}/utils/generate.py 'wheel_graph(N)' ${vertices} > $1/wheel.in
${DIR}/utils/generate.py 'connected_watts_strogatz_graph(N, K, 0.1)' ${vertices} -k 8 > $1/ws.in
${DIR}/utils/generate.py 'connected_caveman_graph(5, N // 20)' ${vertices} > $1/caveman.in

