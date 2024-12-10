NÃO FINAL

# Bancada 12
## Cabos:
- tux2 E1 -> 2
- tux3 E1 -> 3
- tux4 E1 -> 8
- tux4 E2 -> 7
- porta serie ligada ao tux 3 (prof)

## SSH (eth0) (ESTAR NO TUX3):
- tux2 -> ssh 10.227.20.122 (ssh)
- tux4 -> ssh 10.227.20.124

trouble shooting: systemctl restart networking

## configurar:
### ligação 1 - tux3-tux4
- 3 -> ifconfig eth1 172.16.30.1/24
- 4 -> ifconfig eth1 172.16.30.254/24

### ligação 2 - tux2-tux4
- 2 -> ifconfig eth1 172.16.31.1/24
- 4 -> ifconfig eth1 172.16.31.253/24

## testar:
### ligação 1 - tux3-tux4
- 3 -> ping 172.16.30.254
- 4 -> ping 172.16.30.1

### ligação 2 - tux2-tux4
- 2 -> ping 172.16.31.253
- 4 -> ping 172.16.31.1


## gtkTerm -> /system reset-configuration
user: admin
password: <enter>

### creating a bridge
/interface bridge add name=bridge120
/interface bridge add name=bridge121

### remove port1 from bridge
/interface bridge port remove [find interface =ether2]
/interface bridge port remove [find interface =ether3]
/interface bridge port remove [find interface =ether8]
/interface bridge port remove [find interface =ether7]

### add port1 to bridge Y0
/interface bridge port add bridge=bridge121 interface=ether2
/interface bridge port add bridge=bridge121 interface=ether8

/interface bridge port add bridge=bridge120 interface=ether3
/interface bridge port add bridge=bridge120 interface=ether7



