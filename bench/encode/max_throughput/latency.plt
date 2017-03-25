set ylabel "Time(s)"
set yrange [0:20]
set title "Encode 40GB data"
set term jpeg
set style data histogram
set style fill solid
set output 'latency.jpg'

plot "latency.data" using 2:xticlabels(1) title "sync_verbs", \
	 "latency.data" using 3:xticlabels(1) title "async_verbs", \
	 "latency.data" using 4:xticlabels(1) title "Jerasure"
