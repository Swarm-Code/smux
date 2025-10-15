#!/bin/bash
# Test if new-project command hangs

set -e

SMUX_DIR="/home/alejandro/Swarm/smux"
cd "$SMUX_DIR"

echo "=== TESTING PROJECT CREATION FOR HANG ==="
echo ""

# Kill any existing smux servers
killall -9 smux 2>/dev/null || true
rm -rf /tmp/smux-* 2>/dev/null || true

# Test 1: new-session (baseline)
echo "[TEST 1] Testing new-session (should work)..."
timeout 3 ./smux new-session -d -s test1 2>&1 && echo "✓ new-session OK" || echo "✗ new-session HUNG"
./smux kill-session -t test1 2>/dev/null || true
echo ""

# Test 2: new-project (likely to hang)
echo "[TEST 2] Testing new-project (may hang)..."
timeout 3 ./smux new-project -d testproject 2>&1 &
PID=$!
sleep 4

if kill -0 $PID 2>/dev/null; then
    echo "✗ HANG DETECTED in new-project"
    echo ""
    echo "Getting backtrace..."

    cat > /tmp/quick-gdb.txt << EOF
thread apply all backtrace
quit
EOF

    sudo -S gdb -batch -x /tmp/quick-gdb.txt -p $PID <<< "Luis2901" 2>&1 | tee /tmp/project-hang-backtrace.log

    kill -9 $PID 2>/dev/null || true

    echo ""
    echo "Backtrace saved to /tmp/project-hang-backtrace.log"
    echo ""
    echo "Key functions in backtrace:"
    grep "#[0-9]" /tmp/project-hang-backtrace.log | grep -E "format_|style_|window_tree|project" | head -20
else
    echo "✓ new-project OK"
    ./smux kill-session -t testproject-0 2>/dev/null || true
fi

echo ""

# Test 3: choose-tree (triggers window-tree.c formatting)
echo "[TEST 3] Testing choose-tree (triggers window-tree formatting)..."
# First create a session to display
./smux new-session -d -s bgtest
timeout 3 ./smux choose-tree -f "#{session_name}" 2>&1 &
PID=$!
sleep 4

if kill -0 $PID 2>/dev/null; then
    echo "✗ HANG DETECTED in choose-tree"

    cat > /tmp/quick-gdb.txt << EOF
thread apply all backtrace
quit
EOF

    sudo -S gdb -batch -x /tmp/quick-gdb.txt -p $PID <<< "Luis2901" 2>&1 | tee /tmp/choosetree-hang-backtrace.log

    kill -9 $PID 2>/dev/null || true

    echo ""
    echo "Backtrace saved to /tmp/choosetree-hang-backtrace.log"
else
    echo "✓ choose-tree OK"
fi

./smux kill-server 2>/dev/null || true

echo ""
echo "=== TEST COMPLETE ==="
