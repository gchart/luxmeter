#!/bin/bash

read -p 'Filename (no extension): ' filename
read -p 'Default interval (seconds) [15]: ' interval
read -p 'Maximum duration (minutes) [1440]: ' duration
read -p 'Duration of 1sec readings (minutes) [5]: ' init_dur

export config_file="$filename.txt"
export filename="$filename.csv"

interval=${interval:-15}
duration=${duration:-1440}
init_dur=${init_dur:-5}


datetime=$(date '+%Y-%m-%d %H:%M')
echo "Testing date and time: $datetime" > $config_file
echo "Default interval (seconds): $interval" >> $config_file
echo "Total duration (minutes): $duration" >> $config_file
echo "Initial duration (minutes): $init_dur" >> $config_file

echo $(nohup lux-logger-process.sh $filename $interval $duration $init_dur >/dev/null 2>&1 &)
echo Logging has begun.  Goodbye!
