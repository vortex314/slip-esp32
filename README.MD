# Purpose
- Use USB connected microcontrollers as slave devices from Raspberry Pi 
- run SLIP or PPP via USB 
- run MQTT client on microcontroller
- run telnet CLI on microcontroller
- run tftp server on microcontroller for Over-The-Air updates
# build 
CONFIG_LWIP_NETIF_STATUS_CALLBACK=y
lieven@pcpro:~/esp/esp-idf/components/lwip$ cp -r lwip/src/apps/mqtt apps 

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
The delay looked huge on a 1Mbit line, measuring the delay within the LWIP stack was smaller than 1 mS
Then investigated what latency could occur on the USB line , and found these articles
- https://www.ftdichip.com/Documents/AppNotes/AN232B-04_DataLatencyFlow.pdf
- https://granitedevices.com/wiki/FTDI_Linux_USB_latency

So I applied the command 
```
$ sudo echo 1 >/sys/bus/usb-serial/devices/ttyUSB2/latency_timer
```
With the immediate effect on ping results. Hurrah ! 
```
38 bytes from 192.168.1.2: icmp_seq=1456 ttl=255 time=18.6 ms
38 bytes from 192.168.1.2: icmp_seq=1457 ttl=255 time=18.4 ms
38 bytes from 192.168.1.2: icmp_seq=1458 ttl=255 time=3.47 ms
38 bytes from 192.168.1.2: icmp_seq=1459 ttl=255 time=3.17 ms
38 bytes from 192.168.1.2: icmp_seq=1460 ttl=255 time=3.67 ms
```
# RSYSLOG logging via UDP on SLIP
- uncomment udp adapt config /etc/rsyslog.conf
```
module(load="imudp")
input(type="imudp" port="514")
```
- send user log to user.log , change /etc/rsyslog.d/50-default.conf

```
auth,authpriv.*                 /var/log/auth.log
# *.*;auth,authpriv.none                -/var/log/syslog
#cron.*                         /var/log/cron.log
#daemon.*                       -/var/log/daemon.log
kern.*                          -/var/log/kern.log
#lpr.*                          -/var/log/lpr.log
mail.*                          -/var/log/mail.log
user.*                          -/var/log/user.log

```

```

lieven@pcpro:/var/log$ sudo service rsyslog restart
lieven@pcpro:/var/log$ netstat -lnpu
```

# History
- May 10 : MQTT based on LWIP works fine till 50 msg/sec and UDP log till 100 logs/sec
- May 11 : PPP reliably 1.8 ms Ping time

# IP routing on ppd server
? how do i get DNS and internet routing
```
sudo iptables -t nat -A POSTROUTING -o ppp0 -j MASQUERADE
sudo sysctl -w net.ipv4.ip_forward=1

```

https://unix.stackexchange.com/questions/71500/iptables-forward-with-pptp-and-openvpn
iptables -t filter -F
iptables -t nat -F
iptables --flush
  iptables --table nat --flush
  iptables --delete-chain
  iptables --table nat --delete-chain
  iptables --table nat --append POSTROUTING --out-interface enp3s0 -j MASQUERADE
  iptables --append FORWARD --in-interface ppp0 -j ACCEPT
  iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT

  ```

