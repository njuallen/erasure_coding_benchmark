set xlabel "Number of threads/processes"
set ylabel "Time(s)"
set title "Encode 40GB data"
set grid
set term jpeg
set output 'running_time.jpg'

plot "verbs.data" using 1:3 w lp pt 8 title "Verbs", \
	 "sw.data" using 1:3 w lp pt 5 title "Jerasure"
