- For the mean time, id will only contain the host:port.
- using 0.0.0.0 will not display all of the ips of network devices available
- adding portproxy might be necessary (windows)
    - `netsh interface portproxy add v4tov4 connectaddress=127.0.0.1 listenport=1101 listenaddress=192.168.1.14 connectport=1101`
- there should be an equivalent way in other platform (iptables?)