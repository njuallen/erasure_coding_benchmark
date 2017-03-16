#!/usr/bin/python
# -*- coding: utf-8 -*-

import subprocess

tests_dir = "../../../tests/"

def run_test(program, file, size, api, api_name, erasure):
	"""
	program: the program to run
	file: the file to store test result
	size: the size of all the data blocks to be encoded/decoded
	api: which encode/decode api to use
	api_name: the name of the encode/decode api
	"""
	print api_name, ":"
	with open(file, "w") as f:
		for i in range(1, 30):
			cmd = "%s%s -i mlx5_0 -F %d -s 41943040 -t %d -%s -E %s" % (tests_dir, program,
					size, i, api, erasure)
			output = subprocess.check_output(cmd, shell=True)
			output = "%d\t" % i + output
			f.write(output)
			print output,
	print

if __name__ == "__main__":
	# make sure we are using the latest version of ibv_ec_single_thread_encoder
	subprocess.check_output("cd %s; make" % tests_dir, shell=True)
	erasure = "1,1,1" + 11 * ",0" 
	# use verbs api, file size = 40GB
	run_test("ibv_ec_decoder_mem", "verbs.data", 42949672960, "V", "verbs", erasure)
