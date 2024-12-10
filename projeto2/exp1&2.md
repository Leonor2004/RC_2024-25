bancada 6

tux2-2 - lab2
tux3-3
tux4 - e1 - 4
tux4 - e2 - 10 - lab3


# lab1:
ligação 1 - tux3-tux4
- 3 -> ifconfig eth1 172.16.60.1/24
- 4 -> ifconfig eth1 172.16.60.254/24

- 3 -> ping 172.16.60.254
- 4 -> ping 172.16.60.1

route -n - temos print disto
arp -a - temos print disto
wireshark logs - temos print disto

# lab2:
- 2 -> ifconfig eth1 172.16.61.1/24

gtkTerm -> /system reset-configuration
user: admin
password: <enter>

creating a bridge
/interface bridge add name=bridge60
/interface bridge add name=bridge61

seing the bridges:
/interface bridge print (temos print disto)

remove the ports
/interface bridge port remove [find interface =ether2]
/interface bridge port remove [find interface =ether3]
/interface bridge port remove [find interface =ether4]
(/interface bridge port remove [find interface =ether10]) - lab3

add the ports to the corresponding bridges
/interface bridge port add bridge=bridge61 interface=ether2
/interface bridge port add bridge=bridge60 interface=ether4
/interface bridge port add bridge=bridge60 interface=ether3





