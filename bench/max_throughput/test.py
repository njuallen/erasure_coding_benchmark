#!/usr/bin/python
# -*- coding: utf-8 -*-

import subprocess

if __name__ == "__main__":
	# make sure we are using the latest version of ibv_ec_single_thread_encoder
	subprocess.check_output("cd ../../tests; make", shell=True)
	print "verbs:"
	with open("verbs.data", "w") as f:
		for i in range(1, 30):
			# use verbs api, file size = 40GB
			# use up to 30 threads, not numa binded, not cpu binded
			cmd = "../../tests/ibv_ec_encoder_mem -i mlx5_0 -F 42949672960 -s 41943040 -t %d -V" % i
			output = subprocess.check_output(cmd, shell=True)
			output = "%d\t" % i + output
			f.write(output)
			print output,
	print
	print "sw:"
	with open("sw.data", "w") as f:
		for i in range(1, 30):
			# Use sw Jerasure api, file size = 400MB
			# We should use small block size, such as 128bytes.
			# If we use large block size, such as 4MB
			# then, one thread will fetch all the work in one batch(4MB * 10 * 20).
			# where 20 is the batch size
			# Then other threads have no work to do and they will simply exit.
			# In this case, our multi-threading process becomes single threaded.
			# 
			# Our previous test shows that the running time of Jerasure library 
			# is insensitive to block size.
			# So we can make block size smaller,
			# and there will be enough work for all threads.
			cmd = "../../tests/ibv_ec_encoder_mem -i mlx5_0 -F 419430400 -s 1280 -t %d -S" % i
			output = subprocess.check_output(cmd, shell=True)
			output = "%d\t" % i + output
			f.write(output)
			print output,
