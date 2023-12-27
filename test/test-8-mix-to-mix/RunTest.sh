# Testfile generator: ./makeTestfiles A X WXY 12018
# Testfile generator: ./makeTestfiles B B XYZ 17012

#!/bin/bash

BIN_PATH="../.."
xterm -ls -e "$BIN_PATH/proxy 7653" &

read -n 1 -p "Press key when proxy has started."

xterm -ls -e "$BIN_PATH/anyReceiver W X 127.0.0.1 7653 30" &
sleep 1
xterm -ls -e "$BIN_PATH/anyReceiver X X 127.0.0.1 7653 30" &
sleep 1
xterm -ls -e "$BIN_PATH/anyReceiver Y B 127.0.0.1 7653 30" &
sleep 1
xterm -ls -e "$BIN_PATH/anyReceiver Z B 127.0.0.1 7653 30" &
sleep 5

xterm -ls -e "$BIN_PATH/binSender B 127.0.0.1 7653" &

$BIN_PATH/xmlSender A 127.0.0.1 7653
