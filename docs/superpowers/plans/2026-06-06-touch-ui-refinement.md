# Touch UI Refinement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Refine the valve board UI for touch-screen use by widening scrollbars, restoring a two-column channel layout, and changing valve indicators to circular lights with labels below.

**Architecture:** Keep the existing controller and protocol flow unchanged. Limit changes to the Qt UI layer: theme stylesheet for touch sizing, `MainWindow` layout for channel presentation, and valve indicator widgets for the new circular visual treatment.

**Tech Stack:** C++17, Qt Widgets, CMake, MSVC/Qt 5.14 cross-platform build

---

### Task 1: Restore Two-Column Channel Layout

**Files:**
- Modify: `src/ui/MainWindow.cpp`

- [ ] Update the channel container from a single-column `QVBoxLayout` back to a `QGridLayout` with two cards per row.
- [ ] Keep the outer `QScrollArea` so the whole channel region still scrolls vertically when all 8 channels are visible.
- [ ] Preserve the existing per-card visible valve count propagation and click-to-command behavior.

### Task 2: Improve Touch Scrolling

**Files:**
- Modify: `src/app/ApplicationTheme.cpp`

- [ ] Increase vertical and horizontal scrollbar dimensions in the application stylesheet.
- [ ] Increase scrollbar handle minimum size so both the page-level and per-card scroll areas are usable on touch screens.

### Task 3: Convert Valve Indicators to Circular Lights

**Files:**
- Modify: `src/ui/ValveIndicatorWidget.cpp`
- Modify: `src/ui/ValveIndicatorWidget.h`
- Modify: `src/ui/ValveGridWidget.cpp`
- Modify: `src/ui/ChannelCardWidget.cpp`

- [ ] Change the indicator paint routine from rounded rectangular button style to a circular light with the valve number rendered below the light.
- [ ] Keep the ack-highlight behavior, with confirmed valve actions shown in green.
- [ ] Reduce the widget footprint from the previous large rectangular control to a compact touch-friendly size.
- [ ] Tune grid spacing and card sizing so two-column cards remain readable.

### Task 4: Verification

**Files:**
- Verify: `build-vs`

- [ ] Run `cmake --build build-vs --target valve_test_tool --config Debug`.
- [ ] Confirm the executable is produced without compilation errors.
