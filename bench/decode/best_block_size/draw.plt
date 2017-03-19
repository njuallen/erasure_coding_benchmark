set xlabel "Block size/Bytes"
set xtics font ", 10"
set logscale x 2
set format x "%.0f"
set ylabel "Time/s"
set title "Decode 1GB data"
set grid
set term jpeg
set output 'decode_best_block_size.jpg'

plot "verbs.data" using 1:2 w lp pt 8 title "Verbs", \
	 "sw.data" using 1:2 w lp pt 5 title "Jerasure"
