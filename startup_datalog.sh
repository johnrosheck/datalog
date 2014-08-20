#!/bin/bash
#
#
echo "starting datalogging..."

# the program should be running so start the get_resp program
datetime=`date +%Y%m%d`
#echo $datetime
/home/web/interface/get_resp/get_resp -l /home/web/data/datalog/dl >& /home/web/data/dl-$datetime.txt &
#/home/datalog/control/gh-sys/cmnd_sys -p /home/datalog/data/control/cmnd_sys.pid \
#-c /home/datalog/data/control/control_settings.txt \
#-l /home/datalog/data/datalog/gh >& /home/datalog/data/gh-$datetime.log &

cd /home/web/datalog
./dl_daemon.pl >& /home/web/data/dd-$datetime.log &

echo "all done."

exit 0
