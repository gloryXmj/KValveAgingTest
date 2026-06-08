# Touch Layout Tuning Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Tune the Qt UI for touch-screen usability by widening scrollbars, restoring two-column channel cards, and changing valve indicators to circular LEDs with labels below.

**Architecture:** Keep the existing controller and protocol flow unchanged and limit changes to the UI layout, widget painting, and application stylesheet. Persist the visible-valve count setting as-is and reuse the current ack-driven pulse behavior while making the indicator state easier to read.

**Tech Stack:** C++17, Qt Widgets, CMake, custom-painted QWidget indicators

---

### Task 1: Rework the Channel Area Layout

**Files:**
- Modify: `src/ui/MainWindow.cpp`
- Modify: `src/ui/ChannelCardWidget.cpp`

- [ ] Switch the channel card container back to a two-column `QGridLayout`.
- [ ] Keep the outer channel area inside a vertically scrollable `QScrollArea`.
- [ ] Preserve each channel card's internal scroll area for valve indicators.
- [ ] Reduce channel card height from the previous single-column layout so two columns remain practical on the page.

### Task 2: Make Scrollbars Touch-Friendly

**Files:**
- Modify: `src/app/ApplicationTheme.cpp`

- [ ] Increase vertical and horizontal scrollbar thickness.
- [ ] Increase scrollbar handle size and make the track visibly touchable.
- [ ] Apply the same scrollbar styling to the page-level and card-level scroll areas through the shared stylesheet.

### Task 3: Redesign Valve Indicators as Circular LEDs

**Files:**
- Modify: `src/ui/ValveIndicatorWidget.h`
- Modify: `src/ui/ValveIndicatorWidget.cpp`
- Modify: `src/ui/ValveGridWidget.cpp`

- [ ] Change the indicator painting from rounded rectangles to circular LEDs.
- [ ] Move the valve number from the center of the indicator to a label area below the circle.
- [ ] Keep neutral coloring in the idle state and green fill on ack pulse.
- [ ] Reduce the overall footprint so the two-column channel layout remains balanced.
- [ ] Slightly increase pulse visibility time so ack feedback is easier to notice on the touch display.

### Task 4: Build Verification

**Files:**
- Verify: `build-vs/Debug/valve_test_tool.exe`

- [ ] Run `cmake --build build-vs --target valve_test_tool --config Debug`.
- [ ] Confirm the build completes successfully with the updated UI changes.
