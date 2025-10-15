# GDB script to debug smux startup hang
# Focus on format string processing in window-tree.c

# Set breakpoints on critical functions
break main
break window_tree_init
break window_tree_build
break window_tree_build_project
break window_tree_build_session
break format_expand
break format_parse
break style_parse

# Set watchpoints for potential infinite loops
break window_tree.c:603 if i > 100
break window_tree.c:628 if i > 100

# Log function calls
commands 1
  printf "=== MAIN STARTED ===\n"
  continue
end

commands 2
  printf "=== window_tree_init called ===\n"
  backtrace 3
  continue
end

commands 3
  printf "=== window_tree_build called ===\n"
  backtrace 3
  continue
end

commands 4
  printf "=== window_tree_build_project called ===\n"
  printf "  Project: %s (id=%d)\n", p->name, p->id
  continue
end

commands 5
  printf "=== window_tree_build_session called ===\n"
  if s
    printf "  Session: %s (id=%d)\n", s->name, s->id
  end
  continue
end

commands 6
  printf "=== format_expand called ===\n"
  printf "  Format string (first 80 chars): %.80s\n", $arg2
  continue
end

# Set timeout handling
set pagination off
set logging file /home/alejandro/Swarm/smux/gdb-startup.log
set logging overwrite on
set logging on

# Run with timeout detection
run -v

# If we get here, print where we are
printf "\n=== EXECUTION STOPPED ===\n"
backtrace
info threads
quit
