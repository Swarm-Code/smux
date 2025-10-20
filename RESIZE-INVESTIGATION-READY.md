# 🔍 Smux Resize Investigation - READY FOR TESTING

## Executive Summary

**Status**: ✅ **READY FOR PHASE 2 TESTING**

**Root Cause Discovery**: Ceregrep analysis revealed the scroll issue is actually caused by **TERMINAL RESIZING** during Claude Code output.

**Current Phase**: Instrumentation to capture WHEN and WHY terminal dimensions change.

---

## 📊 Phase 1 Results (Ceregrep Analysis)

### What We Discovered

Using ceregrep to analyze `/tmp/smux_scroll_trigger.log`:

```bash
ceregrep query "analyze the scroll trigger log and find patterns"
```

**Critical Finding**: The terminal is **dynamically resizing** during Claude Code operation!

**Evidence**:
```
State 1: cy=18 rlower=18 sy=19  (19-line screen)
State 2: cy=19 rlower=19 sy=20  (20-line screen)
State 3: cy=23 rlower=23 sy=24  (24-line screen)
State 4: cy=26 rlower=26 sy=27  (27-line screen)
```

**Screen height (sy) transitions**: 19 → 20 → 24 → 27 lines

**Cursor position (cy)**: ALWAYS equals rlower (cursor always at bottom)

### Why This Matters

When the terminal resizes:
1. Screen must reorganize all content
2. Scroll regions are recalculated
3. Line wrapping may change
4. This triggers MASSIVE scroll operations

**If the terminal is resizing 10-20 times during output:**
- Each resize triggers screen reorganization
- Each reorganization causes scroll cascades
- This explains the 4,000-6,700 scrolls/second!

---

## 🎯 Phase 2 Objective

**Goal**: Capture resize events to understand:
1. HOW OFTEN does the terminal resize?
2. WHEN do resizes occur? (continuously? bursts?)
3. WHAT triggers the resize? (external signal? internal logic? Claude Code?)
4. HOW does resize correlate with scroll bursts?

---

## 🔧 What Was Implemented

### New Instrumentation: Resize Event Logging

**Location**: `/home/alejandro/Swarm/smux/window.c:1125-1146`

**Function**: `window_pane_resize()`

**Code Added**:
```c
/* WINDOW RESIZE LOGGING FOR DEBUGGING */
{
    static FILE *resize_log = NULL;
    if (resize_log == NULL)
        resize_log = fopen("/tmp/smux_resize.log", "a");

    if (resize_log != NULL) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        fprintf(resize_log,
                "[%ld.%06ld] PANE_RESIZE pane=%%%u old=%ux%u new=%ux%u delta_x=%d delta_y=%d\n",
                tv.tv_sec, tv.tv_usec, wp->id,
                wp->sx, wp->sy, sx, sy,
                (int)sx - (int)wp->sx, (int)sy - (int)wp->sy);
        fflush(resize_log);
    }
}
```

**What It Captures**:
- Pane ID (to identify which pane is resizing)
- Old dimensions (before resize): `wp->sx × wp->sy`
- New dimensions (after resize): `sx × sy`
- Delta changes: `delta_x`, `delta_y`
- Microsecond timestamps

**Output File**: `/tmp/smux_resize.log`

**Example Output**:
```
[1760920078.123456] PANE_RESIZE pane=%1 old=80x19 new=80x20 delta_x=0 delta_y=+1
[1760920078.234567] PANE_RESIZE pane=%1 old=80x20 new=80x24 delta_x=0 delta_y=+4
[1760920078.345678] PANE_RESIZE pane=%1 old=80x24 new=80x27 delta_x=0 delta_y=+3
```

### Build and Installation Status

- ✅ **Code modified**: `window.c` with resize logging
- ✅ **Build successful**: Clean build with `-DDEBUG`
- ✅ **Installation complete**: `/usr/local/bin/smux`
- ✅ **Old processes killed**: `pkill -9 smux`
- ✅ **Logs cleared**: `rm -f /tmp/smux_*.log`

---

## 🚀 Testing Instructions

### Quick Start

```bash
# 1. Start fresh smux session
smux new-session -s resize-test

# 2. Inside smux, launch Claude Code
claude-code

# 3. Trigger rapid output (use Claude Code normally or generate output)
# The resize logging will capture events automatically

# 4. Detach and analyze
# Ctrl-b d (or your smux prefix + d)

# 5. Check resize logs
cat /tmp/smux_resize.log
```

### What to Look For

**Resize Event Count**:
```bash
grep "PANE_RESIZE" /tmp/smux_resize.log | wc -l
```

**Resize Timeline**:
```bash
grep "PANE_RESIZE" /tmp/smux_resize.log
```

**Correlation with Scrolls**:
```bash
# Compare resize timestamps with scroll trigger timestamps
paste <(grep "PANE_RESIZE" /tmp/smux_resize.log) \
      <(grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | head -n $(grep -c "PANE_RESIZE" /tmp/smux_resize.log))
```

---

## 🔬 Analysis Plan

### Step 1: Count Resize Events

How many times does the terminal resize during Claude Code operation?

**Expected**: If resizing is the root cause, we should see 10-50 resize events during a typical Claude Code response.

### Step 2: Analyze Resize Pattern

Is the terminal:
- **Growing continuously**? (19 → 20 → 24 → 27...)
- **Oscillating**? (19 → 24 → 19 → 24...)
- **Shrinking**? (27 → 24 → 20 → 19...)

### Step 3: Time Correlation

Do resize events happen:
- **Before scroll bursts**? (resize triggers scrolls)
- **During scroll bursts**? (scrolls trigger resize?)
- **After scroll bursts**? (resize is consequence?)

### Step 4: Identify Trigger

What causes the resize?
- **Claude Code sending SIGWINCH**? (window change signal)
- **Claude Code sending ANSI resize sequences**? (`\e[8;rows;cols t`)
- **Smux auto-resizing**? (internal logic)
- **External window manager**? (user resizing terminal window)

---

## 🎯 Expected Outcomes

### Scenario 1: Claude Code is Sending Resize Signals

**Evidence**:
- `/tmp/smux_pty_raw.log` contains ANSI resize sequences
- Resize events correlate with specific PTY data chunks

**Solution**:
- Report to Anthropic as a bug
- Implement resize signal filtering in smux

### Scenario 2: Smux is Auto-Resizing

**Evidence**:
- No resize signals in PTY raw data
- Resize events correlate with content overflow

**Solution**:
- Implement intelligent resize throttling
- Add resize debouncing (wait for output to stabilize)

### Scenario 3: Resize Triggers Scroll Cascades

**Evidence**:
- Each resize event immediately followed by 100-500 scroll events
- Scroll rate spikes correlate with resize timestamps

**Solution**:
- Batch scroll operations during resize
- Defer screen reorganization until resize completes
- Implement resize-aware scroll buffering

---

## 📋 Testing Checklist

Before starting test:
- [x] Smux rebuilt with resize logging
- [x] Binary installed to `/usr/local/bin/smux`
- [x] Old smux processes killed
- [x] Debug logs cleared
- [ ] Fresh smux session started
- [ ] Claude Code launched inside smux
- [ ] Rapid output triggered

After test:
- [ ] Resize log collected: `/tmp/smux_resize.log`
- [ ] Scroll trigger log collected: `/tmp/smux_scroll_trigger.log`
- [ ] PTY raw log collected: `/tmp/smux_pty_raw.log`
- [ ] Resize count analyzed
- [ ] Resize pattern identified
- [ ] Correlation with scrolls established
- [ ] Root cause determined

---

## 🎓 Why This Approach is Better

### Previous Approach (Scroll Batching)
- **Problem**: Deferred scroll execution
- **Consequence**: Grid state desync, broken rendering
- **Why it failed**: Can't defer grid operations without breaking cursor positioning

### Current Approach (Resize Investigation)
- **Advantage**: Understand WHY scrolls happen before fixing them
- **Methodology**: Data-driven root cause analysis
- **Goal**: Fix the SOURCE of the problem, not the symptom

**User's Wisdom**: "it comes down to the terminal shrinking and growing and lines being displayed over and over"

---

## 📊 Success Criteria

**Phase 2 is successful when we can answer**:
1. ✅ How many resize events occur? (quantitative)
2. ✅ What is the resize pattern? (qualitative)
3. ✅ Do resizes correlate with scroll bursts? (correlation)
4. ✅ What triggers the resize? (root cause)

**Next Steps After Phase 2**:
1. Implement targeted fix based on resize trigger
2. Test fix with Claude Code
3. Verify jitter/flickering eliminated
4. Update GitHub issue with findings

---

## 🚨 Important Notes

- **DO NOT** test with tmux - only smux (this is a smux fork)
- **DO** let Claude Code run normally - don't force artificial output
- **DO** capture logs for 30-60 seconds of Claude Code operation
- **DO** check all log files - correlation is key

---

**Status**: ✅ **READY FOR PHASE 2 TESTING**

**Last Updated**: 2025-10-19
**Build Version**: smux next-3.7 with 4-layer instrumentation
**Resize Logging**: Active at `window.c:1125-1146`
