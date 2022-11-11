PORT=2020

./kills.sh >& /dev/null
make > /dev/null

./server $PORT 1 1 random &
SPID=$!
sleep 0.5

./client localhost $PORT output.cgi >& tmp/tmp1 &
CPID1=$!
sleep 0.2
./client localhost $PORT home.html >& tmp/tmp2 &
CPID2=$!
sleep 0.2
./client localhost $PORT home.html >& tmp/tmp3
CPID3=$!

wait $CPID1 $CPID2 $CPID3


kill $SPID
