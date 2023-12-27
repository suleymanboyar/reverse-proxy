# test file creation manual

#!/bin/bash

BIN_PATH="../.."
xterm -ls -e "$BIN_PATH/proxy 7655" &

read -n 1 -p "Press key when proxy has started."

xterm -ls -e "$BIN_PATH/anyReceiver X B 127.0.0.1 7655 10" &
sleep 5

nc 127.0.0.1 7655 < nc-send.bin

