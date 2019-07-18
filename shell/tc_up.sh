#!/bin/bash 
# 5900+N, 3389 port under 70Mbps
INTERFACE=ovirtmgmt
RDPPORT=3389
VNCPORT=5900
MAXVDESKTOP=50
LOWERBAND=80mbit
UPPERBAND=100mbit
INTERFACEBAND=1000mbit

tc qdisc add dev $INTERFACE root handle 1: htb default 10
tc class add dev $INTERFACE parent 1: classid 1:1 htb rate 1kbit ceil $INTERFACEBAND burst 15k
for((i=VNCPORT;i<=VNCPORT+MAXVDESKTOP;i++));  
do   
tc class add dev $INTERFACE parent 1:1 classid 1:$i htb rate $LOWERBAND ceil $UPPERBAND burst 15k
tc qdisc add dev $INTERFACE parent 1:$i handle $i: sfq perturb 10 
tc filter add dev $INTERFACE protocol ip parent 1:0 prio 1 u32 \
match ip sport $i 0xffff flowid 1:$i
done  
tc class add dev $INTERFACE parent 1:1 classid 1:$RDPPORT htb rate $LOWERBAND ceil $UPPERBAND burst 15k
tc qdisc add dev $INTERFACE parent 1:$RDPPORT handle $RDPPORT: sfq perturb 10 
tc filter add dev $INTERFACE protocol ip parent 1:0 prio 1 u32 \
match ip sport $RDPPORT 0xffff flowid 1:$RDPPORT

echo "Finish."
