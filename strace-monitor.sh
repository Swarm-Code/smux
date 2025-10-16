#!/bin/bash
# Comprehensive strace monitoring for smux hang investigation

STRACE_LOG="strace-comprehensive.log"
TIMEOUT=30

echo "===== STRACE MONITORING FOR SMUX HANG INVESTIGATION ====="
echo "Log file: $STRACE_LOG"
echo "Timeout: ${TIMEOUT}s"
echo ""

# Kill any existing smux instances
pkill -9 smux 2>/dev/null
sleep 1

# Run smux with comprehensive strace
echo "Starting smux with strace monitoring..."
timeout ${TIMEOUT}s strace -f -tt -T -s 200 -o "$STRACE_LOG" \
    -e trace=open,openat,close,read,write,poll,epoll_wait,select,pselect6,socket,connect,accept,fork,clone,execve,wait4,futex,nanosleep,clock_nanosleep \
    ./smux new -s test-strace 2>&1 &

STRACE_PID=$!

echo "strace PID: $STRACE_PID"
echo "Monitoring for ${TIMEOUT} seconds..."

# Monitor in real-time
sleep 2

# Check if it's still running
if ps -p $STRACE_PID > /dev/null; then
    echo "Process still running after 2 seconds..."
    echo "Checking for blocking operations..."

    # Wait a bit more
    sleep 3

    if ps -p $STRACE_PID > /dev/null; then
        echo "Process still running after 5 seconds - likely hanging"
        echo "Analyzing strace output for last operations..."

        # Show last 50 lines
        tail -50 "$STRACE_LOG"

        echo ""
        echo "Looking for repeating patterns (potential infinite loops)..."
        tail -100 "$STRACE_LOG" | sort | uniq -c | sort -rn | head -20

        echo ""
        echo "Checking for blocking system calls..."
        grep -E "(poll|epoll_wait|select|pselect6|futex|nanosleep)" "$STRACE_LOG" | tail -20

        # Wait for timeout
        wait $STRACE_PID
    fi
else
    echo "Process completed quickly"
fi

echo ""
echo "===== ANALYSIS ====="
echo ""

# Count system calls
echo "System call frequency:"
grep -o '^[0-9:]* [a-z_]*(' "$STRACE_LOG" | sed 's/.*\([a-z_]*\)(/\1/' | sort | uniq -c | sort -rn | head -20

echo ""
echo "Last 20 system calls:"
tail -20 "$STRACE_LOG"

echo ""
echo "Files opened:"
grep -E 'open(at)?\(' "$STRACE_LOG" | grep -v ENOENT

echo ""
echo "Blocking operations:"
grep -E '(poll|epoll_wait|select|futex).*<unfinished' "$STRACE_LOG"

echo ""
echo "Full strace log saved to: $STRACE_LOG"
echo "===== END ANALYSIS ====="
