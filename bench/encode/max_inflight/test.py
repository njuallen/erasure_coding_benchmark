#!/usr/bin/python
# -*- coding: utf-8 -*-

import subprocess

tests_dir = "../../../tests/"

def run_test(program, size, frame_size, api_name):
	"""
	program: the program to run
	size: the size of all the data blocks to be encoded/decoded
	api_name: the name of the encode/decode api
	"""
	print api_name, ":"
	with open(api_name + ".data", "w") as f:
		for i in range(1, 24):
			cmd = "%s%s -i mlx5_0 -F %d -s %d -k 10 -m 4 -w 8 -t 1 --max_inflight %d" % (tests_dir, program,
					size, frame_size, i)
			output = subprocess.check_output(cmd, shell=True)
			output = "%d\t" % i + output
			f.write(output)
			print output,
	print

if __name__ == "__main__":
    # make sure we are using the latest version of ibv_ec_encoder_mem
    subprocess.check_output("cd %s; make" % tests_dir, shell=True)
    # use async_verbs api, file size = 40GB
    # experiment show that best block size is 262144 bytes = 256KB
    file_size = 42949672960
    run_test("ibv_ec_encoder_async_verbs", file_size, 2621440, "async_verbs")
