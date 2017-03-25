set ylabel "average CPU usage(%)"
set yrange [0:100]
set title "Encode 40GB data"
set term jpeg
set style data histogram
set style fill solid
set output 'cpu_usage.jpg'

plot "cpu_usage.data" using 2:xticlabels(1) title "sync_verbs", \
	 "cpu_usage.data" using 3:xticlabels(1) title "async_verbs", \
	 "cpu_usage.data" using 4:xticlabels(1) title "Jerasure"
