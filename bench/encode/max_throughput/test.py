#!/usr/bin/python
# -*- coding: utf-8 -*-

import subprocess
import sys
# add path so that we could import stat
sys.path.append("../../")
from cmd_stat import get_command_execution_result

tests_dir = "../../../tests/"

def run_test(program, size, frame_size, api_name):
    """
    program: the program to run
    file: the file to store test result
    size: the size of all the data blocks to be encoded/decoded
    api: which encode/decode api to use
    api_name: the name of the encode/decode api
    """
    print api_name, ":"
    with open(api_name + ".data", "w") as f:
        for i in range(1, 30):
            cmd = "%s%s -i mlx5_0 -F %d -s %d -k 10 -m 4 -w 8 -t %d --max_inflight 10" % (tests_dir, program,
                    size, frame_size, i)
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
    # use sync verbs api, file size = 40GB
    # use up to 30 threads, not numa binded, not cpu binded
    # best block size = 256KB
    file_size = 42949672960
    run_test("ibv_ec_encoder_sync_verbs", file_size, 2621440, "sync_verbs")

    # use async verbs api, file size = 40GB
    # use up to 30 threads, not numa binded, not cpu binded
    # best block size = 256KB
    # set max_inflight to 10
    run_test("ibv_ec_encoder_async_verbs", file_size, 2621440, "async_verbs")

    # Use sw Jerasure api, file size = 40GB
    # best block size = 16KB
    run_test("ibv_ec_encoder_sw", file_size, 163840, "sw")
