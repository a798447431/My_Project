#!/bin/bash
DyAver=$1

if [[ ${DyAver}x == "x" ]]; then
    exit 1
fi

MemValue=(`free -m | grep -w Mem | awk '{printf("%d %d"), $3, $2}'`)
MemLeft=`echo "${MemValue[1]}-${MemValue[0]}" | bc`
MemAvaPrec=`echo "scale=1;${MemValue[0]}*100/${MemValue[1]}" | bc`
TIME=$(date +"%Y-%m-%d__%H:%M:%S")

DyAver=`echo "scale=1;${DyAver}*0.3+${MemAvaPrec}*0.7" | bc`

echo "${TIME} ${MemValue[1]}M ${MemLeft}M ${MemAvaPrec}% ${DyAver}%"
