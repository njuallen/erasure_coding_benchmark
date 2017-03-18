#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys

# on my machine clktck = 100
Hertz = 100

def meminfo():
    # Return the information in /proc/meminfo as a dictionary
    meminfo = {} 

    with open('/proc/meminfo') as f:
        for line in f:
            name = ''
            value = ''
            l = line.split(':')
            if len(l) == 2:
                # do not take the "kB" following the numbers
                value = l[1].strip().split()[0]
            else:
                value = ' '
            name = l[0].strip()
            meminfo[name] = value
            
    return meminfo

def get_pid_stat(pid):
    """
    Return the content of /proc/$pid/stat as a list.
    """
    with open("/proc/" + str(pid) + "/stat") as f:
        line = f.read()
        l = line.split(' ')
        return l

def get_machine_up_time():
	"""
	Get the uptime of the system in seconds.
	"""
	with open("/proc/uptime") as f:
		line = f.read()
		l = line.split(" ")
		return float(l[0])

def get_pid_avg_cpu_usage(pid):
	"""
	Get the average cpu usage of a process in percents.
	To understand the simple equations, please read the links below:
	http://stackoverflow.com/questions/16726779/how-do-i-get-the-total-cpu-usage-of-an-application-from-proc-pid-stat
	http://unix.stackexchange.com/a/58541
	http://stackoverflow.com/a/3017438/1806289
	http://stackoverflow.com/a/1424556/1806289
	"""
	stat = get_pid_stat(pid)
	# utime - CPU time spent in user code, measured in clock ticks
	utime = int(stat[13])
	# stime - CPU time spent in kernel code, measured in clock ticks
	stime = int(stat[14])
	# cutime - Waited-for children's CPU time spent in user code (in clock ticks)
	cutime = int(stat[15])
	# cstime - Waited-for children's CPU time spent in kernel code (in clock ticks)
	cstime = int(stat[16])
	# starttime - Time when the process started, measured in clock ticks
	starttime = int(stat[21])

	# determine the total time spent for the process
	total_time = utime + stime
	# include the time from children processes
	total_time = total_time + cutime + cstime

	uptime = get_machine_up_time()

	# get the total elapsed time in seconds since the process started
	seconds = uptime - (starttime / Hertz)

	cpu_usage = 100 * ((total_time / Hertz) / seconds)

	return cpu_usage

def get_command_execution_result(cmd):
    """
    Get the execution result of a shell command.
    Including stdout, stderr, exit status, exit_signal and average_cpu_usage.
    The returned result is a json.
    """
    # file descriptors r, w for reading and writing
    out_r, out_w = os.pipe() 
    err_r, err_w = os.pipe() 
    pid = os.fork()
    if pid:
        # This is the parent process 
        os.close(out_w)
        os.close(err_w)
        out_r = os.fdopen(out_r)
        err_r = os.fdopen(err_r)

        # collect child stdout and stderr
        stdout = out_r.read()
        stderr = err_r.read()

        # we should not do this after os.wait
        # since after the child is waited and reaped
        # the corresponding status file "/proc/pid/stat" may not exist
        # in that case, we will get an file not exist error
        avg_cpu_usage = get_pid_avg_cpu_usage(pid)

        # get the exit status of child process
        status = os.wait()
        ret = {
                "status": status,
                "stdout": stdout,
                "stderr": stderr,
                "avg_cpu_usage": avg_cpu_usage
                }
        return ret
    else:
        # This is the child process
        os.close(out_r)
        os.close(err_r)

        # redirect stdout and stderr
        os.dup2(out_w, sys.stdout.fileno())
        os.dup2(err_w, sys.stderr.fileno())

        l = cmd.split()
        # l[0] is the path of the executable
        os.execv(l[0], l)

if __name__=='__main__':
    print get_command_execution_result("/usr/bin/python test.py 1000000")
