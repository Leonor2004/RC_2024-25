# Part 2 / Exp 1- Configure an IP Network
## 1.Connect E1 of tuxY3 and E1 of tuxY4 to the switch
Bancada 12
Cabos:
tux2 E1 -> 2
tux3 E1 -> 3
tux4 E1 -> 8
tux4 E2 -> 7
porta serie ligada ao 3

## 2.Configure eth1 interface of tuxY3 and  eth1 interface of tuxY4
- 3 -> ifconfig eth1 172.16.30.1/24
- 4 -> ifconfig eth1 172.16.30.254/24

## 4.Use ping command to verify connectivity between these computers
- 3 -> ping 172.16.30.254
- 4 -> ping 172.16.30.1

## 5.Inspect forwarding (route -n) and ARP (arp -a) tables

fotos

## 6.Delete ARP table entries in tuxY3 (arp -d ipaddress)
## 7.Start Wireshark in tuxY3.eth1 and start capturing packets
## 8.In tuxY3 , ping tuxY4 for a few seconds
## 9.Stop capturing packets
## 10. Save the log and study it at home
Done -> fotos yay




# Part 2 / Exp 2- Implement two bridges in a switch
fazer passos 1,2 e 3
2 -> ifconfig eth0 172.16.31.1/24

slide 48
/system reset configuration -> mandar as configs das bridges com as couves

slide 50
- creating a bridge
- remove port1 from bridge
- add port1 to bridge Y0

gtkTerm -> /system reset-networking

/interface bridge add name=bridge120
/interface bridge add name=bridge121

/interface bridge port remove [find interface =ether2]
/interface bridge port remove [find interface =ether3]
/interface bridge port remove [find interface =ether8]
/interface bridge port remove [find interface =ether7]


/interface bridge port add bridge=bridge121 interface=ether2
/interface bridge port add bridge=bridge121 interface=ether8

/interface bridge port add bridge=bridge120 interface=ether3
/interface bridge port add bridge=bridge120 interface=ether7









