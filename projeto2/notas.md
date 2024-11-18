# Part 2 / Exp 1- Configure an IP Network
## 1.Connect E1 of tuxY3 and E1 of tuxY4 to the switch
Bancada 12
- gnu32 E1 -> 9 (Cloud Router Switch)
- gnu33 E1 -> 1 (Cloud Router Switch)

## 2.Configure eth1 interface of tuxY2 and  eth1 interface of tuxY3
- 2 -> ifconfig eth1 172.16.30.1
- 3 -> ifconfig eth1 172.16.30.254

## 4.Use ping command to verify connectivity between these computers
- 2 -> ping 172.16.30.254
- 3 -> ping 172.16.30.1

## 5.Inspect forwarding (route -n) and ARP (arp -a) tables

fotos



## 6.Delete ARP table entries in tuxY3 (arp -d ipaddress)
## 7.Start Wireshark in tuxY3.eth1 and start capturing packets
## 8.In tuxY3 , ping tuxY4 for a few seconds
## 9.Stop capturing packets
## 10.Stop capturing packets