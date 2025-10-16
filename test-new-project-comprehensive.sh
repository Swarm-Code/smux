#!/bin/bash
# Comprehensive test for smux new project hang investigation

echo "===== COMPREHENSIVE SMUX NEW PROJECT TEST ====="
echo ""

# Clean slate
pkill -9 smux 2>/dev/null
sleep 1
rm -f /tmp/smux-debug-trace.log
rm -f /tmp/smux-1000/default*
rm -rf ~/.smux

echo "Step 1: Start smux server in background..."
timeout 5s /home/alejandro/Swarm/smux/smux start-server &
SERVER_PID=$!
sleep 2

if ps -p $SERVER_PID > /dev/null 2>&1; then
    echo "  Server running (PID: $SERVER_PID)"
else
    echo "  Server already exited or failed to start"
fi

echo ""
echo "Step 2: Check server is listening..."
if [ -e /tmp/smux-1000/default ]; then
    echo "  Socket exists: /tmp/smux-1000/default"
    ls -la /tmp/smux-1000/default
else
    echo "  ERROR: Socket NOT created!"
    echo "  Debug log:"
    tail -20 /tmp/smux-debug-trace.log
    exit 1
fi

echo ""
echo "Step 3: List current projects (should be empty)..."
timeout 5s /home/alejandro/Swarm/smux/smux list-projects 2>&1 || echo "  Command timed out or failed"

echo ""
echo "Step 4: Attempt to create new project with timeout and monitoring..."
echo "  Command: smux new-project test-project"
rm -f /tmp/smux-debug-trace.log

# Run in background to monitor
(
    timeout 10s /home/alejandro/Swarm/smux/smux new-project test-project 2>&1 | tee /tmp/new-project-output.log
) &
CMD_PID=$!

# Monitor progress
for i in {1..10}; do
    sleep 1
    echo "  [${i}s] Process status:"
    if ps -p $CMD_PID > /dev/null 2>&1; then
        echo "    - Command still running (PID: $CMD_PID)"
        echo "    - Last 5 debug log entries:"
        tail -5 /tmp/smux-debug-trace.log 2>/dev/null | sed 's/^/      /'
    else
        echo "    - Command completed/exited"
        break
    fi
done

wait $CMD_PID 2>/dev/null
CMD_EXIT=$?

echo ""
echo "Step 5: Results"
echo "  Exit code: $CMD_EXIT"
if [ -f /tmp/new-project-output.log ]; then
    echo "  Output:"
    cat /tmp/new-project-output.log | sed 's/^/    /'
fi

echo ""
echo "Step 6: Final debug log (last 100 lines):"
tail -100 /tmp/smux-debug-trace.log 2>/dev/null

echo ""
echo "Step 7: Check if project was created..."
timeout 5s /home/alejandro/Swarm/smux/smux list-projects 2>&1

echo ""
echo "===== END TEST ====="

# Cleanup
pkill -9 smux 2>/dev/null
