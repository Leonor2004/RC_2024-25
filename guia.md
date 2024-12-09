bancada 11

cabo:
    tux3- 3
    tux4 - e1 - 4


- 4 -> ifconfig eth1 172.16.110.254/24
- 3 -> ifconfig eth1 172.16.110.1/24

- 3 -> ping 172.16.110.254


gtkTerm -> /system reset-configuration

creating as bridges:
    /interface bridge add name=bridge110
    /interface bridge add name=bridge111
    /interface bridge print

remove the ports
    /interface bridge port remove [find interface =ether3]
    /interface bridge port remove [find interface =ether4]

add the ports to the corresponding bridges
    /interface bridge port add bridge=bridge110 interface=ether3
    /interface bridge port add bridge=bridge110 interface=ether4
    /interface bridge port print

- 3 -> ping 172.16.110.254
- 4 -> ping 172.16.110.1

-----
cabo:
    tux4 - e2 - 12 - lab3
    tux2- 2 - lab2

- 2 -> ifconfig eth1 172.16.111.1/24
- 4 -> ifconfig eth2 172.16.111.253/24

gtk:
    /interface bridge port remove [find interface =ether2]
    /interface bridge port remove [find interface =ether12]
    /interface bridge port add bridge=bridge111 interface=ether2
    /interface bridge port add bridge=bridge111 interface=ether12

- 2 -> ping 172.16.111.253
- 4 -> ping 172.16.111.1

- 3 -> ping 172.16.110.254
- 4 -> ping 172.16.110.1

2 conneções a funcionar: done/not done

--------------------------------------------------------------------------

no tux4:
    sysctl net.ipv4.ip_forward=1
    sysctl net.ipv4.icmp_echo_ignore_broadcasts=0

- 3 -> route add -net 172.16.111.0/24 gw 172.16.110.254
- 2 -> route add -net 172.16.110.0/24 gw 172.16.111.253

- 3 -> ping 172.16.111.1
- 4 -> ping 172.16.110.1

4 como router : done/not done

--------------------------------------------------------------------------

cabos:
    ether1 of RC to the lab network on PY.12
    ether2 of RC to a port on bridgeY1 - 22

gtk:
    /interface bridge port remove [find interface =ether22]
    /interface bridge port add bridge=bridge111 interface=ether22

cabo:
    mudar o cabo da porta serie para o router

gtk: /system reset-configuration
    /ip address add address=172.16.1.119/24 interface=ether1
    /ip address add address=172.16.111.254/24 interface=ether2

- 2 -> route add default gw 172.16.111.254
- 3 -> route add default gw 172.16.110.254
- 4 -> route add default gw 172.16.111.254

gtk:
    /ip route add dst-address=172.16.110.0/24 gateway=172.16.111.253
    /ip route add dst-address=0.0.0.0/0 gateway=172.16.1.254


no tux3 - todos tem de dar resposta: -> prints yay
    ping 172.16.110.254
    ping 172.16.111.1
    ping 172.16.111.254

no tux2:
    sysctl net.ipv4.conf.eth0.accept_redirects=0
    sysctl net.ipv4.conf.all.accept_redirects=0

    route del -net 172.16.110.0 gw 172.16.111.253 netmask 255.255.255.0

    ping 172.16.110.1

    traceroute -n 172.16.110.1 -> temos print
    route add -net 172.16.110.0/24 gw 172.16.111.253
    traceroute -n 172.16.110.1 -> temos print 2

    sysctl net.ipv4.conf.eth0.accept_redirects=1
    sysctl net.ipv4.conf.all.accept_redirects=1

no tux3: (deve responder)
    ping 172.16.1.10

router console: 
    /ip firewall nat disable 0

no tux3: (não deve responde)
    ping 172.16.1.10

router console: 
    /ip firewall nat enable 0


(neste momento está igual ao lab4, mas vai mudar)