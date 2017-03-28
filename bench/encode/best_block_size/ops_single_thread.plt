set xlabel "Block size(Bytes)"
set xtics font ", 10"
set logscale x 2
set format x "%.0f"
set format y "%.0f"
set ylabel "Encode OPS"
set title "Encode 1GB data with single thread"
set grid
set term jpeg
set output 'encode_ops_single_thread.jpg'

plot "sync_verbs.data" using 1:(1073741824/($1 * 10 * $2)) w lp pt 8 title "sync_verbs", \
     "async_verbs.data" using 1:(1073741824/($1 * 10 * $2)) w lp pt 12 title "async_verbs", \
	 "sw.data" using 1:(1073741824/($1 * 10 * $2)) w lp pt 5 title "jerasure"
