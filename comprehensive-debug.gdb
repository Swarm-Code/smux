# Comprehensive GDB debugging script for smux hang investigation
# Usage: gdb -x comprehensive-debug.gdb ./smux

set pagination off
set print pretty on
set print array on
set logging file gdb-comprehensive-trace.log
set logging on

# Enable all debug output
set debug infrun 0
set verbose on

# Catch all signals
handle SIGPIPE nostop noprint pass
handle SIGTERM nostop print pass
handle SIGINT stop print nopass

# Breakpoint 1: Main entry point
break main
commands 1
  printf "\n===== BREAKPOINT 1: main() =====\n"
  printf "Arguments: argc=%d\n", argc
  if argc > 0
    printf "argv[0]=%s\n", argv[0]
  end
  backtrace 5
  continue
end

# Breakpoint 2: Server start
break server_start
commands 2
  printf "\n===== BREAKPOINT 2: server_start() =====\n"
  printf "Flags: 0x%lx\n", flags
  backtrace 5
  continue
end

# Breakpoint 3: Server loop entry
break server_loop
commands 3
  printf "\n===== BREAKPOINT 3: server_loop() =====\n"
  printf "Time: %ld\n", current_time
  backtrace 5
  continue
end

# Breakpoint 4: Event base dispatch (libevent)
break event_base_loop
commands 4
  printf "\n===== BREAKPOINT 4: event_base_loop() =====\n"
  backtrace 5
  continue
end

# Breakpoint 5: Format expansion
break format_expand
commands 5
  printf "\n===== BREAKPOINT 5: format_expand() =====\n"
  if ft != 0
    printf "Format tree: %p\n", ft
  end
  backtrace 5
  continue
end

# Breakpoint 6: Format parse
break format_parse
commands 6
  printf "\n===== BREAKPOINT 6: format_parse() =====\n"
  backtrace 5
  continue
end

# Breakpoint 7: Style parse
break style_parse
commands 7
  printf "\n===== BREAKPOINT 7: style_parse() =====\n"
  if style != 0
    printf "Style: %p\n", style
  end
  if string != 0
    printf "String: %s\n", string
  end
  backtrace 5
  continue
end

# Breakpoint 8: Style apply
break style_apply
commands 8
  printf "\n===== BREAKPOINT 8: style_apply() =====\n"
  backtrace 5
  continue
end

# Breakpoint 9: Project create
break project_create
commands 9
  printf "\n===== BREAKPOINT 9: project_create() =====\n"
  if name != 0
    printf "Project name: %s\n", name
  end
  backtrace 5
  continue
end

# Breakpoint 10: Session create
break session_create
commands 10
  printf "\n===== BREAKPOINT 10: session_create() =====\n"
  if name != 0
    printf "Session name: %s\n", name
  end
  backtrace 5
  continue
end

# Breakpoint 11: Command queue processing
break cmdq_next
commands 11
  printf "\n===== BREAKPOINT 11: cmdq_next() =====\n"
  backtrace 5
  continue
end

# Breakpoint 12: Server accept
break server_accept
commands 12
  printf "\n===== BREAKPOINT 12: server_accept() =====\n"
  printf "FD: %d\n", fd
  backtrace 5
  continue
end

# Breakpoint 13: Window tree build (potential hang location)
break window_tree_build
commands 13
  printf "\n===== BREAKPOINT 13: window_tree_build() =====\n"
  backtrace 5
  continue
end

# Breakpoint 14: Choose tree mode (potential hang location)
break window_choose_init
commands 14
  printf "\n===== BREAKPOINT 14: window_choose_init() =====\n"
  backtrace 5
  continue
end

# Breakpoint 15: Screen write (potential blocking)
break screen_write_start
commands 15
  printf "\n===== BREAKPOINT 15: screen_write_start() =====\n"
  backtrace 5
  continue
end

# Breakpoint 16: TTY write (potential blocking I/O)
break tty_write
commands 16
  printf "\n===== BREAKPOINT 16: tty_write() =====\n"
  backtrace 5
  continue
end

# Breakpoint 17: Job run (potential hanging subprocess)
break job_run
commands 17
  printf "\n===== BREAKPOINT 17: job_run() =====\n"
  if cmd != 0
    printf "Command: %s\n", cmd
  end
  backtrace 5
  continue
end

# Breakpoint 18: Proc loop (main event loop)
break proc_loop
commands 18
  printf "\n===== BREAKPOINT 18: proc_loop() =====\n"
  backtrace 5
  continue
end

# Breakpoint 19: Server client loop
break server_client_loop
commands 19
  printf "\n===== BREAKPOINT 19: server_client_loop() =====\n"
  backtrace 5
  continue
end

# Breakpoint 20: Format jobs (potential async hangs)
break format_job_run
commands 20
  printf "\n===== BREAKPOINT 20: format_job_run() =====\n"
  backtrace 5
  continue
end

# Breakpoint 21: UTF-8 processing (emoji-related)
break utf8_open
commands 21
  printf "\n===== BREAKPOINT 21: utf8_open() =====\n"
  printf "Character: U+%04X\n", u
  backtrace 5
  continue
end

# Breakpoint 22: UTF-8 width calculation
break utf8_width
commands 22
  printf "\n===== BREAKPOINT 22: utf8_width() =====\n"
  backtrace 5
  continue
end

# Breakpoint 23: Plugin init (potential hang)
break plugin_init
commands 23
  printf "\n===== BREAKPOINT 23: plugin_init() =====\n"
  backtrace 5
  continue
end

# Breakpoint 24: Key bindings init
break key_bindings_init
commands 24
  printf "\n===== BREAKPOINT 24: key_bindings_init() =====\n"
  backtrace 5
  continue
end

# Breakpoint 25: Options init
break options_init
commands 25
  printf "\n===== BREAKPOINT 25: options_init() =====\n"
  backtrace 5
  continue
end

# Watchpoint for infinite loop detection
# We'll set this after breaking to check iteration counts

printf "\n========================================\n"
printf "Comprehensive GDB Debug Script Loaded\n"
printf "========================================\n"
printf "Total breakpoints: 25\n"
printf "Coverage:\n"
printf "  - Main entry and server initialization\n"
printf "  - Event loop and processing\n"
printf "  - Format and style processing\n"
printf "  - Project/session management\n"
printf "  - I/O operations\n"
printf "  - UTF-8 and emoji handling\n"
printf "  - Job and subprocess management\n"
printf "\n"
printf "Type 'run' to start debugging\n"
printf "Type 'continue' to continue execution\n"
printf "Type 'bt' for backtrace\n"
printf "Type 'info threads' to see all threads\n"
printf "========================================\n"

# Auto-run with test command
# run new -s test-debug
