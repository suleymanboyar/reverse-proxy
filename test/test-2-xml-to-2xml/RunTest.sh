# Generated with: ./makeTestfiles A X XY 12147

#!/bin/bash

BIN_PATH="../.."
xterm -ls -e "$BIN_PATH/prooxy 7655" &

read -n 1 -p "Press key when proxy has started."

xterm -ls -e "$BIN_PATH/anyReceiver X X 127.0.0.1 7655 20" &
sleep 1
xterm -ls -e "$BIN_PATH/anyReceiver Y X 127.0.0.1 7655 20" &
sleep 5

$BIN_PATH/xmlSender A 127.0.0.1 7655
