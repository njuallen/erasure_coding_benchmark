set xlabel "Block size(Bytes)"
set xtics font ", 10"
set logscale x 2
set format x "%.0f"
set ylabel "Time(s)"
set title "Encode 1GB data with single thread"
set grid
set term jpeg
set output 'encode_best_block_size.jpg'

plot "sync_verbs.data" using 1:2 w lp pt 8 title "sync_verbs", \
     "async_verbs.data" using 1:2 w lp pt 12 title "async_verbs", \
	 "sw.data" using 1:2 w lp pt 5 title "jerasure"
