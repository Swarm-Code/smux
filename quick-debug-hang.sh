#!/bin/bash
# Quick debug script to identify smux startup hang

set -e

SMUX_DIR="/home/alejandro/Swarm/smux"
cd "$SMUX_DIR"

echo "=== QUICK DEBUG: SMUX STARTUP HANG ==="
echo ""

# Kill any existing smux servers
killall -9 smux 2>/dev/null || true
rm -rf /tmp/smux-* 2>/dev/null || true

echo "[1/5] Testing if smux hangs..."
timeout 3 ./smux new-session -d -s quicktest 2>&1 &
PID=$!
sleep 4

if kill -0 $PID 2>/dev/null; then
    echo "✗ HANG DETECTED - smux is still running after timeout"
    echo ""
    echo "[2/5] Getting process state..."
    ps aux | grep $PID | grep -v grep || true

    echo ""
    echo "[3/5] Attaching GDB to get backtrace..."
    cat > /tmp/quick-gdb.txt << EOF
thread apply all backtrace
quit
EOF

    sudo -S gdb -batch -x /tmp/quick-gdb.txt -p $PID <<< "Luis2901" 2>&1 | tee /tmp/hang-backtrace.log

    echo ""
    echo "[4/5] Checking system calls with strace..."
    timeout 2 sudo -S strace -p $PID <<< "Luis2901" 2>&1 | head -50 | tee /tmp/hang-strace.log

    # Kill the hung process
    kill -9 $PID 2>/dev/null || true

    echo ""
    echo "[5/5] Analyzing backtrace..."
    if grep -q "format_expand\|style_parse" /tmp/hang-backtrace.log; then
        echo "✓ FOUND: Hang is in format parsing code"
        echo ""
        echo "Stack trace showing format parsing:"
        grep -B2 -A2 "format_expand\|style_parse" /tmp/hang-backtrace.log
    elif grep -q "window_tree" /tmp/hang-backtrace.log; then
        echo "✓ FOUND: Hang is in window tree code"
        echo ""
        echo "Stack trace showing window tree:"
        grep -B2 -A2 "window_tree" /tmp/hang-backtrace.log
    else
        echo "? Location unclear - full backtrace:"
        cat /tmp/hang-backtrace.log
    fi

    echo ""
    echo "=== HANG ANALYSIS COMPLETE ==="
    echo "Log files created:"
    echo "  - /tmp/hang-backtrace.log (GDB backtrace)"
    echo "  - /tmp/hang-strace.log (system calls)"

else
    echo "✓ No hang detected - smux started successfully"
    ./smux kill-session -t quicktest 2>/dev/null || true
fi

echo ""
echo "=== DONE ==="
