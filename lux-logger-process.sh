#!/bin/bash

filename=$1
interval=$2
duration=$3
init_dur=$4

minimum_level=1

# wait until we have sufficient light
lumens=0
while [ ${lumens%.*} -lt $minimum_level ]
do
  # update this with the IP address of the ESP8266
  lumens=$(curl -s 192.168.1.109/lumens)
done

echo "Reading,Datetime,Minutes,Lumens,Temperature" > $filename

reading=1
elapsed=0
keep_going=1
starttime=$(date '+%s.%N')
low_readings=0

while [ $keep_going -eq 1 ] && [ $low_readings -lt 5 ]
do
  currtime=$(date '+%s.%N')
  elapsed=$(awk "BEGIN {printf(\"%.3f\",($currtime - $starttime)/60)}")
  datetime=$(date '+%Y-%m-%d %H:%M:%S.%N')
  
  # update this with the IP address of the ESP8266
  result=$(curl -s 192.168.1.109/read)
  
  SaveIFS=$IFS
  IFS=","
  declare -a fields=($result)
  IFS=$SaveIFS
  lumens=${fields[0]}
  temp=${fields[1]}

  #if [ $lumens -lt $minimum_level ]; then
  #force INT comparison
  if [ ${lumens%.*} -lt $minimum_level ]; then
    (( low_readings++ ))
  fi
  
  echo "$reading,$datetime,$elapsed,$lumens,$temp" >> $filename

  endtime=$(date '+%s.%N')
  # for the first X minutes, poll every second no matter what the interval is
  int=$(awk "BEGIN {print (($endtime - $starttime) < ($init_dur * 60) ? 1 : $interval)}")
  sleep_time=$(awk "BEGIN {print $int - $endtime + $currtime}")
  sleep $sleep_time
  
  keep_going=$(awk "BEGIN {print (($endtime - $starttime) >= ($duration * 60) ? 0 : 1)}")
  
  (( reading++ ))
done
