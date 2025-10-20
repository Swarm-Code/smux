#!/bin/bash

# Live Debug Session Launcher for Smux Scroll Issue
# Run this script, then join the session and trigger the jitter

set -e

cd /home/alejandro/Swarm/smux

echo "🚀 SMUX SCROLL DEBUG SESSION LAUNCHER"
echo "======================================"
echo ""

# Clean up any existing debug session
if ./smux list-sessions 2>/dev/null | grep -q "debug-scroll"; then
    echo "Cleaning up existing debug session..."
    ./smux kill-session -t debug-scroll 2>/dev/null || true
    sleep 1
fi

# Create the debug session with specific sizes
echo "📺 Creating smux session..."
./smux new-session -d -s "debug-scroll" -x 220 -y 60 -c /home/alejandro/Swarm/smux

# Split windows for monitoring
./smux split-window -t "debug-scroll" -h -l 100

# In left pane: show system calls related to output
./smux send-keys -t "debug-scroll:0.0" "echo 'LEFT PANE: Ready for Claude Code output' && bash" Enter

# In right pane: show the debug log in real-time
./smux send-keys -t "debug-scroll:0.1" "clear && echo 'RIGHT PANE: Debug Events (will appear here)' && tail -f /tmp/smux_scroll_debug.log 2>/dev/null || echo 'Waiting for debug output...'" Enter

sleep 1

# Now display the session info
echo ""
echo "✅ Session created: 'debug-scroll'"
echo ""
echo "📋 NEXT STEPS:"
echo "1. Open another terminal and run:"
echo "   cd /home/alejandro/Swarm/smux && ./smux attach -t debug-scroll"
echo ""
echo "2. In the LEFT PANE of smux, run Claude Code or paste rapid output"
echo ""
echo "3. Watch the RIGHT PANE for scroll events"
echo ""
echo "4. To trigger jitter:"
echo "   - Output text rapidly (you can use: yes | head -100)"
echo "   - Or run: seq 1 1000 | while read i; do echo \"Line $i with some text\"; sleep 0.01; done"
echo ""
echo "5. When you see flickering, check the debug log:"
echo "   tail -100 /tmp/smux_scroll_debug.log"
echo ""
echo "🔍 ANALYSIS POINTS TO WATCH:"
echo "   - How frequently do [SCROLL] events occur?"
echo "   - Are [SYNC_START] and [SYNC_END] markers present?"
echo "   - Do [REDRAW] events correlate with scrolls?"
echo "   - What's the TTY buffer size when jitter happens?"
echo ""

# Show current session
echo "Current sessions:"
./smux list-sessions

