#!/usr/bin/env python
# -*- coding: utf-8 -*-

from time import sleep
import sys

"""
Use this to test get_pid_avg_cpu_usage in stat.py
"""

if __name__=='__main__':
	assert len(sys.argv) == 2, "Expect a iteration"
	n = int(sys.argv[1])
	interval = 0.1
	while True:
		for i in range(n):
			pass
		sleep(interval)
