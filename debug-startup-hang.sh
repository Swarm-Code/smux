#!/bin/bash
# Debug script for smux startup hang issue
# This script helps identify where smux hangs during startup after format string changes

set -e

SMUX_DIR="/home/alejandro/Swarm/smux"
cd "$SMUX_DIR"

echo "=== SMUX STARTUP HANG DEBUGGING SCRIPT ==="
echo "Investigating hang after format string changes in window-tree.c"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_step() {
    echo -e "${BLUE}==>${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Step 1: Clean rebuild with debug symbols
print_step "Step 1: Rebuilding smux with debug symbols (-g -O0)"
make clean
CFLAGS="-g -O0 -DDEBUG" make -j$(nproc)
print_success "Build complete with debug symbols"
echo ""

# Step 2: Create a minimal test case
print_step "Step 2: Creating minimal test case"
cat > /tmp/test-smux-hang.sh << 'EOF'
#!/bin/bash
# Minimal test to reproduce the hang

echo "Testing smux startup..."
timeout 5 /home/alejandro/Swarm/smux/smux -vvv new-session -d -s test-hang 2>&1 | tee /tmp/smux-startup.log

if [ $? -eq 124 ]; then
    echo "HANG DETECTED: smux timed out after 5 seconds"
    exit 1
else
    echo "SUCCESS: smux started without hanging"
    /home/alejandro/Swarm/smux/smux kill-session -t test-hang 2>/dev/null
    exit 0
fi
EOF
chmod +x /tmp/test-smux-hang.sh

print_success "Test case created at /tmp/test-smux-hang.sh"
echo ""

# Step 3: Test current version
print_step "Step 3: Testing if current version hangs"
if /tmp/test-smux-hang.sh; then
    print_success "Current version does NOT hang"
else
    print_error "Current version HANGS - this is the bug we're debugging"
fi
echo ""

# Step 4: Run with strace to identify hang type
print_step "Step 4: Running with strace to identify hang location"
print_warning "This will generate a lot of output..."
timeout 5 strace -o /tmp/smux-strace.log -f -tt -T ./smux new-session -d -s strace-test 2>&1 &
STRACE_PID=$!
sleep 6
kill $STRACE_PID 2>/dev/null || true

if [ -f /tmp/smux-strace.log ]; then
    print_success "Strace log saved to /tmp/smux-strace.log"
    print_warning "Last 20 system calls before hang:"
    tail -20 /tmp/smux-strace.log
else
    print_error "Strace log not generated"
fi
echo ""

# Step 5: Create GDB debug script
print_step "Step 5: Creating GDB debugging script"
cat > /tmp/debug-smux.gdb << 'EOF'
# GDB script to debug smux startup hang

# Set logging
set logging file /tmp/smux-gdb.log
set logging on

# Set breakpoints on format parsing functions
break format_expand
break format_expand1
break style_parse
break window_tree_build
break window_tree_build_session
break window_tree_build_project

# Breakpoint on the specific format string usage
break window_tree.c:512 if text != 0

# Commands to print state at each breakpoint
commands
  echo \n=== BREAKPOINT HIT ===\n
  backtrace 5
  info locals
  continue
end

# Run with arguments
run new-session -d -s gdb-test

# If we hit a hang, print backtrace
thread apply all backtrace

quit
EOF

print_success "GDB script created at /tmp/debug-smux.gdb"
echo ""

# Step 6: Run under GDB
print_step "Step 6: Running smux under GDB"
print_warning "This may take a minute..."
timeout 30 gdb -batch -x /tmp/debug-smux.gdb ./smux 2>&1 | tee /tmp/gdb-output.log
print_success "GDB output saved to /tmp/gdb-output.log"
echo ""

# Step 7: Analyze GDB output for infinite loops
print_step "Step 7: Analyzing for infinite loops or recursion"
if grep -q "format_expand.*format_expand" /tmp/gdb-output.log; then
    print_error "POSSIBLE INFINITE RECURSION detected in format_expand"
    echo "Stack trace showing recursion:"
    grep -A 10 "format_expand.*format_expand" /tmp/gdb-output.log | head -20
fi
echo ""

# Step 8: Test with format string reverted
print_step "Step 8: Testing with format string change reverted"
print_warning "Reverting the #[default] change..."

# Backup current version
cp window-tree.c window-tree.c.debug-backup

# Revert the specific line
sed -i 's/#\[fg=#E6A95E,bold\]SESSION #\[default\]#\[fg=#BFBDB6\]🖥️  #{session_name}/#[fg=#E6A95E,bold]SESSION #[fg=#BFBDB6]#[bold]🖥️  #{session_name}#[nobold]/' window-tree.c

# Rebuild
print_warning "Rebuilding with reverted change..."
make -j$(nproc)

print_warning "Testing reverted version..."
if /tmp/test-smux-hang.sh; then
    print_success "REVERTED version does NOT hang - this confirms the bug is in the format string change"
else
    print_error "REVERTED version STILL hangs - the bug may be elsewhere"
fi

# Restore original
mv window-tree.c.debug-backup window-tree.c
make -j$(nproc)

echo ""
echo "=== DEBUGGING COMPLETE ==="
echo ""
echo "Generated files:"
echo "  - /tmp/test-smux-hang.sh      - Minimal reproduction test"
echo "  - /tmp/smux-startup.log       - Verbose startup log"
echo "  - /tmp/smux-strace.log        - System call trace"
echo "  - /tmp/debug-smux.gdb         - GDB debugging script"
echo "  - /tmp/gdb-output.log         - GDB execution output"
echo ""
echo "Next steps:"
echo "  1. Review /tmp/smux-strace.log for last system calls before hang"
echo "  2. Review /tmp/gdb-output.log for breakpoint hits and stack traces"
echo "  3. Look for infinite loops in format_expand or style_parse"
echo "  4. Check if #[default] parsing differs from #[nobold]"
echo ""
