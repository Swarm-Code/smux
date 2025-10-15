# GDB script to debug format string parsing in smux
# Specifically targeting the #[default] vs #[nobold] issue

set pagination off
set logging file /tmp/smux-format-debug.log
set logging overwrite on
set logging on

# Print detailed info
echo === Starting smux format parsing debug ===\n

# Break on format expansion
break format_expand
commands
  silent
  printf "format_expand() called with ft=%p, fmt=%s\n", $arg0, $arg1
  continue
end

# Break on format_expand1 (internal expansion)
break format_expand1
commands
  silent
  printf "format_expand1() called\n"
  backtrace 3
  continue
end

# Break on style_parse - this is where #[default] is processed
break style_parse
commands
  printf "\n=== style_parse() called ===\n"
  printf "Input string: %s\n", $arg2
  backtrace 5
  continue
end

# Break specifically on the "default" keyword parsing in style.c
break style.c:87
commands
  printf "\n!!! PARSING 'default' keyword !!!\n"
  printf "tmp = %s\n", tmp
  backtrace 5
  # Print the style structure before modification
  printf "Before: fg=%d, bg=%d, attr=%d\n", sy->gc.fg, sy->gc.bg, sy->gc.attr
  step
  printf "After: fg=%d, bg=%d, attr=%d\n", sy->gc.fg, sy->gc.bg, sy->gc.attr
  continue
end

# Break on window_tree_build_project where format is expanded
break window_tree_build_project
commands
  silent
  printf "\n=== window_tree_build_project() called for project: %s ===\n", $arg0->name
  continue
end

# Break on window_tree_build_session where format is expanded
break window_tree_build_session
commands
  silent
  printf "\n=== window_tree_build_session() called for session: %s ===\n", $arg0->name
  continue
end

# Break on the specific line where session format is expanded
break window-tree.c:547
commands
  printf "\n=== EXPANDING SESSION FORMAT STRING ===\n"
  printf "Session: %s\n", s->name
  printf "Format: %s\n", data->format
  # Step through format_expand
  step
  step
  continue
end

# Set a watchpoint to detect infinite loops (recursion depth)
# We'll set a counter to detect if we're stuck
set $recursion_count = 0

# Break on function entry/exit to count recursion
break format_expand1
commands
  silent
  set $recursion_count = $recursion_count + 1
  if $recursion_count > 100
    printf "\n!!! POSSIBLE INFINITE RECURSION DETECTED !!!\n"
    printf "Recursion count: %d\n", $recursion_count
    backtrace 20
    quit
  end
  continue
end

# Catch segfaults
catch signal SIGSEGV
commands
  printf "\n!!! SEGMENTATION FAULT !!!\n"
  backtrace
  info registers
  quit
end

# Catch infinite loops via alarm
handle SIGALRM print nostop pass

echo Setting up timeout...\n
# Start the program
run new-session -d -s debug-test

# If we get here, check what happened
echo \n=== PROGRAM COMPLETED ===\n
echo Checking if session was created...\n

# Print final state
info threads
thread apply all backtrace

quit
