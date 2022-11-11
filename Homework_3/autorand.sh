function expect(){
    if [ $# -eq 1 ] ; then
        if [ ! $(cat $1 | grep -c "") -eq 1 ] ; then
            echo "FAIL"
            return
        elif [ "$(cat $1)" != "Rio_readlineb error: Connection reset by peer" ] ; then
            echo "FAIL"
            return
        fi
        echo "SUCCESS"
        return
    fi
    if [ $# -eq 4 ] ; then
        if [ "$(cat $1 | grep Stat-thread-count | awk '{print $3}')" -ne $2 ] ; then
            echo "FAIL"
            return
        elif [ "$(cat $1 | grep Stat-thread-static | awk '{print $3}')" -ne $3 ] ; then
            echo "FAIL"
            return
        elif [ "$(cat $1 | grep Stat-thread-dynamic | awk '{print $3}')" -ne $4 ] ; then
            echo "FAIL"
            return
        fi
        echo "SUCCESS"
        return
    fi
    echo "expect() got wrong number of arguments!"
}

PORT=2020

./kills.sh >& /dev/null
make > /dev/null

./server $PORT 1 1 random &
SPID=$!
sleep 0.1

./client localhost $PORT output.cgi >& tmp1 &
CPID1=$!
sleep 0.05
./client localhost $PORT home.html >& tmp2 &
CPID2=$!
sleep 0.05
./client localhost $PORT home.html >& tmp3 &
CPID3=$!

wait $CPID1 $CPID2 $CPID3

expect tmp1 1 1 0
expect tmp2
expect tmp3 2 1 1

kill $SPID
