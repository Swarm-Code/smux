# SMUX Startup Hang Analysis

## Problem Description
After recent format string changes in window-tree.c (commit cd04a79a), smux hangs during startup when executing `new-session` or `new-project` commands.

## Format String Change
**Original (working):**
```
"#[fg=#E6A95E,bold]SESSION #[fg=#BFBDB6]#[bold]🖥️  #{session_name}#[nobold] "
```

**New (potentially hanging):**
```
"#[fg=#E6A95E,bold]SESSION #[default]#[fg=#BFBDB6]🖥️  #{session_name} "
```

## Key Differences

### 1. `#[nobold]` Processing
- Located in `style.c:219-224`
- Calls `attributes_fromstring("bold")` → returns `GRID_ATTR_BRIGHT`
- Clears ONLY the bold/bright attribute: `sy->gc.attr &= ~value`
- Does NOT modify colors (fg, bg, us)
- Does NOT modify other attributes

### 2. `#[default]` Processing
- Located in `style.c:87-92`
- Resets ALL attributes to the base parameter:
  - `sy->gc.fg = base->fg`
  - `sy->gc.bg = base->bg`
  - `sy->gc.us = base->us`
  - `sy->gc.attr = base->attr`
  - `sy->gc.flags = base->flags`
- Requires `base` parameter to be valid (non-NULL)

## Potential Hang Scenarios

### Scenario A: NULL Base Parameter
If `base` parameter is NULL when `style_parse()` is called with `#[default]`, dereferencing would cause segfault, not hang.

**Evidence:** In `format-draw.c:826`, `style_parse(&sy, &current_default, tmp)` always passes initialized `current_default`.

**Conclusion:** Unlikely to be the issue.

### Scenario B: Infinite Recursion in Format Expansion
If the format string parsing triggers recursive expansion that never terminates.

**Check:** Look for circular dependencies in format variables used in SESSION format.

**Variables in format:**
- `#{session_name}` - Simple variable, no recursion
- `#{session_windows}` - Counter, no recursion
- `#{session_group}` - String, no recursion
- `#{session_group_list}` - List, no recursion
- `#{session_attached}` - Boolean, no recursion

**Conclusion:** No obvious recursion in format variables.

### Scenario C: Style State Machine Issue
The sequence `#[fg=#E6A95E,bold]SESSION #[default]#[fg=#BFBDB6]` might create an unexpected state.

**Analysis:**
1. `#[fg=#E6A95E,bold]` - Sets fg to orange, attr to BRIGHT
2. `#[default]` - Resets to base (likely default colors, no attrs)
3. `#[fg=#BFBDB6]` - Sets fg to light gray

vs original:

1. `#[fg=#E6A95E,bold]SESSION ` - Sets fg to orange, attr to BRIGHT
2. `#[fg=#BFBDB6]` - Sets fg to light gray, attr STILL BRIGHT
3. `#[bold]` - Redundant, already set
4. `#[nobold]` - Clears BRIGHT attribute

**Observation:** The new version has `#[default]` immediately followed by another style change. This might trigger a bug in the style state management.

### Scenario D: Performance Issue with Repeated Style Changes
The new format has 3 style changes in quick succession:
- `#[fg=#E6A95E,bold]`
- `#[default]`
- `#[fg=#BFBDB6]`

If there's quadratic complexity or inefficiency in style application, this could slow down rendering significantly.

### Scenario E: Project Format vs Session Format
Looking at line 62 in window-tree.c, the PROJECT format ALSO uses `#[default]`:
```
"\n#[fg=#95E6CB,bold]PROJECT 📂 #{project_name}#[default]#[fg=#BFBDB6] - #{project_sessions} sessions #[fg=#565B66](#{t:project_created})#[default]\n"
```

This format has MULTIPLE `#[default]` tags. If there's an issue with default state tracking, this could compound the problem.

## Testing Results

### Test 1: Simple new-session
**Command:** `smux new-session -d -s test`
**Result:** ✓ No hang (completes in <1 second)

### Test 2: new-project
**Command:** `smux new-project -d testproject`
**Result:** ✓ No hang (completes in <1 second)

### Test 3: choose-tree
**Command:** `smux choose-tree`
**Result:** ✓ No hang

## Current Status
**NO HANG DETECTED** in current testing environment.

## Possible Explanations

1. **The hang may be intermittent** - dependent on terminal state or existing sessions
2. **The hang may occur only during server startup** - first format parsing
3. **The hang may be fixed** - if recent commits addressed it
4. **The hang may require specific conditions** - certain number of projects/sessions

## Next Steps

### Step 1: Test with Multiple Sessions/Projects
Create environment with 10+ projects and 20+ sessions to see if complexity triggers hang.

### Step 2: Add Debug Logging
Add extensive logging to:
- `format_draw()` entry/exit
- `style_parse()` for each `#[default]` call
- Base parameter values in style_parse

### Step 3: Compare with Original Version
Check out commit before cd04a79a and verify it doesn't hang, then binary search for the exact commit that introduces hang.

### Step 4: Check format_expand Recursion Depth
Add recursion depth counter to detect if format expansion is going too deep.

### Step 5: Profile Performance
Use perf or gprof to identify where time is being spent during startup.

## Code Locations

### Format String Definition
- File: `window-tree.c`
- Lines: 45-72 (`WINDOW_TREE_DEFAULT_FORMAT`)
- Line 65: SESSION format with `#[default]`

### Style Parsing
- File: `style.c`
- Function: `style_parse()`
- Lines: 87-92: `#[default]` handling
- Lines: 219-224: `#[no*]` attribute handling

### Format Drawing
- File: `format-draw.c`
- Function: `format_draw()`
- Line 826: `style_parse()` call
- Lines 732-733: `current_default` initialization

### Attributes
- File: `attributes.c`
- Function: `attributes_fromstring()`
- Line 68: `bold` → `GRID_ATTR_BRIGHT` mapping

## Debugging Tools Created

1. **quick-debug-hang.sh** - Fast hang detection script
2. **debug-startup-hang.sh** - Comprehensive debugging with strace/gdb
3. **debug-format-parsing.gdb** - GDB script with breakpoints on format functions
4. **test-project-hang.sh** - Tests various commands for hangs

## Recommendations

### Short-term Fix
If hang is confirmed, revert the change on line 65 of window-tree.c:
```diff
-		"#[fg=#E6A95E,bold]SESSION #[default]#[fg=#BFBDB6]🖥️  #{session_name} " \
+		"#[fg=#E6A95E,bold]SESSION #[fg=#BFBDB6]#[bold]🖥️  #{session_name}#[nobold] " \
```

### Better Fix
If the goal is to avoid the redundant `#[bold]` before the emoji:
```
"#[fg=#E6A95E,bold]SESSION #[nobold]#[fg=#BFBDB6]🖥️  #{session_name} " \
```
This clears bold BEFORE changing color, avoiding the full reset of `#[default]`.

### Root Cause Fix
Investigate why `#[default]` causes issues and fix the underlying bug in:
- Style state management
- Format expansion recursion
- Performance of repeated style changes
