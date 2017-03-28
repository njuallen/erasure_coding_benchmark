#!/usr/bin/python
# -*- coding: utf-8 -*-

import subprocess

tests_dir = "../../../tests/"

def run_test(program, size, api_name, thread_num):
	"""
	program: the program to run
	size: the size of all the data blocks to be encoded/decoded
	api_name: the name of the encode/decode api
	"""
	# number of data blocks
	k = 10
	# number of code blocks
	m = 4
	w = 4
	block_size = 64
	print api_name, ":"
	with open(api_name + ".data", "w") as f:
		for i in range(1, 19):
			# block size ranges from 64 bytes to 8MB
			frame_size = block_size * k
			cmd = "%s%s -i mlx5_0 -F %d -s %d -k 10 -m 4 -w 8 --max_inflight 20 -t %d" % (tests_dir, program,
					size, frame_size, thread_num)
			output = subprocess.check_output(cmd, shell=True)
			output = "%d\t" % block_size + output
			f.write(output)
			print output,
			block_size *= 2
	print

if __name__ == "__main__":
    # make sure we are using the latest version of ibv_ec_encoder_mem
    subprocess.check_output("cd %s; make" % tests_dir, shell=True)

    # 1GB
    file_size = 1073741824

    # use sync_verbs api, file size = 1GB
    # experiment show that best block size is 262144 bytes = 256KB
    # run_test("ibv_ec_encoder_sync_verbs", file_size, "sync_verbs", 1)

    # use async_verbs api, file size = 1GB
    # experiment show that best block size is 262144 bytes = 256KB
    # run_test("ibv_ec_encoder_async_verbs", file_size, "async_verbs", 1)

    # Use sw Jerasure api, file size = 1GB
    # experiment show that best block size is 16384 bytes = 16KB
    # run_test("ibv_ec_encoder_sw", file_size, "sw", 1)

    # 10GB
    file_size = 10737418240
    # use this to see the maximum encode ops of this NIC
    run_test("ibv_ec_encoder_sync_verbs", file_size, "nic_sync_ops", 12)
    run_test("ibv_ec_encoder_async_verbs", file_size, "nic_async_ops", 12)
    run_test("ibv_ec_encoder_sw", file_size, "sw_ops", 12)
