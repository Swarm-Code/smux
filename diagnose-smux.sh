#!/bin/bash
echo "=== SMUX Diagnostic Information ==="
echo ""
echo "1. Current TTY:"
tty
echo ""
echo "2. TMUX environment variable:"
echo "TMUX=$TMUX"
echo ""
echo "3. stdin/stdout/stderr devices:"
echo "stdin:  $(readlink -f /proc/self/fd/0)"
echo "stdout: $(readlink -f /proc/self/fd/1)"
echo "stderr: $(readlink -f /proc/self/fd/2)"
echo ""
echo "4. TTY permissions:"
ls -l $(tty)
echo ""
echo "5. Process tree:"
ps -p $$ -o pid,ppid,cmd
echo ""
echo "6. Parent process:"
ps -p $PPID -o pid,ppid,cmd
echo ""
echo "7. Are we in tmux/smux?"
if [ -n "$TMUX" ]; then
    echo "YES - inside tmux/smux session"
else
    echo "NO - not in tmux/smux"
fi
echo ""
echo "8. Running smux processes:"
ps aux | grep -E '[s]mux|[t]mux' | head -10
