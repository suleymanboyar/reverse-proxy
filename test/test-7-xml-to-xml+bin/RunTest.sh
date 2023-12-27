# Testfile generator: ./makeTestfiles A X XYZ 12

#!/bin/bash

BIN_PATH="../.."
xterm -ls -e "$BIN_PATH/proxy 7653" &

read -n 1 -p "Press key when proxy has started."

xterm -ls -e "$BIN_PATH/anyReceiver X X 127.0.0.1 7653 30" &
sleep 1
xterm -ls -e "$BIN_PATH/anyReceiver Y B 127.0.0.1 7653 30" &
sleep 1
xterm -ls -e "$BIN_PATH/anyReceiver Z B 127.0.0.1 7653 30" &
sleep 5

$BIN_PATH/xmlSender A 127.0.0.1 7653
