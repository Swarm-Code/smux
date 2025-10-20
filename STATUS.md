# 📊 Smux Scroll Investigation - Current Status

**Last Updated**: 2025-10-19 20:57 UTC
**Current Phase**: Phase 2 - Resize Investigation
**Status**: ✅ **READY FOR TESTING**

---

## 🎯 Quick Summary

**Problem**: Claude Code causes 4,000-6,700 scrolls/second in smux, resulting in UI jitter and flickering.

**Root Cause Discovered**: Terminal is **dynamically resizing** during Claude Code output (sy: 19→20→24→27).

**Current Objective**: Capture resize events to understand WHEN, WHY, and HOW the terminal resizes.

---

## ✅ What's Been Completed

### Phase 1: Root Cause Analysis

1. ✅ **Three-layer instrumentation implemented**:
   - Layer 1: PTY raw input logging (`window.c:1040-1087`)
   - Layer 2: Linefeed parser logging (`input.c:1289-1304`)
   - Layer 3: Scroll trigger logging (`screen-write.c:1451-1468`)

2. ✅ **Captured scroll behavior**:
   - 1.5 million scroll events over 480 seconds
   - 4,000-6,700 scrolls/second sustained rate
   - 94.7% of scrolls occur in sub-millisecond bursts

3. ✅ **Ceregrep analysis revealed resize pattern**:
   - Screen height changing: sy: 19 → 20 → 24 → 27
   - Cursor always at bottom: cy == rlower
   - User's insight confirmed: "terminal shrinking and growing"

4. ✅ **Scroll batching attempted and reverted**:
   - Initial implementation: deferred scroll execution
   - Problem discovered: grid state desync breaks rendering
   - Reverted to original scroll behavior
   - Lesson learned: can't defer grid operations

5. ✅ **GitHub issue created**:
   - Issue #9935 on Claude Code repository
   - Documented 4,000-6,700 scroll rate
   - Shared analysis and findings

### Phase 2: Resize Investigation Setup

6. ✅ **Resize logging implemented** (`window.c:1125-1146`):
   - Captures every window pane resize operation
   - Logs old/new dimensions, deltas, timestamps
   - Output: `/tmp/smux_resize.log`

7. ✅ **Build and installation**:
   - Clean rebuild with resize logging
   - Binary installed: `/usr/local/bin/smux` (5.5MB with debug symbols)
   - Verified: resize logging strings present in binary

8. ✅ **Environment prepared**:
   - Old smux processes killed
   - Debug logs cleared
   - Fresh slate for Phase 2 testing

9. ✅ **Documentation created**:
   - `ROOT-CAUSE-ANALYSIS.md`: Complete Phase 1 analysis
   - `SCROLL-BATCHING-IMPLEMENTED.md`: Batching attempt (now outdated)
   - `DETECTION-READY.md`: Updated for Phase 2
   - `RESIZE-INVESTIGATION-READY.md`: Phase 2 comprehensive guide
   - `STATUS.md`: This file
   - `analyze-resize-logs.sh`: Helper script for log analysis

---

## 🔬 Current Instrumentation

### 4-Layer Diagnostic System

```
Claude Code Output
       ↓
[Layer 1: PTY Raw] → /tmp/smux_pty_raw.log
       ↓ (raw bytes from Claude Code)
[Layer 2: Linefeed Parser] → /tmp/smux_linefeed.log
       ↓ (parsed LF/VT/FF characters)
[Layer 3: Scroll Trigger] → /tmp/smux_scroll_trigger.log
       ↓ (scroll events when cy == rlower)
[Layer 4: Resize Events] → /tmp/smux_resize.log ← NEW!
       ↓ (window dimension changes)
   Grid Operations
```

**Key Files**:
- `/tmp/smux_resize.log` - **PRIMARY FOCUS** for Phase 2
- `/tmp/smux_scroll_trigger.log` - Correlation with scrolls
- `/tmp/smux_pty_raw.log` - Check for ANSI resize sequences
- `/tmp/smux_linefeed.log` - Linefeed patterns

---

## 🚀 Next Steps (Ready to Execute)

### Immediate: Phase 2 Testing

1. **Start fresh smux session**:
   ```bash
   smux new-session -s resize-test
   ```

2. **Launch Claude Code inside smux**:
   ```bash
   claude-code
   ```

3. **Use Claude Code normally** (let it generate output naturally)

4. **Detach and analyze**:
   ```bash
   # Detach: Ctrl-b d (or your smux prefix + d)

   # Run analysis script
   ./analyze-resize-logs.sh

   # Or manual analysis
   cat /tmp/smux_resize.log
   grep "PANE_RESIZE" /tmp/smux_resize.log | wc -l
   ```

### Analysis Goals

**Answer these questions**:
1. How many resize events occur? (expect 10-50 if this is the root cause)
2. What is the resize pattern? (growing? shrinking? oscillating?)
3. Do resizes correlate with scroll bursts?
4. What triggers the resize? (ANSI sequences? SIGWINCH? internal logic?)

### Possible Outcomes & Solutions

**If Claude Code sends resize signals**:
- Report to Anthropic as bug
- Implement resize signal filtering in smux

**If smux auto-resizes**:
- Implement resize throttling/debouncing
- Add option to disable auto-resize during rapid output

**If resize triggers scroll cascades**:
- Batch scroll operations during resize
- Defer screen reorganization until resize completes

---

## 📊 Expected Results

Based on Phase 1 findings, if resizing is the root cause:

- **Resize count**: 10-50 events during typical Claude Code response
- **Pattern**: Screen growing (19 → 20 → 24 → 27 lines)
- **Correlation**: Each resize followed by burst of 100-500 scrolls
- **Timing**: Resizes happen during rapid output phases

**This would explain**:
- Why scrolls happen in massive bursts
- Why cursor is always at bottom (cy == rlower)
- Why screen state changes during output
- Why jitter/flickering occurs

---

## 🛠️ Tools Available

### Analysis Script

```bash
./analyze-resize-logs.sh
```

**Features**:
- Counts resize events
- Shows resize timeline
- Analyzes resize patterns (Y-axis deltas)
- Correlates resize events with scroll bursts
- Calculates resize-to-scroll ratio
- Provides manual investigation commands

### Manual Commands

```bash
# View all resizes
cat /tmp/smux_resize.log

# Count resizes
grep -c "PANE_RESIZE" /tmp/smux_resize.log

# Resize pattern
grep "PANE_RESIZE" /tmp/smux_resize.log | awk '{print $5, $6, $7}'

# Check for ANSI resize sequences
grep -a '\x1b\[8' /tmp/smux_pty_raw.log
```

---

## 📋 Files in This Directory

### Documentation
- `STATUS.md` ← **YOU ARE HERE**
- `ROOT-CAUSE-ANALYSIS.md` - Complete Phase 1 findings
- `RESIZE-INVESTIGATION-READY.md` - Phase 2 comprehensive guide
- `DETECTION-READY.md` - Updated instrumentation docs
- `SCROLL-BATCHING-IMPLEMENTED.md` - Batching attempt (outdated)

### Scripts
- `analyze-resize-logs.sh` - **USE THIS** for log analysis
- `diagnose-smux.sh` - Earlier diagnostic script
- `monitor-scroll.sh` - Scroll monitoring script
- `test-rapid-output.sh` - Output generation test

### Source Code (Modified)
- `window.c` - PTY raw logging + **resize logging** (lines 1040-1087, 1125-1146)
- `input.c` - Linefeed parser logging (lines 1289-1304)
- `screen-write.c` - Scroll trigger logging (lines 1451-1468)
- `grid.c` - Grid scroll logging (lines 419-430)

---

## 🎓 Lessons Learned

### What Worked
- ✅ Three-layer instrumentation provided complete data flow visibility
- ✅ Microsecond timestamps revealed sub-millisecond scroll bursts
- ✅ Ceregrep analysis identified resize pattern from 1.5M scroll events
- ✅ User's insight ("terminal shrinking and growing") was correct

### What Didn't Work
- ❌ Scroll batching (deferred execution broke grid state)
- ❌ Simple rate limiting (doesn't address root cause)

### Key Insights
- Can't defer grid operations (breaks cursor positioning)
- Must fix SOURCE of problem, not symptoms
- Data-driven root cause analysis > quick fixes
- User observations are valuable - listen carefully

---

## 🔍 Current Investigation State

**Hypothesis**: Terminal resizing triggers scroll cascades

**Evidence**:
- Ceregrep found sy changing: 19 → 20 → 24 → 27
- Cursor always at bottom (cy == rlower)
- 4,000-6,700 scrolls/second is too high to be normal output

**Missing Data**:
- HOW MANY resize events occur? (need `/tmp/smux_resize.log`)
- WHAT triggers the resize? (ANSI? SIGWINCH? internal?)
- WHEN do resizes happen relative to scrolls?

**Phase 2 will answer these questions.**

---

## 🚨 Important Reminders

- **USE SMUX, NOT TMUX** - This is a smux fork with custom changes
- **Let Claude Code run naturally** - Don't force artificial output
- **Capture 30-60 seconds** of normal Claude Code operation
- **Check ALL log files** - Correlation between logs is key
- **Use the analysis script** - `./analyze-resize-logs.sh`

---

## ✅ System Verification

**Binary Status**:
```
File: /usr/local/bin/smux
Size: 5.5MB
Debug symbols: Present (with debug_info, not stripped)
Resize logging: ✅ Confirmed (strings check passed)
Build time: 2025-10-19 20:52
```

**Environment Status**:
```
Smux processes: None (clean)
Debug logs: None (clean)
Documentation: Complete
Scripts: Ready
```

**System is GO for Phase 2 testing.**

---

## 📞 Reference

**GitHub Issue**: #9935 (Claude Code repository)
**Build Version**: smux next-3.7
**Instrumentation**: 4-layer diagnostic system
**Primary Log**: `/tmp/smux_resize.log`

---

**Status**: ✅ **READY FOR PHASE 2 TESTING**

**When you're ready to test, just run**:
```bash
smux new-session -s resize-test
# Then launch Claude Code inside
# Use it normally
# Detach and run: ./analyze-resize-logs.sh
```

**The instrumentation is active and will automatically capture resize events.**
