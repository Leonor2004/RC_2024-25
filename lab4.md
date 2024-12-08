bancada 11

tux2- 2 - lab2
tux3- 3
tux4 - e1 - 4
tux4 - e2 - 12 - lab3


# lab4:
ligação 1&2 - tux3-tux4-tux2
- 3 -> ifconfig eth1 172.16.100.1/24
- 4 -> ifconfig eth1 172.16.100.254/24
- 4 -> ifconfig eth2 172.16.101.253/24
- 2 -> ifconfig eth1 172.16.101.1/24

gtkTerm -> /system reset-configuration
user: admin
password: <enter>

creating a bridge
/interface bridge add name=bridge110
/interface bridge add name=bridge111

confirmar bridges criadas:
/interface bridge print

remove the ports
/interface bridge port remove [find interface =ether2]
/interface bridge port remove [find interface =ether3]
/interface bridge port remove [find interface =ether4]
/interface bridge port remove [find interface =ether12]

confirmar que apagamos as ports:
/interface bridge port print

add the ports to the corresponding bridges
/interface bridge port add bridge=bridge111 interface=ether2
/interface bridge port add bridge=bridge110 interface=ether3
/interface bridge port add bridge=bridge110 interface=ether4
/interface bridge port add bridge=bridge111 interface=ether12

confirmar que adicionamos as ports às bridges certas:
/interface bridge port print

confirmação dos pings:
- 3 -> ping 172.16.100.254
- 2 -> ping 172.16.101.253
- 4 -> ping 172.16.100.1
- 4 -> ping 172.16.101.1

no tux4:
sysctl net.ipv4.ip_forward=1
sysctl net.ipv4.icmp_echo_ignore_broadcasts=0


tirar pints do tuxY4.eth1 e tuxY4.eth2 -> temos print (tirar outra vez no novo pc :| )

reconfigurar o tux3 e o tux2 para conseguirem comunicar atravez do tux4:
- 3 -> route add -net 172.16.100.0/24 gw 172.16.100.254
- 2 -> route add -net 172.16.101.0/24 gw 172.16.101.253



//passo1:
cabos:
ether1 of RC to the lab network on PY.12
ether2 of RC to a port on bridgeY1 - 22

gtk:
/interface bridge port remove [find interface =ether22]
/interface bridge port add bridge=bridge??1 interface=ether22

mudar o cabo da porta serie para o router

gtk: (não parece que mudou, mas mudou)
/system reset-configuration

/ip address add address=172.16.1.109/24 interface=ether1 (ou: /ip address add address=172.16.1.0/24 interface=ether1 )
/ip address add address=172.16.101.254/24 interface=ether2

- 2 -> route add default gw 172.16.101.254
- 3 -> route add default gw 172.16.100.254
- 4 -> route add default gw 172.16.101.254

/ip route add dst-address=172.16.100.0/24 gateway=172.16.101.253
/ip route add dst-address=0.0.0.0/0 gateway=172.16.1.254

já deve estar feito -> 2 -> route add -net 172.16.101.0/24 gw 172.16.101.253

no tux3 - todos tem de dar resposta: -> pints yay
ping 172.16.100.254
ping 172.16.101.1
ping 172.16.101.254

no tux2:
sysctl net.ipv4.conf.eth0.accept_redirects=0
sysctl net.ipv4.conf.all.accept_redirects=0

route del -net 172.16.100.0 gw 172.16.101.253 netmask 255.255.255.0

ping 172.16.100.1

traceroute -n 172.16.100.1 -> temos print
route add -net 172.16.100.0/24 gw 172.16.101.253
traceroute -n 172.16.100.1 -> temos print 2

sysctl net.ipv4.conf.eth0.accept_redirects=1
sysctl net.ipv4.conf.all.accept_redirects=1

no tux3: (deve responder)                              -> apartir daqui é preciso netlab, yay
ping 172.16.1.254

router console: 
/ip firewall nat disable 0

no tux3: (não deve responde)
ping 172.16.1.254

router console: 
/ip firewall nat enable 0

