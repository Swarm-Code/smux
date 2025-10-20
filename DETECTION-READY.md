# Smux Scroll Detection Instrumentation - PHASE 2: RESIZE INVESTIGATION

## 🎯 Current Objective (Phase 2)
**ROOT CAUSE IDENTIFIED via ceregrep**: Terminal is RESIZING during Claude Code output (sy: 19→20→24→27)

Previous analysis confirmed 4,000-6,700 scrolls/second, but ceregrep analysis revealed the REAL issue:
- Screen dimensions changing dynamically during output
- Cursor Y (cy) always equals lower scroll bound (rlower)
- Screen height (sy) transitions: 19→20→24→27 lines

**New Goal**: Understand WHY and WHEN the terminal resizes during Claude Code operation.

## 📊 Three-Layer Instrumentation Architecture

The instrumented smux binary now has three diagnostic layers that capture the complete data flow:

```
Claude Code Output
       ↓
[LAYER 1: PTY Raw Input] ← /tmp/smux_pty_raw.log
       ↓ (raw bytes from Claude Code's PTY)
[LAYER 2: Linefeed Parser] ← /tmp/smux_linefeed.log
       ↓ (parsed LF/VT/FF control characters)
[LAYER 3: Scroll Trigger] ← /tmp/smux_scroll_trigger.log
       ↓ (actual scroll events when cy == rlower)
  Grid Scroll Execution
```

### Layer 1: PTY Raw Input (`window.c:1040-1087`)
**File**: `/tmp/smux_pty_raw.log`

**Captures**:
- Raw bytes arriving from Claude Code's PTY
- Hex dump of first 512 bytes per read callback
- ASCII representation (with `\n`, `\r`, `\033` visible)
- Newline count per chunk
- Total bytes received and callback count
- Microsecond timestamps

**Purpose**: Understand the EXACT data Claude Code is sending before any parsing

**Example Output**:
```
[1729371234.123456] PTY_READ pane=%1 bytes=2048 total_bytes=524288 callback#42
  HEX: 1b 5b 33 31 6d 48 65 6c 6c 6f 0a 1b 5b 30 6d ...
  ASCII: \e[31mHello\n\e[0m...
  NEWLINES_IN_CHUNK: 15
```

### Layer 2: Linefeed Counting (`input.c:1289-1304`)
**File**: `/tmp/smux_linefeed.log`

**Captures**:
- Every LF (\n / 0x0A), VT (\v / 0x0B), FF (\f / 0x0C) character processed
- Sequential linefeed counter
- Microsecond timestamps
- Character hex code

**Purpose**: Count how many linefeeds are being processed from the raw input

**Example Output**:
```
[1729371234.123456] LF#1 char=\x0a
[1729371234.123512] LF#2 char=\x0a
[1729371234.123598] LF#3 char=\x0a
```

### Layer 3: Scroll Trigger Detection (`screen-write.c:1451-1468`)
**File**: `/tmp/smux_scroll_trigger.log`

**Captures**:
- Actual scroll events triggered by `screen_write_linefeed()`
- Only logs when scroll condition is met: `s->cy == s->rlower`
- Screen state: cursor position (cy), scroll region bounds (rupper, rlower)
- Sequential scroll trigger counter
- Microsecond timestamps

**Purpose**: Identify when linefeeds actually cause scrolling (cursor at bottom of scroll region)

**Example Output**:
```
[1729371234.123456] SCROLL_TRIGGER#1 cy=23 rlower=23 rupper=0 sy=24 wrapped=0
[1729371234.123512] SCROLL_TRIGGER#2 cy=23 rlower=23 rupper=0 sy=24 wrapped=0
[1729371234.123598] SCROLL_TRIGGER#3 cy=23 rlower=23 rupper=0 sy=24 wrapped=0
```

### Legacy Layer: Grid Scroll History (`grid.c:419-430`)
**File**: `/tmp/smux_scroll_debug.log`

**Captures**:
- Grid scroll history operations
- Line wrapping state
- Scroll region details
- Time deltas between scrolls

**Purpose**: Previously added instrumentation - shows grid-level scroll execution

### **NEW** Layer 4: Window Resize Detection (`window.c:1125-1146`)
**File**: `/tmp/smux_resize.log`

**Captures**:
- Window pane resize operations via `window_pane_resize()`
- Old dimensions (wp->sx, wp->sy) before resize
- New dimensions (sx, sy) after resize
- Delta changes (delta_x, delta_y)
- Pane ID for correlation
- Microsecond timestamps

**Purpose**: **THIS IS THE KEY!** Understand WHEN and WHY the terminal dimensions change during Claude Code output

**Example Output**:
```
[1760920078.123456] PANE_RESIZE pane=%1 old=80x19 new=80x20 delta_x=0 delta_y=+1
[1760920079.234567] PANE_RESIZE pane=%1 old=80x20 new=80x24 delta_x=0 delta_y=+4
[1760920080.345678] PANE_RESIZE pane=%1 old=80x24 new=80x27 delta_x=0 delta_y=+3
```

**What to Look For**:
- Frequency of resize events (continuous? bursts?)
- Correlation with scroll bursts from Layer 3
- Delta patterns (always growing? shrinking? oscillating?)
- What triggers the resize (external signal? internal logic?)

---

## 🚀 Test Procedure

### Step 1: Start Fresh Smux Session
```bash
# Kill any existing smux sessions
pkill -9 smux

# Verify logs are cleared (should show "No such file or directory")
ls -lh /tmp/smux_*.log

# Start new smux session
smux new-session -s debug-scroll
```

### Step 2: Launch Claude Code Inside Smux
```bash
# Inside the smux session, start Claude Code in this directory
claude-code
```

### Step 3: Trigger Rapid Output
**Method 1**: Ask Claude Code to generate rapid output
```
Hey Claude, please output a large amount of text rapidly to test terminal scrolling.
Generate at least 500 lines of output as fast as possible.
```

**Method 2**: Run a command that generates output
```bash
# Inside Claude Code's shell
for i in {1..1000}; do echo "Line $i: Testing scroll performance with Claude Code output"; done
```

**Method 3**: Let normal Claude Code responses trigger it (original issue)
- Just use Claude Code normally
- The flickering/jitter should appear during response generation

### Step 4: Observe and Stop
- Watch for UI jitter/flicker
- Let it run for ~10-30 seconds of rapid output
- Detach from smux: `Ctrl-b d` (or whatever your prefix is)

### Step 5: Collect and Analyze Logs
```bash
# Check log sizes
ls -lh /tmp/smux_*.log

# Quick stats
echo "=== PTY Raw Input Stats ==="
grep "PTY_READ" /tmp/smux_pty_raw.log | tail -1
grep "NEWLINES_IN_CHUNK" /tmp/smux_pty_raw.log | \
    awk '{sum+=$2} END {print "Total newlines in raw input:", sum}'

echo "=== Linefeed Parser Stats ==="
grep "^\\[" /tmp/smux_linefeed.log | wc -l

echo "=== Scroll Trigger Stats ==="
grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | wc -l

echo "=== **NEW** Resize Event Stats ==="
grep "PANE_RESIZE" /tmp/smux_resize.log | wc -l
echo "First resize:"
head -1 /tmp/smux_resize.log
echo "Last resize:"
tail -1 /tmp/smux_resize.log

echo "=== Grid Scroll Stats ==="
grep "SCROLL_EVENT" /tmp/smux_scroll_debug.log | wc -l
```

---

## 🔍 Analysis Guide

### Key Questions to Answer

1. **🔥 PRIMARY: Resize Event Analysis** (`/tmp/smux_resize.log`) **← START HERE**
   - How many resize events occur during the test?
   - What is the frequency of resize events? (continuous? bursts?)
   - What is the resize pattern? (growing? shrinking? oscillating?)
   - Are resizes correlated with scroll bursts from Layer 3?
   - Does each resize trigger a cascade of scrolls?
   - Is Claude Code sending resize signals (SIGWINCH, ANSI sequences)?
   - Is smux resizing automatically for some internal reason?

2. **Raw Input Analysis** (`/tmp/smux_pty_raw.log`)
   - How many bytes is Claude Code sending?
   - What's the pattern of data? (streaming? bursts? chunks?)
   - How many newlines per chunk?
   - Are there ANSI escape sequences? (`\033[...m`)
   - **CRITICAL**: Are there window size control sequences? (`\033[8;rows;cols t`)
   - Is there carriage return + linefeed (`\r\n`) or just linefeed (`\n`)?

3. **Linefeed Processing** (`/tmp/smux_linefeed.log`)
   - How many linefeeds total were parsed?
   - What's the time distribution? (rapid bursts? steady stream?)
   - Compare to newline count from Layer 1

4. **Scroll Triggers** (`/tmp/smux_scroll_trigger.log`)
   - How many linefeeds actually triggered scrolls?
   - What percentage of linefeeds cause scrolling?
   - What's the cursor position pattern? (always cy == rlower?)
   - Time gaps between scroll triggers?
   - **Do scroll bursts happen AFTER resize events?**

5. **Data Flow Comparison**
   ```
   Resize Events (Layer 4) ← NEW FOCUS
       ↓ (triggers screen reorganization)
   Raw Newlines (Layer 1)
       vs
   Parsed Linefeeds (Layer 2)
       vs
   Scroll Triggers (Layer 3)
       vs
   Grid Scrolls (Legacy Layer)
   ```
   - Do resize events trigger scroll cascades?
   - Where is data being multiplied?
   - Where is data being lost?
   - What's the ratio at each stage?

### Example Analysis Commands

```bash
# === NEW: Resize Event Analysis ===
echo "=== Resize Event Analysis ==="
grep "PANE_RESIZE" /tmp/smux_resize.log | wc -l
echo ""
echo "Resize timeline:"
grep "PANE_RESIZE" /tmp/smux_resize.log | head -20
echo ""
echo "Resize deltas (Y-axis changes):"
grep "PANE_RESIZE" /tmp/smux_resize.log | awk '{print $7}' | sort | uniq -c

# Correlate resize events with scroll bursts
echo ""
echo "=== Correlation: Resize → Scroll Bursts ==="
# Extract first resize timestamp
FIRST_RESIZE_TS=$(grep "PANE_RESIZE" /tmp/smux_resize.log | head -1 | sed 's/.*\[\([0-9.]*\)\].*/\1/')
# Extract first scroll after that resize
grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | \
    awk -v ts="$FIRST_RESIZE_TS" '$0 ~ ts {print; for(i=1;i<=10;i++) {getline; print}}'

# Time distribution analysis
echo "=== First 10 and Last 10 Scroll Triggers ==="
grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | head -10
echo "..."
grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | tail -10

# Calculate scroll rate
START_TIME=$(grep "SCROLL_TRIGGER#1" /tmp/smux_scroll_trigger.log | \
    sed 's/.*\\[\\([0-9]*\\)\\..*/ \\1/')
END_TIME=$(grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | tail -1 | \
    sed 's/.*\\[\\([0-9]*\\)\\..*/ \\1/')
TOTAL_SCROLLS=$(grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | wc -l)
DURATION=$((END_TIME - START_TIME))
echo "Duration: ${DURATION}s, Total Scrolls: $TOTAL_SCROLLS"
echo "Scroll Rate: $((TOTAL_SCROLLS / DURATION)) scrolls/second"

# Extract time deltas between consecutive scrolls
grep "SCROLL_TRIGGER" /tmp/smux_scroll_trigger.log | \
    awk -F'[\\[\\]]' '{
        if (prev != "") {
            delta = $2 - prev;
            if (delta < 0.001) print "SUB-MILLISECOND:", delta, "seconds";
        }
        prev = $2;
    }' | head -20

# Sample raw input chunks
echo "=== Sample PTY Raw Input Chunks ==="
grep -A 3 "PTY_READ" /tmp/smux_pty_raw.log | head -30
```

---

## 🎯 Expected Findings

Based on the previous detection (681,220 scrolls in 137 seconds):

- **Scroll rate**: ~4,972 scrolls/second
- **99.8% of scrolls**: < 1ms apart
- **Hypothesis**: Claude Code is sending data in a way that causes rapid scroll triggers

### What We're Looking For

1. **Burst Pattern**: Does Claude Code send output in massive bursts?
2. **Newline Pattern**: Are there excessive newlines? (`\n\n\n...`)
3. **Control Sequences**: Are ANSI codes causing cursor movements? (`\033[...`)
4. **Redraw Pattern**: Is Claude Code redrawing the same content repeatedly?
5. **Streaming Strategy**: How does Claude Code stream responses? (character-by-character? line-by-line? chunks?)

### Root Cause Candidates

1. **Excessive Linefeeds**: Claude Code sending too many `\n` characters
2. **Redraw on Every Char**: Clearing and redrawing on each character streamed
3. **ANSI Escape Abuse**: Using cursor movement instead of proper scrolling
4. **Buffer Flushing**: Flushing on every character causing many small writes
5. **Terminal State Confusion**: Cursor position management issues

---

## 📦 Files Modified

This instrumentation required changes to:

1. **window.c** (lines 1040-1087): PTY raw input logging in `window_pane_read_callback()`
2. **window.c** (lines 1125-1146): **NEW** Window resize event logging in `window_pane_resize()`
3. **input.c** (lines 1289-1304): Linefeed counting in `input_c0_dispatch()`
4. **screen-write.c** (lines 1451-1468): Scroll trigger logging in `screen_write_linefeed()`
5. **grid.c** (lines 419-430): Grid scroll history logging (previous session)

---

## 🔧 Next Steps After Analysis

Once we understand WHY the terminal is resizing:

1. **If Claude Code is sending resize signals**:
   - Report to Anthropic as a bug
   - Implement resize signal filtering/throttling in smux
   - Check for ANSI window size control sequences in PTY raw logs

2. **If smux is auto-resizing due to content overflow**:
   - Implement intelligent resize throttling
   - Add option to disable auto-resize during rapid output
   - Implement resize debouncing (wait for output to stabilize)

3. **If resize is triggering scroll cascades**:
   - Batch scroll operations during resize
   - Defer screen reorganization until resize completes
   - Implement scroll coalescing ONLY during resize events

4. **If it's unavoidable behavior**:
   - Document the correlation between resize and scrolling
   - Implement resize-aware scroll buffering
   - Add user-configurable resize throttling option

---

## 🎓 Understanding the Problem

**The Goal**: Don't just "fix" the symptom with a rate limiter. Understand the ROOT CAUSE.

**The User's Words**:
> "we should honestly in our scrolling, we need to get better debug logs over what is claude sending over to the terminal which is causing this instead of some stupid solution like the one just offered"

This instrumentation gives us:
- **Visibility**: See exactly what data flows through the system
- **Precision**: Microsecond timestamps on every event
- **Context**: Understand the transformation at each layer
- **Intelligence**: Make informed decisions based on real data

**Now we can build an INTELLIGENT solution, not a band-aid.**

---

## ✅ System Ready - Phase 2: RESIZE INVESTIGATION

The instrumented smux binary (with resize logging) is:
- ✅ Built with FOUR diagnostic layers (added Layer 4: resize events)
- ✅ Installed to `/usr/local/bin/smux`
- ✅ Old processes killed
- ✅ Debug logs cleared
- ✅ Ready for Phase 2 testing

**Phase 1 Findings** (via ceregrep):
- Terminal resizing during output: sy: 19→20→24→27
- 4,000-6,700 scrolls/second confirmed
- Cursor always at bottom (cy == rlower)

**Phase 2 Goal**:
- Capture resize events in `/tmp/smux_resize.log`
- Correlate resize timing with scroll bursts
- Identify WHAT triggers the terminal dimension changes

**Start your Phase 2 test run whenever you're ready!**
