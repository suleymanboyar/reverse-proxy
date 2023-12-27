# Testfile generator: ./makeTestfiles A B X 11133

#!/bin/bash
BIN_PATH="../.."

xterm -ls -e "$BIN_PATH/proxy 7654" &

read -n 1 -p "Press key when proxy has started."

xterm -ls -e "$BIN_PATH/anyReceiver X B 127.0.0.1 7654 30" &
sleep 5

$BIN_PATH/binSender A 127.0.0.1 7654
