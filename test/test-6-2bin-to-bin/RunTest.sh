# test file creation ./makeTestfiles A B X 12147
# test file creation ./makeTestfiles B B X 12148

#!/bin/bash

BIN_PATH="../.."
xterm -ls -e "$BIN_PATH/proxy 7655" &

read -n 1 -p "Press key when proxy has started."

xterm -ls -e "$BIN_PATH/anyReceiver X B 127.0.0.1 7655 20" &
sleep 5

xterm -ls -e "$BIN_PATH/binSender A 127.0.0.1 7655" &

$BIN_PATH/binSender B 127.0.0.1 7655
