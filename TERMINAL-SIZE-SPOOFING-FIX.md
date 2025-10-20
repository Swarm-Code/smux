# ✅ Terminal Size Spoofing Fix - READY TO TEST

## What Was Fixed

**Root Cause Identified:**
- Claude Code's UI is ~300 lines tall
- Your terminal shows only 19 lines
- Claude Code detected "overflow" and spam-redrew ~31 times/second
- Each redraw caused ~256 scroll operations
- Result: **7,340+ scrolls/second** causing severe flickering/jitter

**The Elegant Solution:**
Instead of trying to batch/suppress scrolls (complex, can break rendering), we **lie to Claude Code about terminal size**:

```c
/* window.c:443-447 - window_pane_send_resize() */
ws.ws_row = (sy < 100) ? 300 : sy;  /* Report 300 lines when really 19 */
```

**Why This Works:**
- When terminal is 19 lines tall, smux tells Claude Code it's 300 lines
- Claude Code thinks its ~300-line UI "fits" perfectly
- Claude Code stops spam-redrawing (no overflow detected!)
- Smux still renders correctly (only shows actual 19 lines)
- **Simple, elegant, addresses root cause**

## Files Modified

**window.c** (lines 439-448):
```c
log_debug("%s: %%%u resize to %u,%u", __func__, wp->id, sx, sy);

memset(&ws, 0, sizeof ws);
ws.ws_col = sx;
/* INTELLIGENT FIX: Report LARGE height to Claude Code so it thinks its UI fits */
/* This stops Claude Code from spam-redrawing! Real height: sy, Fake height: 300 */
ws.ws_row = (sy < 100) ? 300 : sy;  /* Only lie if terminal is small */
ws.ws_xpixel = w->xpixel * sx;  /* Use real sx for pixels */
ws.ws_ypixel = w->ypixel * sy;  /* Use real sy for pixels */
if (ioctl(wp->fd, TIOCSWINSZ, &ws) == -1)
```

## Current Status

✅ **Fix implemented** in window.c
✅ **smux rebuilt** successfully
✅ **Binary installed** at `/usr/local/bin/smux`
✅ **Logs cleared** - fresh slate
✅ **Old smux processes killed**

**STATUS: READY TO TEST!**

## How to Test

### 1. Start Fresh Smux Session
```bash
smux new-session -s spoofing-test
```

### 2. Launch Claude Code
```bash
# Inside smux session
claude-code
```

### 3. Use Claude Code Normally
- Ask it questions
- Let it generate code/output
- Watch for flickering/jitter

### 4. Expected Results

**✅ SUCCESS (Fix Works):**
- **NO flickering** during Claude Code output
- Smooth, clean rendering
- UI feels responsive and stable
- When you detach and check logs:
  ```bash
  # Detach: Ctrl-b d
  wc -l /tmp/smux_scroll_trigger.log
  ```
  Should show **DRAMATICALLY fewer scrolls** (maybe 100-500 instead of 850,000+)

**❌ FAILURE (Fix Didn't Work):**
- Flickering still present
- Logs show 100,000+ scroll events
- Same jitter as before

### 5. Verify Logs (After Testing)

```bash
# Detach from smux first: Ctrl-b d

# Check scroll events
echo "=== SCROLL EVENTS ==="
wc -l /tmp/smux_scroll_trigger.log

# Check clear screen events
echo "=== CLEAR SCREENS ==="
wc -l /tmp/smux_clear_screen.log

# Check resize events (should see our spoofing!)
echo "=== RESIZE EVENTS ==="
cat /tmp/smux_resize.log

# Look for our 300-line spoofing
grep "80x300" /tmp/smux_resize.log || echo "No size spoofing logged"
```

**What to Look For in Resize Log:**
You should see smux reporting terminal size as `80x300` or similar (width x 300 height) even though your real terminal is much smaller (e.g., 76x19).

Example expected output:
```
[timestamp] PANE_RESIZE pane=%1 old=80x24 new=80x300 delta_x=0 delta_y=+276
```

This proves Claude Code is being told the terminal is 300 lines tall!

## Understanding the Fix

**Before Fix:**
1. Real terminal: 80x19 (width x height)
2. Smux tells Claude Code: 80x19
3. Claude Code's UI: 300 lines (doesn't fit!)
4. Claude Code detects overflow
5. Claude Code spam-redraws trying to "fix" it
6. Result: 7,000+ scrolls/second, severe flickering

**After Fix:**
1. Real terminal: 80x19
2. **Smux LIES to Claude Code: 80x300** ← THE FIX!
3. Claude Code's UI: 300 lines (now "fits"!)
4. Claude Code happy, no overflow detected
5. Claude Code renders normally, no spam-redraw
6. Result: Minimal scrolling, smooth rendering

## Troubleshooting

### If Flickering Still Occurs:

1. **Verify smux version:**
   ```bash
   smux -V
   # Should show "smux next-3.7" or similar
   ```

2. **Check if fix is active:**
   ```bash
   grep "INTELLIGENT FIX" /home/alejandro/Swarm/smux/window.c
   # Should show the comment line
   ```

3. **Verify binary was installed:**
   ```bash
   which smux
   # Should show /usr/local/bin/smux

   ls -lh /usr/local/bin/smux
   # Should show recent timestamp (today)
   ```

4. **Check resize log for spoofing:**
   ```bash
   cat /tmp/smux_resize.log
   # Should show ws_row=300 when terminal is small
   ```

### If No Resize Events Logged:

The resize might not trigger until you actually change terminal size. Try:
```bash
# Inside smux, press Ctrl-b :
# Then type: resize-pane -y 20
```

This will trigger a resize event that should be logged.

## Technical Details

**What Gets Spoofed:**
- `ws.ws_row` in TIOCSWINSZ ioctl (terminal height in characters)
- Only when `sy < 100` (don't interfere with genuinely large terminals)

**What Stays Real:**
- `ws.ws_col` (width - no spoofing needed, Claude Code handles width fine)
- `ws.ws_xpixel` and `ws.ws_ypixel` (pixel dimensions - use real size for proper rendering)
- Actual grid/screen rendering (smux still only shows real 19 lines)

**Why 300 Lines?**
- Claude Code's UI appears to be ~300 lines in typical usage
- This ensures it always thinks it "fits"
- Large enough to prevent overflow, small enough to be reasonable
- Won't cause issues with terminal capabilities

## Success Criteria

**Primary:** No flickering during Claude Code output ✨

**Secondary Indicators:**
- Scroll events drop from ~850,000 to <1,000
- Clear screen events drop from 626 to minimal
- Resize log shows spoofed height (300 instead of 19)
- CPU usage during Claude Code output drops significantly

## What's Next

**If Fix Works:**
1. Test with different terminal sizes
2. Verify no side effects with other applications
3. Consider making 300 configurable (smux.conf option)
4. Document this as the official fix

**If Fix Doesn't Work:**
1. Analyze logs to see if spoofing is happening
2. Check if Claude Code is reading terminal size differently
3. Try different spoofed heights (400? 500?)
4. Fall back to scroll batching approach

## Credit

**User's Insight:** "if I zoom out my render or terminal size it stops doing it"
**User's Solution:** "make it so the terminal is large but we dont show everything"

This brilliant observation led directly to the size spoofing fix! 🎯

---

## Quick Test Command

```bash
# One-liner to test everything
smux new-session -s test 'claude-code' && sleep 30 && smux detach && echo "=== SCROLL EVENTS ===" && wc -l /tmp/smux_scroll_trigger.log && echo "=== RESIZE EVENTS ===" && cat /tmp/smux_resize.log
```

**Ready to test!** Just run `smux new-session -s spoofing-test` and use Claude Code normally! 🚀
