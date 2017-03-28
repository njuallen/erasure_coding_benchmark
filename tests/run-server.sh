# A function to echo in blue color
function blue() {
	es=`tput setaf 4`
	ee=`tput sgr0`
	echo "${es}$1${ee}"
}

export HRD_REGISTRY_IP="114.212.85.170"

blue "Reset server QP registry"
sudo killall memcached
memcached -l 0.0.0.0 1>/dev/null 2>/dev/null &

sleep 1
./ibv_server -i mlx5_0 -p 0 -g 0 -s 163840 -k 10 -m 4 -d -v

sleep 1
