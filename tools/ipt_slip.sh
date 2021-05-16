# iptables -t filter -F
# iptables -t nat -F
iptables --table filter --flush
iptables --table nat --flush
iptables --table filter --delete-chain
iptables --table nat --delete-chain

iptables --table nat --append POSTROUTING --out-interface enp3s0 -j MASQUERADE
iptables --append FORWARD --in-interface sl0 -j ACCEPT
iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT
  # iptables -t nat -A POSTROUTING -i ppp0 -o enp3sà -j SMAT --to-source 192.168.1.2
