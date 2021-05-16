export TTY=$1
export BAUDRATE=$2
echo "baudrate " $BAUDRATE
sudo ./slattach -L -p slip /dev/tty$1 -s $BAUDRATE &
sudo ifconfig sl0 192.168.1.1 pointopoint 192.168.1.2 up mtu 1500
ifconfig sl0
