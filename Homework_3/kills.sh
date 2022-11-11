if [ $# -eq 0 ] ; then
	PORT=2020
else
	PORT=$1
fi

echo "before:"
ps -aux | grep "[/]server 2020"
kill $(ps -aux | grep "[/]server 2020" | awk '{print $2}')
echo "after:"
ps -aux | grep "[/]server 2020"
