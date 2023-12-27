# test file creation ./makeTestfiles A X X 12149
# test file creation ./makeTestfiles B B X 12150

#
# Note: because of the zero sleep values, reordering is normal
#

#!/bin/bash

BIN_PATH="../.."
xterm -ls -e "$BIN_PATH/proxy 7655" &

read -n 1 -p "Press key when proxy has started."

xterm -ls -e "$BIN_PATH/anyReceiver X X 127.0.0.1 7655 20" &
sleep 5

xterm -ls -e "$BIN_PATH/xmlSender A 127.0.0.1 7655" &

$BIN_PATH/binSender B 127.0.0.1 7655
