if [ $# -eq 0 ] ; then
    PORT=2020
else
    PORT=$1
fi

INTERVAL=0.1
NUM_REQ=17

./kills.sh $PORT >& /dev/null
make > /dev/null

./server $PORT 1 1 dt &
SPID=$!
sleep 0.3

for INDEX in {1..4}
do
    ./client localhost $PORT output.cgi >& tmp/tmp$INDEX &
    sleep 0.03
done

sleep 2
kill $SPID
