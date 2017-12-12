#!/usr/bin/env python3

import networkx as nx
import sys, os
sys.path.insert(1, os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
from utils import io_utils

print(nx.stoer_wagner(io_utils.read_stdin()))
