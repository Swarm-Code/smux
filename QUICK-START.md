# ⚡ Quick Start - Resize Investigation

## What's Ready

✅ **smux rebuilt** with resize logging
✅ **Binary installed** at `/usr/local/bin/smux`
✅ **Logs cleared** - fresh slate
✅ **Analysis script** ready: `./analyze-resize-logs.sh`

## Root Cause Found (Phase 1)

**Terminal is RESIZING during Claude Code output!**

Ceregrep analysis revealed:
- Screen height changing: **sy: 19 → 20 → 24 → 27**
- This triggers massive scroll cascades
- Result: 4,000-6,700 scrolls/second

## Test Now (Phase 2)

### 1. Start Test
```bash
# Start fresh smux session
smux new-session -s resize-test

# Launch Claude Code inside
claude-code

# Use it normally - let it generate output
```

### 2. Analyze Results
```bash
# Detach from smux (Ctrl-b d)

# Run analysis
./analyze-resize-logs.sh

# Or check manually
cat /tmp/smux_resize.log
```

### 3. What to Look For

**Key question**: How many resize events?

- **0-5 resizes**: Not the root cause, keep investigating
- **10-50 resizes**: **ROOT CAUSE CONFIRMED!**
- **100+ resizes**: Severe resize loop issue

**Expected output** (if resizing is the cause):
```
[timestamp] PANE_RESIZE pane=%1 old=80x19 new=80x20 delta_x=0 delta_y=+1
[timestamp] PANE_RESIZE pane=%1 old=80x20 new=80x24 delta_x=0 delta_y=+4
[timestamp] PANE_RESIZE pane=%1 old=80x24 new=80x27 delta_x=0 delta_y=+3
```

## What Happens Next

**If Claude Code sends resize signals**:
- Report to Anthropic as bug
- Implement smux-side filtering

**If smux auto-resizes**:
- Implement resize throttling
- Add debouncing logic

**If resize triggers scrolls**:
- Batch scroll operations during resize
- Defer screen reorganization

## Files Created

**Documentation**:
- `STATUS.md` - Full status overview
- `RESIZE-INVESTIGATION-READY.md` - Comprehensive Phase 2 guide
- `ROOT-CAUSE-ANALYSIS.md` - Phase 1 analysis
- `DETECTION-READY.md` - Instrumentation details
- `QUICK-START.md` - This file

**Scripts**:
- `analyze-resize-logs.sh` - Log analysis tool

**Logs** (after testing):
- `/tmp/smux_resize.log` - **Primary data source**
- `/tmp/smux_scroll_trigger.log` - Correlation data
- `/tmp/smux_pty_raw.log` - ANSI sequence check

## Summary

**Problem**: Claude Code causes UI jitter in smux
**Discovery**: Terminal resizing (sy: 19→27)
**Hypothesis**: Resize triggers scroll cascades
**Phase 2**: Capture resize events
**Status**: **READY TO TEST**

---

**Just run**: `smux new-session -s resize-test` and use Claude Code normally!
