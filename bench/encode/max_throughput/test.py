#!/usr/bin/python
# -*- coding: utf-8 -*-

import subprocess
import sys
# add path so that we could import stat
sys.path.append("../../")
from cmd_stat import get_command_execution_result

tests_dir = "../../../tests/"

def run_test(program, file, size, frame_size, api, api_name):
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
            cmd = "%s%s -i mlx5_0 -F %d -s %d -t %d -%s" % (tests_dir, program,
                    size, frame_size, i, api)
            result = get_command_execution_result(cmd)

            stdout = result["stdout"]
            avg_cpu_usage = result["avg_cpu_usage"]
            output = "%d\t%f\t" % (i, avg_cpu_usage) + stdout
            f.write(output)
            print output,
    print

if __name__ == "__main__":
	# make sure we are using the latest version of ibv_ec_single_thread_encoder
	subprocess.check_output("cd %s; make" % tests_dir, shell=True)
    # use verbs api, file size = 40GB
    # use up to 30 threads, not numa binded, not cpu binded
	run_test("ibv_ec_encoder_mem", "verbs.data", 42949672960, 41943040, "V", "verbs")

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
	run_test("ibv_ec_encoder_mem", "sw.data", 419430400, 1280, "S", "sw")
