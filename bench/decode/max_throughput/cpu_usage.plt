set xlabel "Number of threads/processes"
set ylabel "average CPU usage/%"
set ytics 200
set mytics 2
set title "Decode 40GB data"
set grid
set term jpeg
set output 'cpu_usage.jpg'

plot "verbs.data" using 1:2 w lp pt 8 title "Verbs", \
	 "sw.data" using 1:2 w lp pt 5 title "Jerasure"
