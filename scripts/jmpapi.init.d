#!/bin/bash
#
#   JmpAPI Web server
#
#  Add to system with:
#    sudo update-rc.d jmpapi defaults 90

### BEGIN INIT INFO
# Provides: jmpapi
# Required-Start:
# Required-Stop:
# Should-Start:
# Should-Stop:
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: Start and stop JmpAPI Web server
# Description: JmpAPI Web server
### END INIT INFO

NAME=jmpapi
EXEC=/usr/bin/$NAME
USER=$NAME
GROUP=$NAME
RUN=/var/run/$NAME
PID_FILE=$RUN/$NAME.pid
CONFIG=/etc/$NAME/jmpapi.yaml
LOCAL=/etc/$NAME/local.yaml
LOG=/var/log/$NAME/log.txt

START_STOP_OPTS="-x $EXEC -n $NAME -p $PID_FILE -d $RUN"


start() {
    mkdir -p $(dirname $PID_FILE)

    ARGS="--fork --set-group $GROUP --run-as $USER $CONFIG"
    ARGS+=" --log $LOG --log-rotate --log-rotate-dir=/var/log/$NAME"
    ARGS+=" --pid-file=$PID_FILE"

    if [ -f "$LOCAL" ]; then ARGS+=" $LOCAL"; fi

    start-stop-daemon --start $START_STOP_OPTS -- $ARGS
}


stop() {
    start-stop-daemon --stop --retry forever/-TERM/10 $START_STOP_OPTS
}


status() {
    start-stop-daemon --status $START_STOP_OPTS
}


################################################################################
# Main switch statement
#
case "$1" in
    start) start ;;
    stop) stop ;;
    restart) stop; start ;;
    status) status ;;

    *)
      echo "Syntax: $0 <start | stop | restart | status>"
      exit 1
    ;;
esac
