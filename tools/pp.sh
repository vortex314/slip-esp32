sudo pppd /dev/ttyUSB0 1000000 192.168.1.1:192.168.1.2 netmask 255.255.255.0 noauth local  dump  nocrtscts persist maxfail 0 holdoff 1 
