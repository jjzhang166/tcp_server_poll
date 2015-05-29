#!/bin/bash

readonly TODAY=`date +%Y%m%d`
echo "TODAY is ${TODAY}"
readonly SERVER_IP="172.16.100.28"
readonly SERVER_PORT=8888


exec {fd}<>/dev/tcp/${SERVER_IP}/${SERVER_PORT}
printf "hello_man\001\002oops\n" >&$fd

echo "reply_msg:"
while read -r line; do
    echo "$line"
done <&$fd
echo "dumping any remaining data in the fd"
echo "closing i/o channels..."
exec $fd<&-
exec $fd>&-
