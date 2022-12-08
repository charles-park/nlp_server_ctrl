# nlp_server_ctrl
ODROID Network label printer &amp; iperf control server test app.

### Ctrl options
```
/* find server command : nmap {192.168.*.*} -p T:8888 --open */
"  -f --find_nlp  find network printer server ip.(T:8888)\n"
"  -p --mac_print print the macaddress of current device.(find 00:1e:06:xx:xx:xx)\n"
"  -a --nlp_addr  Network printer server ip address.(default = 192.168.0.0)\n"
"  -c --channel   Message channel (left or right, default = left)\n"

/* iperf3 udp test command : ipref3 -c {server ip} -t 1 -b G -u */
"  -u --iperf3    iperf3 udp client speed check\n"
/* iperf3 udp test command : ipref3 -c {server ip} -t 1 */
"  -i --iperf3    iperf3 tcp client speed check\n "

"  -e --msg       error msg print"
"  -m --macaddr   print mac address.\n"
"                 mac address data form is 001e06??????\n"
"   e.g) nlp_server_test -a 192.168.20.10 -c left -t error -m usb,sata,hdd\n"
"        nlp_server_test -f -c right -e usb,sata,nvme\n"
"        nlp_server_test -f -c right -m 001e06234567\n"
"        nlp_server_test -f [-u|-i]\n"
```
