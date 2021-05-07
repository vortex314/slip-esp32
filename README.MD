# Purpose
- Use USB connected microcontrollers as slave devices from Raspberry Pi 
- run SLIP via USB 
- run MQTT client on microcontroller
- run telnet CLI on microcontroller
# build 
CONFIG_LWIP_NETIF_STATUS_CALLBACK=y


# adapt slattach 
The standard slattach is available on a lot of linux systems, however, it's not adapted to accept any baudrate higher than 115200. 

- download from https://sourceforge.net/projects/net-tools/
- cd net-tools
- change slattach.c with this copy 
- make

# Issues
```/home/lieven/esp/esp-idf/components/lwip/lwip/src/netif/slipif.c:437: undefined reference to `sio_tryread'```
- related to ???

# Run
- start slattach 
```
$ sudo ./slattach -L -p slip /dev/ttyUSB1 -s 1000000
```
- ifconfig 
```
$ sudo ifconfig sl0 192.168.1.1 pointopoint 192.168.1.2 up mtu 1500
$ ifconfig sl0
ifconfig sl0
sl0: flags=4305<UP,POINTOPOINT,RUNNING,NOARP,MULTICAST>  mtu 1500
        inet 192.168.1.1  netmask 255.255.255.255  destination 192.168.1.2
        slip  txqueuelen 10  (Serial Line IP)
        RX packets 261  bytes 21924 (21.9 KB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 476  bytes 41024 (41.0 KB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```
- test with ping 
```
lieven@pcpro:~$ ping 192.168.1.2
PING 192.168.1.2 (192.168.1.2) 56(84) bytes of data.
64 bytes from 192.168.1.2: icmp_seq=1 ttl=255 time=17.8 ms
64 bytes from 192.168.1.2: icmp_seq=2 ttl=255 time=17.8 ms
64 bytes from 192.168.1.2: icmp_seq=3 ttl=255 time=17.7 ms
^C
--- 192.168.1.2 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2002ms
rtt min/avg/max/mdev = 17.739/17.789/17.819/0.035 ms
```
lieven@pcpro:/var/log$ sudo service rsyslog restart
lieven@pcpro:/var/log$ netstat -lnpu
