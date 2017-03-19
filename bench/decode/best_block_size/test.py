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
	# number of data blocks
	k = 10
	# number of code blocks
	m = 4
	w = 4
	block_size = 64
	print api_name, ":"
	with open(file, "w") as f:
		for i in range(1, 19):
			# block size ranges from 64 bytes to 8MB
			frame_size = block_size * k
			cmd = "%s%s -i mlx5_0 -F %d -s %d -t 1 -k 10 -m 4 -w 8 -%s -E %s" % (tests_dir, program,
					size, frame_size, api, erasure)
			output = subprocess.check_output(cmd, shell=True)
			output = "%d\t" % block_size + output
			f.write(output)
			print output,
			block_size *= 2
	print

if __name__ == "__main__":
	# make sure we are using the latest version of ibv_ec_encoder_mem
	subprocess.check_output("cd %s; make" % tests_dir, shell=True)
	erasure = "1,1,1" + 11 * ",0" 
	# use verbs api, file size = 1GB
	run_test("ibv_ec_decoder_verbs", "verbs.data", 1073741824, "V", "verbs", erasure)

	# use jerasure api, file size = 1GB
	run_test("ibv_ec_decoder_sw", "sw.data", 1073741824, "S", "sw", erasure)
