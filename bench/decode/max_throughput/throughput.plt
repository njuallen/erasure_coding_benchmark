set xlabel "Number of threads/processes"
set ylabel "Throughput(GB/s)"
set title "Decode 40GB data"
set grid
set term jpeg
set output 'throughput.jpg'

plot "verbs.data" using 1:(40/$3) w lp pt 8 title "Verbs", \
	 "sw.data" using 1:(40/$3) w lp pt 5 title "Jerasure"
