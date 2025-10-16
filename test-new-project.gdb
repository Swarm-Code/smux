# GDB script to debug new-project hang
set pagination off
set logging file /home/alejandro/Swarm/smux/gdb-new-project.log
set logging overwrite on
set logging on

# Break on format_expand since that processes format strings
break format_expand
commands
  silent
  printf "=== format_expand called ===\n"
  printf "Format string (first 100 chars): %.100s\n", $arg2
  continue
end

# Break on potential infinite loops in window-tree.c
break window_tree.c:603
commands
  silent
  if i > 50
    printf "WARNING: Possible infinite loop at window_tree.c:603, i=%d\n", i
    backtrace
    quit
  end
  continue
end

break window_tree.c:628
commands
  silent
  if i > 50
    printf "WARNING: Possible infinite loop at window_tree.c:628, i=%d\n", i
    backtrace
    quit
  end
  continue
end

# Set timeout
set $_start_time = $_time_now
define check_timeout
  if $_time_now - $_start_time > 10
    printf "\n=== TIMEOUT AFTER 10 SECONDS ===\n"
    backtrace
    info threads
    quit
  end
end

# Run the command
run new-project -n test-proj

# Should not reach here unless it completes
printf "\n=== Command completed successfully ===\n"
quit
