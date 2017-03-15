#!/usr/bin/python
# -*- coding: utf-8 -*-

import subprocess

if __name__ == "__main__":
	# make sure we are using the latest version of ibv_ec_encoder_mem
	print subprocess.check_output("cd ../../tests; make", shell=True)
	# number of data blocks
	k = 10
	# number of code blocks
	m = 4
	w = 4
	block_size = 64
	print "verbs:"
	with open("verbs.data", "w") as f:
		for i in range(1, 19):
			# use verbs api, file size = 1GB
			# block size ranges from 64 bytes to 8MB
			frame_size = block_size * k
			cmd = "../../tests/ibv_ec_encoder_mem -i mlx5_0 -F 1073741824 -s %d -t 1 -V" % frame_size
			output = subprocess.check_output(cmd, shell=True)
			output = "%d\t" % block_size + output
			f.write(output)
			print output,
			block_size *= 2
	print
	# reset block_size to 64 bytes
	block_size = 64
	print "sw:"
	with open("sw.data", "w") as f:
		for i in range(1, 19):
			# Use sw Jerasure api, file size = 80MB
			# block size ranges from 64 bytes to 8MB
			frame_size = block_size * k
			cmd = "../../tests/ibv_ec_encoder_mem -i mlx5_0 -F 83886080 -s %d -t 1 -S" % frame_size
			output = subprocess.check_output(cmd, shell=True)
			output = "%d\t" % block_size + output
			f.write(output)
			print output,
			block_size *= 2
