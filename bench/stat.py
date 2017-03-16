#!/usr/bin/env python
# -*- coding: utf-8 -*-

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
    with open('/proc/' + str(pid) + '/stat') as f:
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
