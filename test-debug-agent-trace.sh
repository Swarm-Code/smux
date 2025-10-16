#!/bin/bash
# Test using debug agent's comprehensive debugging tools

echo "🎯 TESTING WITH DEBUG AGENT'S TRACING INFRASTRUCTURE"
echo "===================================================="
echo ""

echo "📋 DEBUG AGENT'S ANALYSIS:"
echo "=========================="
echo "✅ Server side: Working correctly, event loop continues"
echo "❌ Client side: Hangs after printing success message"
echo "🎯 Location: After line 84 in cmd-new-project.c"
echo "🎯 Cause: Client doesn't exit after CMD_RETURN_NORMAL"
echo ""

echo "🧪 REPRODUCING HANG WITH DEBUG TRACING:"
echo "========================================"

# Clear previous debug logs
rm -f /tmp/smux-debug-trace.log

echo "Testing hang reproduction with tracing..."
echo "Expected: 'created project debug-hang-test' then hang"

# Use timeout to reproduce hang
timeout 10s ./smux new-project -n debug-hang-test 2>&1 | head -10 &

echo ""
echo "Waiting 5 seconds for hang to occur..."
sleep 5

echo ""
echo "🔍 CHECKING DEBUG TRACE LOG:"
echo "============================"
if [ -f /tmp/smux-debug-trace.log ]; then
    echo "Debug trace log exists. Last 20 lines:"
    tail -20 /tmp/smux-debug-trace.log
else
    echo "❌ Debug trace log not found - tracing not active"
fi

echo ""
echo "🔍 CHECKING FOR HANGING PROCESSES:"
echo "=================================="
ps aux | grep "[s]mux" | head -5

echo ""
echo "🎯 DEBUG AGENT'S NEXT STEPS:"
echo "============================"
echo "1. Use GDB with comprehensive breakpoints:"
echo "   gdb -x comprehensive-debug.gdb ./smux"
echo ""
echo "2. Monitor system calls:"
echo "   strace -o /tmp/smux-strace.log ./smux new-project -n test"
echo ""
echo "3. Check debug trace for hang location:"
echo "   tail -f /tmp/smux-debug-trace.log"
echo ""

echo "🔧 DEBUG AGENT'S INFRASTRUCTURE READY:"
echo "======================================"
echo "✅ debug-trace.h: Microsecond tracing macros"
echo "✅ comprehensive-debug.gdb: 25 strategic breakpoints"
echo "✅ strace-monitor.sh: System call monitoring"
echo "✅ Enhanced server.c: Event loop tracing"
echo "✅ Enhanced format.c: Format expansion tracing"

echo ""
echo "🎯 The debug agent has provided comprehensive tools to identify"
echo "   the exact location where the client hangs after command completion!"