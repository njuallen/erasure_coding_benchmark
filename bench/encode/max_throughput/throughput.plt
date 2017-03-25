set xlabel "Number of threads/processes"
set ylabel "Throughput(GB/s)"
set title "Encode 40GB data"
set grid
set term jpeg
set output 'throughput.jpg'

plot "sync_verbs.data" using 1:(40/$3) w lp pt 8 title "sync_verbs", \
	 "async_verbs.data" using 1:(40/$3) w lp pt 8 title "async_verbs", \
	 "sw.data" using 1:(40/$3) w lp pt 5 title "Jerasure"
