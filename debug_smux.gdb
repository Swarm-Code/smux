# GDB script for debugging smux hanging issue
# Usage: gdb -x debug_smux.gdb ./smux

# Set breakpoints at key functions
break main
break server_start
break key_bindings_init
break event_reinit
break proc_loop
break server_loop
break plugin_init
break start_cfg
break load_cfg

# Set commands for each breakpoint
commands 1
  printf "=== REACHED: main() ===\n"
  continue
end

commands 2
  printf "=== REACHED: server_start() ===\n"
  continue
end

commands 3
  printf "=== REACHED: key_bindings_init() ===\n"
  continue
end

commands 4
  printf "=== REACHED: event_reinit() ===\n"
  continue
end

commands 5
  printf "=== REACHED: proc_loop() ===\n"
  continue
end

commands 6
  printf "=== REACHED: server_loop() ===\n"
  continue
end

commands 7
  printf "=== REACHED: plugin_init() ===\n"
  continue
end

commands 8
  printf "=== REACHED: start_cfg() ===\n"
  continue
end

commands 9
  printf "=== REACHED: load_cfg() ===\n"
  continue
end

# Run the program with arguments
run -f /dev/null new-session -d -s debug-test

# If we reach here, the program completed
printf "=== PROGRAM COMPLETED NORMALLY ===\n"
quit