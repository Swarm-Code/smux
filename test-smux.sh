#!/bin/bash
echo "Testing smux..."
cd /home/alejandro/Swarm/smux
./smux new-session -d -s test-session
if [ $? -eq 0 ]; then
    echo "✅ SUCCESS: smux session created!"
    ./smux list-sessions
    ./smux kill-session -t test-session
    echo "✅ Test session cleaned up"
else
    echo "❌ FAILED: smux still has issues"
fi
