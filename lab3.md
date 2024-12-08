bancada 11

tux2- 2 - lab2
tux3- 3
tux4 - e1 - 4
tux4 - e2 - 12 - lab3


# lab3:
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


tirar pints do tuxY4.eth1 e tuxY4.eth2 -> temos print

reconfigurar o tux3 e o tux2 para conseguirem comunicar atravez do tux4:
- 3 -> route add -net 172.16.101.0/24 gw 172.16.100.254
- 2 -> route add -net 172.16.100.0/24 gw 172.16.101.253

Observe the routes available at the 3 tuxes: - temos print
nos 3 tuxs: route  -n


tux3: start wireshark capture
ping 172.16.100.254
ping 172.16.101.253
ping 172.16.101.1

tirar prints do wireshark -> done

wireshark no tux4, um para o eth1 e o outro para o eth2
dar clean as arp tables nos 3 tuxes - arp -d <>
mandar mensagem do tux3 para o tux2
parar e guardar logs -> temos prints e logs

