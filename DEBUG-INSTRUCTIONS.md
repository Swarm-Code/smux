# SMUX Startup Hang Debugging Instructions

## Quick Start

If smux is hanging during startup after the format string changes in window-tree.c, follow these steps:

### 1. Quick Hang Detection (30 seconds)

```bash
cd /home/alejandro/Swarm/smux
./quick-debug-hang.sh
```

This will:
- Test if smux hangs on `new-session`
- If hung, attach GDB and get backtrace
- Show last system calls with strace
- Identify if hang is in format parsing code

### 2. Comprehensive Debug (5 minutes)

```bash
cd /home/alejandro/Swarm/smux
./debug-startup-hang.sh
```

This will:
- Rebuild with debug symbols
- Run strace to capture system calls
- Run GDB with breakpoints on format functions
- Test with reverted format string
- Generate detailed logs

### 3. Apply Debug Logging

```bash
cd /home/alejandro/Swarm/smux
patch -p1 < add-debug-logging.patch
make clean && CFLAGS="-g -O0" make
./smux -vvv new-session -d -s debug-test
```

Check `/tmp/smux-server-*.log` for detailed format parsing logs.

## Manual GDB Debugging

If you want to manually debug:

```bash
# Terminal 1: Start smux under GDB
gdb -x debug-format-parsing.gdb ./smux

# The script will automatically set breakpoints and run
# Watch for:
# - "PARSING 'default' keyword" messages
# - Recursion count warnings
# - Infinite loop detection
```

## Understanding the Bug

### The Change

**Before (working):**
```
"#[fg=#E6A95E,bold]SESSION #[fg=#BFBDB6]#[bold]🖥️  #{session_name}#[nobold] "
```

**After (potentially hanging):**
```
"#[fg=#E6A95E,bold]SESSION #[default]#[fg=#BFBDB6]🖥️  #{session_name} "
```

### Key Difference

- `#[nobold]` - Only clears the BOLD attribute, leaves colors intact
- `#[default]` - Resets ALL attributes (colors, bold, etc.) to base defaults

### Why This Might Hang

1. **State Reset Issue**: `#[default]` resets to base state which might be uninitialized
2. **Circular Reference**: Resetting to default might trigger re-parsing of the format
3. **Performance**: Multiple rapid style changes might have O(n²) complexity
4. **Edge Case**: The sequence `#[bold]#[default]#[color]` might hit an untested code path

## What to Look For

### In GDB Backtrace

If you see repeated calls to these functions, it's infinite recursion:
- `format_expand`
- `format_expand1`
- `style_parse`
- `format_draw`

### In Strace Output

If you see the process stuck on:
- `poll()` or `select()` → Waiting for I/O (not CPU bound)
- No syscalls → CPU-bound infinite loop
- Repeated `brk()` → Memory allocation loop

### In Debug Logs

Look for:
- Same format string being expanded multiple times
- `current_default` values changing unexpectedly
- Errors in style parsing

## Quick Fixes

### Option 1: Revert to Original (Safest)

```bash
cd /home/alejandro/Swarm/smux
# Edit window-tree.c line 65:
# Change:
"#[fg=#E6A95E,bold]SESSION #[default]#[fg=#BFBDB6]🖥️  #{session_name} "
# To:
"#[fg=#E6A95E,bold]SESSION #[fg=#BFBDB6]#[bold]🖥️  #{session_name}#[nobold] "

make clean && make
```

### Option 2: Better Fix (Cleaner)

```bash
# Edit window-tree.c line 65:
# Change:
"#[fg=#E6A95E,bold]SESSION #[default]#[fg=#BFBDB6]🖥️  #{session_name} "
# To:
"#[fg=#E6A95E,bold]SESSION #[nobold]#[fg=#BFBDB6]🖥️  #{session_name} "

make clean && make
```

This achieves the same visual result (removing redundant `#[bold]`) without using `#[default]`.

### Option 3: Root Cause Fix

If you identify the actual bug:

1. Add null checks in `style_parse()` for base parameter
2. Add recursion depth limit in `format_expand()`
3. Optimize style state management in `format_draw()`
4. Add validation for `#[default]` in format strings

## Verification

After applying a fix:

```bash
# Test basic functionality
./smux new-session -d -s test1 && echo "✓ new-session works"
./smux new-project -d testproj && echo "✓ new-project works"
./smux choose-tree -s && echo "✓ choose-tree works"

# Test format rendering
./smux list-sessions -F "#{session_name}"
./smux list-projects

# Kill test sessions
./smux kill-session -t test1
./smux kill-session -t testproj-0
```

## Log Files

After running debug scripts, check these files:

- `/tmp/smux-startup.log` - Verbose startup log
- `/tmp/smux-strace.log` - System call trace
- `/tmp/gdb-output.log` - GDB debugging output
- `/tmp/smux-format-debug.log` - Format parsing details
- `/tmp/hang-backtrace.log` - Backtrace if hung

## Reporting the Bug

If you confirm the hang, report with:

1. **Exact command that hangs**: e.g., `smux new-session -d -s test`
2. **Time to hang**: How long before it gets stuck
3. **Backtrace**: From `/tmp/hang-backtrace.log`
4. **Last syscalls**: From `/tmp/hang-strace.log`
5. **Environment**: Number of existing sessions/projects
6. **Terminal type**: $TERM variable

## Advanced Debugging

### Add Printf Debugging

```c
// In format-draw.c, line 826:
fprintf(stderr, "DEBUG: Parsing style: %s\n", tmp);
fflush(stderr);

// In style.c, line 87:
fprintf(stderr, "DEBUG: Reset to default, base->fg=%d\n", base->fg);
fflush(stderr);
```

### Use Valgrind

```bash
valgrind --leak-check=full --track-origins=yes --log-file=/tmp/valgrind.log \
    ./smux new-session -d -s valgrind-test
```

### Use perf

```bash
perf record -g ./smux new-session -d -s perf-test
perf report
```

## Contact

If you need help:
1. Share log files from `/tmp/`
2. Include output of `./quick-debug-hang.sh`
3. Describe your environment (sessions/projects count)
