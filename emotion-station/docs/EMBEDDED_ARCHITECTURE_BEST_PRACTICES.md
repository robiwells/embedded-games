# Embedded Systems Architecture Best Practices

> A comprehensive guide to professional embedded firmware patterns

**Version:** 1.0
**Date:** February 2026
**Author:** Industry best practices compilation

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Layered Service Architecture](#2-layered-service-architecture)
3. [Hierarchical State Machines](#3-hierarchical-state-machines)
4. [Event-Driven Architecture](#4-event-driven-architecture)
5. [Hardware Abstraction Layer (HAL)](#5-hardware-abstraction-layer-hal)
6. [Testing Strategies](#6-testing-strategies)
7. [Static Allocation Patterns](#7-static-allocation-patterns)
8. [Error Handling](#8-error-handling)
9. [Code Quality](#9-code-quality)
10. [Build Systems](#10-build-systems)
11. [When to Adopt These Patterns](#11-when-to-adopt-these-patterns)
12. [Summary & Further Reading](#12-summary--further-reading)

---

## 1. Introduction

### Overview

Building embedded systems that scale from prototype to production requires more than functional code—it demands architecture. The patterns documented here come from production-grade firmware frameworks powering commercial embedded devices. These patterns have been proven in systems that have successfully shipped to thousands of customers, demonstrating real-world viability.

### Why Patterns Matter

**Prototype vs Production:**
- **Prototypes** prioritise speed: monolithic files, hard-coded values, minimal abstraction
- **Production firmware** prioritises maintainability: modularity, testability, scalability

The gap between these two worlds is bridged by architectural patterns. Without them:
- ❌ Adding features becomes progressively harder (tight coupling)
- ❌ Bugs reappear after fixes (no regression tests)
- ❌ New team members struggle (unclear structure)
- ❌ Porting to new hardware requires rewriting (hardware dependencies scattered)

With professional patterns:
- ✅ Features plug in cleanly (layered architecture, event bus)
- ✅ Regressions caught early (unit tests with fakes)
- ✅ Onboarding is fast (clear module boundaries, documentation)
- ✅ Porting is straightforward (HAL abstraction)

### When to Adopt

| Project Size | Team Size | Lifetime | Recommended Patterns |
|--------------|-----------|----------|----------------------|
| Small (<3KLOC) | 1 developer | <6 months | Enter/exit/update FSM, Basic HAL |
| Medium (3-10KLOC) | 2-3 developers | 6-24 months | + Layered services, Native testing |
| Large (>10KLOC) | 3+ developers | >24 months | + Event bus, CI/CD, Full patterns |

**Critical decision point:** If your project might live beyond its initial purpose (reused, maintained, extended), adopt professional patterns early. Refactoring a 10KLOC prototype is harder than building it correctly from the start.

### How to Use This Document

This is a **reference guide**, not a tutorial. Each section:
1. Explains **what** the pattern is and **why** it exists
2. Shows **how** production systems implement it (code examples)
3. Lists **benefits and trade-offs**
4. Guides **when to adopt** (small/medium/large projects)

You don't need to adopt all patterns. Start with what your project needs now, and add more as complexity grows.

---

## 2. Layered Service Architecture

### 2.1 Pattern Description

**What it is:** Organising firmware into hierarchical layers—base, middleware, and application—with strict dependency rules.

**Why it exists:** Flat file structures work for prototypes but become unmaintainable as projects grow. Layered architecture enforces separation of concerns:
- **Base layer**: Foundation services (FSM, event bus, logger) with no dependencies
- **Middleware layer**: Hardware abstractions (NFC, audio, storage) depending only on base
- **App layer**: Business logic (game state machine, UI) depending on any layer

This structure makes dependencies explicit, enables parallel development, and improves testability.

**When to use:**
- Projects >10KLOC: **Highly recommended**
- Projects 3-10KLOC: **Recommended**
- Projects <3KLOC: **Optional** (flat structure acceptable)

### 2.2 Production Implementation Example

**Directory structure:**
```
firmware/
├── services/
│   ├── base/              # Foundation (no dependencies except HAL)
│   │   ├── fsm/            # Finite state machine framework
│   │   ├── event_bus/      # Event routing
│   │   ├── event_service/  # Event service
│   │   ├── fs_mgr/         # Filesystem manager
│   │   ├── http_client/    # HTTP client
│   │   └── utils/                # Common utilities
│   ├── middleware/        # Hardware abstraction (depends on base)
│   │   ├── view_mgr/       # UI view management (LVGL)
│   │   ├── audio_source/   # Audio streaming
│   │   ├── alarms/         # Alarm functionality
│   │   ├── wifi_setup/     # WiFi configuration
│   │   └── ble_setup/      # Bluetooth setup
│   └── app/              # Application logic (depends on any layer)
│       ├── pwr_mgr/       # Power management
│       └── ux_mgr/        # User experience management
├── hal/                   # Hardware abstraction layer interfaces
│   └── include/hal/
│       ├── platform_i2c.h
│       ├── platform_rtc.h
│       ├── platform_gpio.h
│       └── nvs.h
└── ports/                 # Platform-specific implementations
    └── native/            # Native (simulator) port
        └── hal/
```

### 2.3 Naming Conventions

**Common pattern:** `<layer>_<component>` or `<component>` (layer implied by directory)
- `<layer>` = `base`, `middleware`, `app` (optional prefix)
- `<component>` = descriptive name (e.g., `fsm`, `audio`, `pwr_mgr`)

**Examples:**
- `fsm` or `base_fsm` - Base service: finite state machine
- `audio_source` or `mw_audio` - Middleware service: audio streaming
- `pwr_mgr` or `app_pwr` - Application service: power management

**Benefits:**
- ✅ **Searchability:** Easy to find all services in a layer
- ✅ **Layer visibility:** Name or directory reveals layer
- ✅ **Collision avoidance:** Prefixes prevent conflicts with third-party libraries

**Alternative patterns:**
- Directory structure only (no prefixes): Cleaner but requires IDE navigation
- C++ namespaces: Ideal for C++ projects
- Module-specific prefixes: `fsm_`, `audio_`, etc. (less structured)

### 2.4 Module Structure Per Service

Each service follows a consistent structure:

```
fsm/
├── CMakeLists.txt         # Build configuration
├── include/
│   └── fsm/
│       └── fsm.h   # Public API (installed to system include path)
├── src/
│   └── fsm.c       # Implementation (private)
├── tests/
│   ├── test_fsm.cpp      # Unit tests (CppUTest)
│   ├── common/           # Shared test utilities
│   └── assets/           # Test data files
└── examples/
    ├── CMakeLists.txt
    └── fsm_demo.c        # Usage example
```

**Key principles:**
- **Public API in include/:** Only public headers installed, implementation stays private
- **Tests alongside code:** Easy to find, encourages test-driven development
- **Examples as documentation:** Executable documentation showing usage

### 2.5 Dependency Rules

Layers enforce strict dependency constraints:

```
┌─────────────────────┐
│   App Layer         │  Can depend on: middleware + base + HAL
│  (app_*)         │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│  Middleware Layer   │  Can depend on: base + HAL
│  (hw_*)          │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│  Base Layer         │  Can depend on: HAL only
│  (fsm_*)          │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│  HAL                │  Platform-specific (ESP32, STM32, native)
│  (hal)         │
└─────────────────────┘
```

**Enforced by:**
- Include paths (CMake controls visibility)
- Linker order (base links before middleware)
- Code review (manual check for violations)

**Example violation:**
```c
// ❌ BAD: Base service depending on middleware
// File: services/base/fsm_logger/src/fsm_logger.c
#include "hw_audio/hw_audio.h"  // VIOLATION: base → middleware

// ✅ GOOD: Base service depending only on HAL
// File: services/base/fsm_logger/src/fsm_logger.c
#include "hal/log.h"  // OK: base → HAL
```

### 2.6 Benefits & Trade-offs

**Benefits:**
- ✅ **Clear boundaries:** Each layer has explicit responsibilities
- ✅ **Parallel development:** Multiple developers work on different layers without conflicts
- ✅ **Reusability:** Base services portable across products (FSM, event bus)
- ✅ **Testability:** Mock layer dependencies (test app without middleware)
- ✅ **Comprehensibility:** New developers follow layers to understand architecture

**Trade-offs:**
- ❌ **More directories/files:** Overhead for small projects (<3KLOC)
- ❌ **Layer transition overhead:** Function calls cross layer boundaries
- ❌ **Risk of violations:** Must enforce via review (no compile-time checks in C)
- ❌ **Initial investment:** Requires upfront design of layer boundaries

### 2.7 Applicability

**Large projects (>10KLOC, 3+ developers):**
- **Highly recommended:** Layered architecture essential for team coordination
- Without it, codebase becomes unmaintainable ("big ball of mud")

**Medium projects (3-10KLOC, 2-3 developers):**
- **Recommended:** Benefits outweigh overhead
- Start simple (2 layers: app + base), split middleware as needed

**Small projects (<3KLOC, 1 developer):**
- **Optional:** Flat structure with good naming may suffice
- Consider if project might grow or be reused

**Real-world example:** A production embedded framework started as a monolithic player app. As features grew (BLE, WiFi, alarms, content management), the team refactored into layered services. This enabled:
- Reusing base services across multiple the products
- Onboarding new developers faster (clear structure)
- Testing services in isolation (unit tests with mocks)

---

## 3. Hierarchical State Machines

### 3.1 Pattern Description

**What it is:** A state machine framework with:
- **Hierarchy:** States can have parent states (inheritance of behaviour)
- **Lifecycle:** Each state has entry/run/exit functions
- **Table-driven dispatch:** Function pointers enable clean state transitions

**Why it exists:**
- Simple switch-based state machines work for 2-4 states
- Beyond that, code becomes hard to maintain:
  - Duplicated initialisation/cleanup logic
  - Missed state transitions
  - Difficult to add sub-states

Hierarchical FSMs solve this by:
- Reusing parent state logic (enter/exit) for child states
- Centralising transition logic (prevents missed init/cleanup)
- Scaling cleanly to complex state hierarchies

**Comparison to alternatives:**
| Approach | Best For | Pros | Cons |
|----------|----------|------|------|
| Switch statement | 2-4 simple states | Easy to understand | Duplicated logic, no hierarchy |
| Flat FSM table | 5-10 flat states | Clean dispatch | No hierarchy, manual init/cleanup |
| Hierarchical FSM | >10 states or nested behaviours | Hierarchy, lifecycle | More complex setup |

**When to use:**
- Complex state machines (>5 states): **Highly recommended**
- Nested behaviours (e.g., "playing" state with sub-states "loading", "playing", "paused"): **Required**
- Simple state machines (2-4 states): **Optional** (switch may be clearer)

### 3.2 Production Implementation Example

**Core data structures:**

```c
// File: services/base/fsm/include/fsm.h

typedef struct fsm_state {
    const char* name;                      // For debugging (e.g., "STATE_IDLE")
    void (*entry)(void* ctx);              // Called once when entering state
    void (*run)(void* ctx);                // Called periodically (or manually)
    void (*exit)(void* ctx);               // Called once when leaving state
    struct fsm_state* parent;       // Parent state (NULL if root)
    uint32_t run_period_ms;               // Run frequency (0 = manual, >0 = timer-based)
    uint8_t id;                           // Unique state ID
} fsm_state_t;

typedef struct {
    fsm_state_t* current_state;     // Currently active state
    fsm_state_t* states;            // Array of all states
    uint8_t num_states;                   // Total number of states
    uint32_t state_entry_time;            // millis() at last transition
    void* user_ctx;                       // Optional user data pointer
} fsm_ctx_t;
```

**Key functions:**

```c
// Initialise FSM and enter initial state
status_t fsm_init(fsm_ctx_t* ctx, fsm_state_t* initial_state);

// Transition to new state (calls exit → update → entry)
status_t fsm_transition(fsm_ctx_t* ctx, fsm_state_t* new_state);

// Run current state's run() function (call from main loop)
status_t fsm_run(fsm_ctx_t* ctx);

// Get current state
fsm_state_t* fsm_get_current_state(const fsm_ctx_t* ctx);
```

### 3.3 Enter/Run/Exit Lifecycle

Each state has three lifecycle phases:

**1. Entry (init code):**
- Called **once** when entering the state
- Use for: Starting animations, resetting timers, logging transitions, initialising variables
- Example:
  ```c
  void state_playing_entry(void* ctx) {
      game_ctx_t* game = (game_ctx_t*)ctx;
      led_set_animation(LED_BREATHING);      // Start LED animation
      audio_play(game->selected_track);      // Start audio playback
      game->activity_start_time = HAL_MILLIS();
      LOG_INFO("GAME", "Started activity: %s", game->selected_track);
  }
  ```

**2. Run (per-frame logic):**
- Called **periodically** (either manually or via timer)
- Use for: Checking conditions, updating displays, processing inputs, timeout logic
- Example:
  ```c
  void state_playing_run(void* ctx) {
      game_ctx_t* game = (game_ctx_t*)ctx;

      // Check for timeout (5 minutes max)
      if (HAL_MILLIS() - game->activity_start_time > 300000) {
          fsm_transition(&game->fsm, &state_complete);
      }

      // Update LED animation
      led_update();
  }
  ```

**3. Exit (cleanup code):**
- Called **once** when leaving the state
- Use for: Stopping animations, saving state, clearing flags, logging exit
- Example:
  ```c
  void state_playing_exit(void* ctx) {
      game_ctx_t* game = (game_ctx_t*)ctx;
      audio_stop();                           // Stop playback
      led_set_animation(LED_IDLE);            // Reset LEDs
      game->last_activity_duration = HAL_MILLIS() - game->activity_start_time;
      LOG_INFO("GAME", "Activity completed after %d ms", game->last_activity_duration);
  }
  ```

**Why this matters:**
- ✅ **No missed cleanup:** Exit guaranteed to run on every transition
- ✅ **No duplicate init:** Entry runs once, not on every loop iteration
- ✅ **Clear separation:** Init, update, cleanup logic in separate functions

### 3.4 Hierarchical Transitions

**Example hierarchy:**
```
ROOT
 ├─ STATE_IDLE (parent: ROOT)
 ├─ STATE_PLAYING (parent: ROOT)
 │   ├─ STATE_PLAYING_LOADING (parent: STATE_PLAYING)
 │   ├─ STATE_PLAYING_ACTIVE (parent: STATE_PLAYING)
 │   └─ STATE_PLAYING_PAUSED (parent: STATE_PLAYING)
 └─ STATE_COMPLETE (parent: ROOT)
```

**Transition algorithm:**
When transitioning from `STATE_PLAYING_ACTIVE` to `STATE_COMPLETE`:

1. **Exit up to common ancestor:**
   - Call `STATE_PLAYING_ACTIVE->exit()`
   - Call `STATE_PLAYING->exit()`
   - Stop (ROOT is common ancestor)

2. **Enter down to target:**
   - Call `STATE_COMPLETE->entry()`

**When transitioning from `STATE_PLAYING_ACTIVE` to `STATE_PLAYING_PAUSED`:**

1. **Exit up to common ancestor:**
   - Call `STATE_PLAYING_ACTIVE->exit()`
   - Stop (STATE_PLAYING is common ancestor)

2. **Enter down to target:**
   - Call `STATE_PLAYING_PAUSED->entry()`
   - Note: `STATE_PLAYING->entry()` NOT called (already in parent state)

**Implementation (simplified):**
```c
status_t fsm_transition(fsm_ctx_t* ctx, fsm_state_t* new_state) {
    if (!ctx || !new_state) return STATUS_ERROR_NULL_POINTER;

    fsm_state_t* old_state = ctx->current_state;

    // Find common ancestor
    fsm_state_t* common_ancestor = find_common_ancestor(old_state, new_state);

    // Exit up to common ancestor
    fsm_state_t* state = old_state;
    while (state && state != common_ancestor) {
        if (state->exit) state->exit(ctx->user_ctx);
        state = state->parent;
    }

    // Enter down to target (recursive)
    enter_state_hierarchy(ctx, common_ancestor, new_state);

    // Update context
    ctx->current_state = new_state;
    ctx->state_entry_time = HAL_MILLIS();

    LOG_INFO("FSM", "Transitioned: %s → %s", old_state->name, new_state->name);
    return STATUS_OK;
}
```

### 3.5 CREATE_CTX Pattern (Static Allocation)

A production embedded framework uses macros to enable compile-time (static) allocation of FSM contexts, avoiding malloc/free:

```c
// Macro definition
#define SERVICE_BS_FSM_CREATE_CTX(name, num_states) \
    static fsm_state_t name##_states[num_states]; \
    static fsm_ctx_t name = { \
        .states = name##_states, \
        .num_states = num_states, \
    }

// Usage example
SERVICE_BS_FSM_CREATE_CTX(game_fsm, 8);  // Creates static game_fsm context with 8 states

// Equivalent to:
// static fsm_state_t game_fsm_states[8];
// static fsm_ctx_t game_fsm = {
//     .states = game_fsm_states,
//     .num_states = 8,
// };
```

**Benefits:**
- ✅ **Zero runtime allocation:** No malloc/free, deterministic memory usage
- ✅ **Compile-time sizing:** Memory usage known at compile time
- ✅ **Clean syntax:** Macro hides boilerplate

**See [Section 7: Static Allocation](#7-static-allocation-patterns) for more details.**

### 3.6 FreeRTOS Integration

A production embedded framework integrates the FSM with FreeRTOS for automatic periodic execution:

**Run timer (automatic periodic run):**
```c
// State with 100ms run period
fsm_state_t state_active = {
    .name = "STATE_ACTIVE",
    .entry = state_active_entry,
    .run = state_active_run,      // Called every 100ms automatically
    .exit = state_active_exit,
    .parent = NULL,
    .run_period_ms = 100,         // FreeRTOS timer triggers run every 100ms
    .id = 1,
};

// State with manual run (polling)
fsm_state_t state_idle = {
    .name = "STATE_IDLE",
    .entry = state_idle_entry,
    .run = state_idle_run,        // Must be called manually from main loop
    .exit = state_idle_exit,
    .parent = NULL,
    .run_period_ms = 0,           // 0 = manual mode
    .id = 0,
};
```

**Task notifications on transitions:**
```c
// Register for state change notifications
TaskHandle_t task_handle = xTaskGetCurrentTaskHandle();
fsm_register_notification(&game_fsm, task_handle, BIT(0));

// In task loop:
void game_task(void* param) {
    while (1) {
        // Wait for state transition notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Handle state change
        fsm_state_t* current = fsm_get_current_state(&game_fsm);
        LOG_INFO("TASK", "State changed to: %s", current->name);
    }
}
```

**Benefits:**
- ✅ **Offloads timing:** FreeRTOS handles periodic run execution
- ✅ **Thread coordination:** Task notifications wake tasks on transitions
- ✅ **Priority management:** FreeRTOS scheduler handles priorities

**Note:** For Arduino/cooperative multitasking, use manual mode (`run_period_ms = 0`) and call `fsm_run()` from main loop.

### 3.7 Benefits & Trade-offs

**Benefits:**
- ✅ **Clear lifecycle:** Entry/exit prevent missed init/cleanup
- ✅ **Hierarchical states:** Reduce code duplication via parent states
- ✅ **Centralised transitions:** Single function handles all transitions, enables logging/debugging
- ✅ **Table-driven dispatch:** Function pointers cleaner than large switch statements
- ✅ **Scalable:** Easily add new states without refactoring existing code

**Trade-offs:**
- ❌ **Complexity overhead:** More complex than switch statement for simple FSMs (2-4 states)
- ❌ **Cognitive load:** Hierarchy adds mental model complexity (when does parent exit run?)
- ❌ **Function pointer overhead:** Slight performance cost (mitigated by -O2 optimisation, compiler often inlines)
- ❌ **Debugging difficulty:** Call stack has extra indirection (function pointers)

**Performance note:** Compiler optimisation typically inlines function pointer calls, making overhead negligible. A production embedded framework shows **zero measurable overhead** with -O2 on ARM Cortex-M4.

### 3.8 Applicability

**When to use hierarchical FSM:**

**Highly recommended:**
- State machines with >5 states
- Nested behaviours (parent-child state relationships)
- Complex lifecycle requirements (init/cleanup critical)
- State machines that will grow over time

**Optional:**
- Simple state machines (2-4 states): Switch statement may be clearer
- Performance-critical code (measure first—overhead usually negligible)

**Real-world example:** this system has a main FSM with 15+ states (idle, playing, paused, loading, error, etc.) and sub-state machines for audio (buffering, decoding, playing) and UI (splash, menu, settings). Hierarchical FSM enables clean composition:
```
STATE_PLAYING (parent)
 ├─ STATE_PLAYING_LOADING (child: show loading animation)
 ├─ STATE_PLAYING_ACTIVE (child: play audio, update progress)
 └─ STATE_PLAYING_PAUSED (child: pause audio, show pause icon)
```

---

## 4. Event-Driven Architecture

### 4.1 Pattern Description

**What it is:** A publish/subscribe messaging system (event bus) that decouples components. Publishers emit events without knowing who receives them; subscribers register for events without knowing who sends them.

**Why it exists:**
- Direct function calls couple components: `audio_play()` might call `ui_update()`, `logger_log()`, etc.
- This coupling makes testing hard (can't test audio without UI) and changes fragile (modifying audio API breaks UI)
- Event buses decouple: Audio publishes "AUDIO_STARTED" event; UI/logger subscribe independently

**Comparison to alternatives:**

| Approach | Coupling | Testability | Scalability |
|----------|----------|-------------|-------------|
| Direct function calls | High | Low | Poor |
| Callbacks | Medium | Medium | Medium |
| Event bus | Low | High | Excellent |

**When to use:**
- Multi-component systems: **Highly recommended**
- Systems with asynchronous events (hardware interrupts, timers): **Highly recommended**
- Simple single-threaded systems: **Optional** (direct calls may be simpler)

### 4.2 Production Implementation Example

**Event structure (type-based routing):**

```c
// File: services/base/event_bus/include/event_bus.h

// Event type (32 types supported via bit flags)
typedef enum {
    AUDIO_EVENT = BIT(0),      // Audio playback events
    NFC_EVENT = BIT(1),        // NFC tag detection
    BATTERY_EVENT = BIT(2),    // Battery status changes
    UI_EVENT = BIT(3),         // UI interactions
    // ... up to BIT(31)
} event_type_t;

// Event structure (up to 2048 bytes payload)
typedef struct {
    event_type_t type;         // Event type (routing key)
    uint32_t id;               // Event ID within type (31 IDs per type)
    uint32_t timestamp;        // millis() at event creation
    uint8_t payload[2048];     // Event data (flexible payload)
} event_t;
```

**Macro-based emitter/receiver definition:**

```c
// Define emitter (publishes events)
EVENT_BUS_EMITTER_DEFINE(audio_events,    // Name
                         AUDIO_EVENT,     // Type
                         audio_data_t,    // Payload struct
                         8);              // Queue depth

// Define receiver (subscribes to events)
EVENT_BUS_RECEIVER_DEFINE(ui_receiver,    // Name
                          2048);          // Buffer size (bytes)

// Register receiver for event type
event_bus_register(&ui_receiver,    // Receiver
                         AUDIO_EVENT,     // Type filter
                         EVENT_BUS_ALL_IDS);  // ID filter (all IDs)
```

**Publishing events:**

```c
// In audio player code
void audio_play_track(const char* filename) {
    // ... start playback ...

    // Publish AUDIO_STARTED event
    audio_event_data_t event_data = {
        .event_id = AUDIO_STARTED,
        .filename = filename,
        .duration_ms = get_track_duration(filename),
    };

    event_bus_publish(&audio_events, &event_data, sizeof(event_data));

    // Note: No knowledge of who receives this event!
}
```

**Receiving events:**

```c
// In UI code
void ui_task(void* param) {
    while (1) {
        event_t event;

        // Block until event received
        if (event_bus_receive(&ui_receiver, &event, portMAX_DELAY) == STATUS_OK) {
            if (event.type == AUDIO_EVENT) {
                audio_event_data_t* data = (audio_event_data_t*)event.payload;

                switch (data->event_id) {
                    case AUDIO_STARTED:
                        ui_show_now_playing(data->filename);
                        break;
                    case AUDIO_PAUSED:
                        ui_show_paused_icon();
                        break;
                    case AUDIO_STOPPED:
                        ui_hide_now_playing();
                        break;
                }
            }
        }
    }
}
```

### 4.3 Type-Based Routing

the's event bus uses **type + ID filtering** for efficient routing:

**32 event types (BIT flags):**
- Each event type occupies one bit (BIT(0) to BIT(31))
- Subscribers register for types via bitmask (can subscribe to multiple types)
- Example: Subscribe to audio + battery events: `AUDIO_EVENT | BATTERY_EVENT`

**31 event IDs per type:**
- Within each type, 31 unique event IDs (0-30, 31 reserved for "all")
- Enables granular filtering (e.g., subscribe to "AUDIO_STARTED" but not "AUDIO_PROGRESS")

**Routing example:**
```c
// Subscriber A: Wants all audio events
event_bus_register(&sub_a, AUDIO_EVENT, EVENT_BUS_ALL_IDS);

// Subscriber B: Wants only AUDIO_STARTED and AUDIO_STOPPED
event_bus_register(&sub_b, AUDIO_EVENT, BIT(AUDIO_STARTED) | BIT(AUDIO_STOPPED));

// Subscriber C: Wants audio + battery events
event_bus_register(&sub_c, AUDIO_EVENT | BATTERY_EVENT, EVENT_BUS_ALL_IDS);
```

**Benefits:**
- ✅ **Targeted delivery:** Subscribers receive only relevant events (no broadcast spam)
- ✅ **Efficient filtering:** Bitmask operations very fast (single CPU instruction)
- ✅ **Scalable:** 32 types × 31 IDs = 992 unique event combinations

### 4.4 Static Allocation

the's event bus uses **static allocation** to avoid malloc/free:

**Fixed-size queues:**
```c
#define EVENT_BUS_EMITTER_DEFINE(name, type, payload_t, queue_depth) \
    static struct { \
        payload_t queue[queue_depth];  /* Static array */ \
        uint8_t head, tail, count; \
    } name##_storage; \
    static event_bus_emitter_t name = { \
        .type = type, \
        .storage = &name##_storage, \
        .queue_depth = queue_depth, \
    }

// Example: 8-entry queue, 176 bytes per event = 1408 bytes static RAM
EVENT_BUS_EMITTER_DEFINE(audio_events, AUDIO_EVENT, audio_data_t, 8);
```

**Benefits:**
- ✅ **Deterministic memory:** Queue size known at compile time
- ✅ **No fragmentation:** No malloc/free (embedded-safe)
- ✅ **Fast allocation:** Zero runtime cost

**Trade-offs:**
- ❌ **Fixed capacity:** Queue can overflow if events published faster than consumed
- ❌ **Memory waste:** Queue always allocates max size (even if underutilised)

**Overflow handling:**
```c
status_t event_bus_publish(event_bus_emitter_t* emitter, void* data, size_t len) {
    if (emitter->count >= emitter->queue_depth) {
        LOG_WARN("EVENT_BUS", "Queue full, dropping event (type: %d)", emitter->type);
        return STATUS_ERROR_QUEUE_FULL;  // Event dropped
    }

    // Enqueue event (circular buffer)
    memcpy(&emitter->queue[emitter->tail], data, len);
    emitter->tail = (emitter->tail + 1) % emitter->queue_depth;
    emitter->count++;

    return STATUS_OK;
}
```

**Monitoring:** A production embedded framework logs queue fullness every 10 seconds (debug builds) to detect sizing issues early.

### 4.5 FreeRTOS Message Buffers

the's event bus integrates with **FreeRTOS message buffers** for thread-safe event passing:

**Backend: StreamBuffer (zero-copy):**
```c
// Event bus receiver backed by FreeRTOS stream buffer
typedef struct {
    StreamBufferHandle_t buffer;  // FreeRTOS stream buffer
    event_type_t subscribed_types;  // Bitmask of subscribed types
    uint32_t subscribed_ids;      // Bitmask of subscribed IDs
} event_bus_receiver_t;

// Publish to receiver (thread-safe)
status_t event_bus_send_to_receiver(event_bus_receiver_t* receiver, event_t* event) {
    size_t bytes_sent = xStreamBufferSend(receiver->buffer,
                                          event,
                                          sizeof(event_t),
                                          0);  // Non-blocking
    return (bytes_sent > 0) ? STATUS_OK : STATUS_ERROR_QUEUE_FULL;
}

// Receive from buffer (thread-safe, blocking)
status_t event_bus_receive(event_bus_receiver_t* receiver, event_t* event, uint32_t timeout_ms) {
    size_t bytes_received = xStreamBufferReceive(receiver->buffer,
                                                  event,
                                                  sizeof(event_t),
                                                  pdMS_TO_TICKS(timeout_ms));
    return (bytes_received > 0) ? STATUS_OK : STATUS_ERROR_TIMEOUT;
}
```

**Benefits:**
- ✅ **Thread-safe:** Multiple producers/consumers without mutexes
- ✅ **Task notifications:** Receivers wake immediately on new events
- ✅ **Zero-copy (within limits):** Stream buffers avoid extra memcpy where possible

**Note:** For Arduino/cooperative multitasking, use simple circular buffer instead (no FreeRTOS dependency).

### 4.6 Benefits & Trade-offs

**Benefits:**
- ✅ **Decoupling:** Audio player doesn't know about UI, logger, or storage
- ✅ **Testability:** Inject events for testing (no need for real audio hardware)
- ✅ **Scalability:** Add new subscribers without modifying publishers
- ✅ **Thread-safe:** FreeRTOS integration enables safe multi-tasking
- ✅ **Flexibility:** Subscribers can be added/removed at runtime

**Trade-offs:**
- ❌ **Indirection:** Control flow harder to trace (who handles this event?)
- ❌ **Queue overflow:** Possible if events published faster than consumed
- ❌ **Memory overhead:** Static queues consume RAM (e.g., 1.4KB for 8-event audio queue)
- ❌ **Debugging:** Event flow not visible in call stack

**Best practices:**
- ✅ **Monitor queue fullness:** Log warnings when queues >75% full
- ✅ **Prioritise events:** Use priority levels (CRITICAL, HIGH, NORMAL, LOW) for important events
- ✅ **Size queues conservatively:** Oversize queues waste RAM; undersize causes drops

### 4.7 Applicability

**Highly recommended:**
- Multi-component systems (audio, UI, storage, networking)
- Asynchronous events (hardware interrupts, timers, network packets)
- Systems requiring testability (inject events for testing)
- Multi-threaded systems (FreeRTOS, Zephyr, Linux)

**Optional:**
- Simple single-threaded systems (direct function calls may be clearer)
- Performance-critical paths (measure overhead first)

**Real-world example:** this system uses event bus for:
- Audio events (STARTED, PAUSED, STOPPED, TRACK_CHANGED)
- NFC events (TAG_DETECTED, TAG_REMOVED)
- Battery events (CHARGING, FULL, LOW, CRITICAL)
- WiFi events (CONNECTED, DISCONNECTED, SCAN_COMPLETE)

This enables clean separation: Audio library doesn't know about NFC, UI doesn't know about WiFi, etc. Each component publishes events; others subscribe as needed.

---

## 5. Hardware Abstraction Layer (HAL)

### 5.1 Pattern Description

**What it is:** An interface-based abstraction of hardware peripherals (GPIO, I2C, SPI, timers, etc.) using function pointers. Business logic calls HAL functions; platform-specific implementations provide the actual hardware access.

**Why it exists:**
- **Platform portability:** Compile same code for ESP32, STM32, native (PC simulator)
- **Testability:** Inject fake HAL for unit tests (no hardware needed)
- **Driver swapping:** Change I2C library without modifying application code

**Dual-interface pattern:**
1. **Abstract interface** (platform-independent): Function pointer struct
2. **Platform-specific implementation** (ESP32, STM32, native): Concrete functions assigned at runtime

**When to use:**
- Multi-platform projects: **Required**
- Projects with unit testing: **Highly recommended**
- Single-platform, hardware-only testing: **Optional** (direct calls acceptable)

### 5.2 Production Implementation Example

**Abstract interface (platform-independent):**

```c
// File: hal/include/hal/hal_time.h

// Time abstraction interface
typedef struct {
    uint32_t (*get_millis)(void);       // Get milliseconds since boot
    void (*delay_ms)(uint32_t ms);      // Blocking delay
} hal_time_t;

// Global HAL instance (assigned by platform init)
extern const hal_time_t* hal_time;

// Convenience macros (optional, improves readability)
#define HAL_MILLIS() hal_time->get_millis()
#define HAL_DELAY_MS(ms) hal_time->delay_ms(ms)
```

**Platform-specific implementation (ESP32/Arduino):**

```c
// File: ports/esp32/hal_esp32.c

#include <Arduino.h>  // Arduino framework
#include "hal/hal_time.h"

// ESP32 implementation (wraps Arduino functions)
static uint32_t esp32_get_millis(void) {
    return millis();  // Arduino millis()
}

static void esp32_delay_ms(uint32_t ms) {
    delay(ms);  // Arduino delay()
}

// ESP32 HAL instance
static const hal_time_t esp32_time = {
    .get_millis = esp32_get_millis,
    .delay_ms = esp32_delay_ms,
};

// Platform initialisation (assigns HAL)
void hal_esp32_init(void) {
    hal_time = &esp32_time;  // Point to ESP32 implementation
    // ... init other HAL modules (GPIO, I2C, etc.)
}
```

**Native (PC) fake implementation:**

```c
// File: ports/native/hal_native.c

#include "hal/hal_time.h"

// Fake time (controllable for tests)
static uint32_t fake_millis_value = 0;

static uint32_t native_get_millis(void) {
    return fake_millis_value;
}

static void native_delay_ms(uint32_t ms) {
    fake_millis_value += ms;  // Advance fake time
}

// Native HAL instance
static const hal_time_t native_time = {
    .get_millis = native_get_millis,
    .delay_ms = native_delay_ms,
};

// Platform initialisation
void hal_native_init(void) {
    hal_time = &native_time;  // Point to fake implementation
}

// Test API for controlling fake time
void hal_native_set_millis(uint32_t value) {
    fake_millis_value = value;
}

void hal_native_advance_time(uint32_t delta) {
    fake_millis_value += delta;
}
```

**Application code (platform-independent):**

```c
// File: services/app/app_game/src/app_game.c

#include "hal/hal_time.h"

void state_active_update(void* ctx) {
    game_ctx_t* game = (game_ctx_t*)ctx;

    // Get current time (works on ESP32, STM32, native)
    uint32_t now = HAL_MILLIS();

    // Check timeout
    if (now - game->activity_start_time > 300000) {  // 5 minutes
        fsm_transition(&game->fsm, &state_complete);
    }
}
```

**Key insight:** Application code never calls `millis()` directly—always calls `HAL_MILLIS()`. This enables platform swapping and testing.

### 5.3 Platform-Specific Implementations

**ESP32 Arduino port:**
- Wraps Arduino functions (`millis()`, `digitalWrite()`, `Serial.println()`)
- Lightweight (thin wrapper, minimal overhead)
- Initialised in `setup()` before application code

**Native (PC/Mac) port:**
- Implements fakes for time, GPIO, logging
- Time is controllable (set/advance for deterministic tests)
- GPIO writes captured for verification
- Logs captured to buffer for assertions

**STM32 HAL port (if porting):**
- Wraps STM32 HAL functions (`HAL_GetTick()`, `HAL_GPIO_WritePin()`)
- May require adapters (e.g., convert pin numbers)

### 5.4 HAL Scope

A production embedded framework abstracts the following hardware interfaces:

**Time:**
- `get_millis()` - Milliseconds since boot
- `delay_ms()` - Blocking delay
- `get_rtc_time()` - Real-time clock (wall time)

**GPIO:**
- `pin_mode(pin, mode)` - Configure pin (INPUT/OUTPUT/INPUT_PULLUP)
- `digital_write(pin, value)` - Set pin HIGH/LOW
- `digital_read(pin)` - Read pin state
- `analog_read(pin)` - Read ADC value

**Communication buses:**
- **I2C:** `i2c_read()`, `i2c_write()`
- **SPI:** `spi_transfer()`
- **UART:** `uart_read()`, `uart_write()`

**Non-volatile storage (NVS):**
- `nvs_get(key)`, `nvs_set(key, value)`
- Key-value storage (settings, calibration data)

**Logging:**
- `log_print(msg)`, `log_println(msg)`
- Abstracted `Serial.println()` for portability

**Filesystem:**
- `fs_open()`, `fs_read()`, `fs_write()`
- LittleFS abstraction for SD card/flash

### 5.5 Compile-Time vs Runtime Selection

Two approaches to HAL implementation:

**1. Runtime (the pattern - function pointers):**
```c
// Assigned at runtime in init()
const hal_time_t* hal_time = NULL;

void setup() {
    hal_esp32_init();  // Assigns hal_time = &esp32_time
}
```

**Pros:**
- ✅ Flexible (swap HAL at runtime)
- ✅ Clean (single binary can support multiple platforms)

**Cons:**
- ❌ Slight overhead (function pointer call vs direct call)
- ❌ Not const (function pointers stored in RAM)

**2. Compile-time (alternative - macros or weak symbols):**
```c
// Macro-based (compile-time substitution)
#ifdef PLATFORM_ESP32
    #define HAL_MILLIS() millis()
#elif defined(PLATFORM_NATIVE)
    #define HAL_MILLIS() native_get_millis()
#endif

// Or weak symbols:
__attribute__((weak)) uint32_t hal_get_millis(void) {
    // Default implementation (overridden by platform)
}
```

**Pros:**
- ✅ Zero overhead (direct function call, compiler can inline)
- ✅ Const (everything in Flash)

**Cons:**
- ❌ Less flexible (must recompile for different platforms)
- ❌ More complex build system (#ifdef hell)

**the choice:** Runtime (function pointers) because:
- Overhead negligible with -O2 (compiler often inlines)
- Flexibility valued (supports simulator + hardware in same codebase)
- Cleaner code (no #ifdef in application logic)

### 5.6 Benefits & Trade-offs

**Benefits:**
- ✅ **Platform portability:** Compile for ESP32, STM32, native with zero application code changes
- ✅ **Testability:** Inject fake HAL for unit tests (no hardware required)
- ✅ **Driver swapping:** Change I2C library (e.g., Wire → TwoWire) without changing app code
- ✅ **Simulation:** Run firmware on PC for debugging (faster than flash + test)

**Trade-offs:**
- ❌ **Function pointer overhead:** Extra indirection (mitigated by compiler optimisation)
- ❌ **Debugging difficulty:** Extra call stack frame makes traces harder to read
- ❌ **Upfront investment:** Must design HAL interfaces before implementation
- ❌ **Maintenance:** More code to maintain (HAL + multiple ports)

**Performance measurement (a production embedded framework):**
- **-O0 (no optimisation):** ~5% overhead vs direct calls
- **-O2 (production):** <1% overhead (compiler inlines most calls)
- **Recommendation:** Measure in your code if concerned; overhead usually negligible

### 5.7 Applicability

**Required:**
- Multi-platform projects (ESP32 + STM32 + simulator)
- Projects with unit testing (HAL enables fakes)

**Highly recommended:**
- Projects >6 months lifetime (likely to port/test)
- Commercial products (testability improves quality)

**Optional:**
- Single-platform, hardware-only testing projects
- Performance-critical code (measure overhead first)

**Real-world example:** this system runs on:
1. **ESP32 (production):** Real hardware, Arduino HAL
2. **Native (simulator):** PC/Mac for development, fake HAL
3. **Docker (CI):** Linux for automated tests, fake HAL

Same application code compiles for all three platforms—only HAL implementation changes. This enables:
- ✅ Fast iteration (test on PC, deploy to hardware)
- ✅ CI/CD (run tests in Docker on every commit)
- ✅ Multi-platform support (easily port to STM32 if needed)

---

## 6. Testing Strategies

### 6.1 Native Testing with Fakes

**What it is:** Compiling embedded firmware for your development machine (PC/Mac/Linux) and running unit tests with fake HAL implementations instead of real hardware.

**Why it exists:**
- **Speed:** Native tests run in ~2 seconds; flashing + testing on hardware takes ~60 seconds
- **Determinism:** Fake HAL provides controllable timing (no race conditions, no flaky tests)
- **CI/CD:** Automated tests run in Docker without hardware
- **Convenience:** Test during development without hardware setup

**How it works:**
1. Application code uses HAL (see [Section 5](#5-hardware-abstraction-layer-hal))
2. For hardware: Link with ESP32/STM32 HAL
3. For tests: Link with native fake HAL
4. Run tests with CppUTest framework

**When to use:**
- Production firmware: **Highly recommended**
- Prototypes/one-off projects: **Optional** (hardware testing may suffice)
- Safety-critical systems: **Required** (+ formal verification)

### 6.2 A Production Embedded Framework Test Infrastructure

**Framework:** CppUTest (C/C++ unit testing framework)

**Test discovery:** CMake automatically discovers `test_*.cpp` files in `tests/` directories:

```cmake
# CMakeLists.txt (a production embedded framework pattern)
file(GLOB_RECURSE TEST_SOURCES "services/*/tests/test_*.cpp")

foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} CppUTest fsm hal_native)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
```

**Shared test plugins:**
A production embedded framework provides common test utilities in `tests/plugins/`:
- **Filesystem plugin:** Helpers for testing file operations
- **Event plugin:** Event bus test utilities (inject/verify events)
- **Provision plugin:** Device provisioning helpers

**Coverage support:**
- **Clang:** `-fprofile-instr-generate -fcoverage-mapping`
- **GCC:** `--coverage`
- **Report:** `lcov` generates HTML coverage report
- **Threshold:** CI fails if coverage drops below 80%

### 6.3 Test Structure

**Example test file:** `services/base/fsm/tests/test_fsm.cpp`

```cpp
#include "CppUTest/TestHarness.h"
#include "fsm/fsm.h"
#include "hal/hal_time.h"
#include "hal/hal_native.h"  // Fake HAL for tests

// Test fixture (setup/teardown)
TEST_GROUP(FSM_Tests) {
    fsm_ctx_t fsm;
    fsm_state_t states[3];

    void setup() {
        // Initialise fake HAL
        hal_native_init();
        fake_reset();

        // Define test states
        states[0] = (fsm_state_t){
            .name = "STATE_IDLE",
            .entry = state_idle_entry,
            .run = NULL,
            .exit = state_idle_exit,
            .parent = NULL,
            .run_period_ms = 0,
            .id = 0,
        };

        states[1] = (fsm_state_t){
            .name = "STATE_ACTIVE",
            .entry = state_active_entry,
            .run = state_active_run,
            .exit = state_active_exit,
            .parent = NULL,
            .run_period_ms = 100,
            .id = 1,
        };

        states[2] = (fsm_state_t){
            .name = "STATE_COMPLETE",
            .entry = NULL,
            .run = NULL,
            .exit = NULL,
            .parent = NULL,
            .run_period_ms = 0,
            .id = 2,
        };

        // Initialise FSM
        fsm.states = states;
        fsm.num_states = 3;
        fsm_init(&fsm, &states[0]);
    }

    void teardown() {
        // Cleanup if needed
    }
};

// Test: Basic state transition
TEST(FSM_Tests, TransitionUpdatesCurrentState) {
    // Initial state should be STATE_IDLE
    CHECK_EQUAL(&states[0], fsm.current_state);

    // Transition to STATE_ACTIVE
    status_t status = fsm_transition(&fsm, &states[1]);

    // Verify transition succeeded
    CHECK_EQUAL(STATUS_OK, status);
    CHECK_EQUAL(&states[1], fsm.current_state);
    STRCMP_EQUAL("STATE_ACTIVE", fsm.current_state->name);
}

// Test: Entry/exit functions called
TEST(FSM_Tests, TransitionCallsEntryAndExit) {
    static bool idle_exit_called = false;
    static bool active_entry_called = false;

    states[0].exit = [](void* ctx) { idle_exit_called = true; };
    states[1].entry = [](void* ctx) { active_entry_called = true; };

    // Transition
    fsm_transition(&fsm, &states[1]);

    // Verify lifecycle functions called
    CHECK_TRUE(idle_exit_called);
    CHECK_TRUE(active_entry_called);
}

// Test: Timing (using fake HAL)
TEST(FSM_Tests, StateEntryTimeRecorded) {
    // Set fake time to 1000ms
    fake_set_millis(1000);

    // Transition (should record entry time)
    fsm_transition(&fsm, &states[1]);

    // Verify entry time
    CHECK_EQUAL(1000, fsm.state_entry_time);

    // Advance time
    fake_advance_time(500);

    // Verify time advanced
    CHECK_EQUAL(1500, HAL_MILLIS());
}

// Test: Null pointer handling
TEST(FSM_Tests, TransitionRejectsNullState) {
    status_t status = fsm_transition(&fsm, NULL);
    CHECK_EQUAL(STATUS_ERROR_NULL_POINTER, status);
}
```

**Running tests:**
```bash
# Build and run all tests
mkdir build && cd build
cmake .. -DCOMPILE_TESTS=ON
make
ctest --output-on-failure

# Output:
# Test project /path/to/build
#     Start 1: test_fsm
# 1/5 Test #1: test_fsm .........................   Passed    0.02 sec
#     Start 2: test_event_bus
# 2/5 Test #2: test_event_bus ...................   Passed    0.03 sec
# ...
# 100% tests passed, 0 tests failed out of 5
```

### 6.4 Fake HAL API

**Controllable time:**
```c
// Set absolute time
void fake_set_millis(uint32_t value);

// Advance time by delta
void fake_advance_time(uint32_t delta);

// Example test:
fake_set_millis(0);
start_timer();
fake_advance_time(1000);  // Simulate 1 second passing
CHECK_TRUE(timer_expired());
```

**Verifiable GPIO:**
```c
// Capture GPIO writes
void fake_digital_write(uint8_t pin, uint8_t value);

// Verify GPIO state
uint8_t fake_get_gpio(uint8_t pin);

// Example test:
fake_digital_write(LED_PIN, HIGH);
CHECK_EQUAL(HIGH, fake_get_gpio(LED_PIN));
```

**Log capture:**
```c
// Get captured log buffer
const char* fake_get_log_buffer(void);

// Check if log contains string
bool fake_get_last_log_contains(const char* substr);

// Reset log buffer
void fake_clear_log_buffer(void);

// Example test:
my_function();  // Logs "Starting operation"
CHECK_TRUE(fake_get_last_log_contains("Starting operation"));
```

**I2C/SPI fakes:**
```c
// Inject I2C read data
void fake_i2c_set_read_data(uint8_t addr, const uint8_t* data, size_t len);

// Verify I2C write
bool fake_i2c_was_written(uint8_t addr, const uint8_t* expected_data, size_t len);

// Example test:
fake_i2c_set_read_data(NFC_ADDR, mock_uid, 4);
uint32_t uid = nfc_read_uid();
CHECK_EQUAL(0x12345678, uid);
```

### 6.5 Memory Leak Detection

**AddressSanitizer (Clang/GCC):**
- Detects: Memory leaks, buffer overflows, use-after-free
- Enable: `-fsanitize=address` compile flag
- Run: Tests automatically check for leaks

```bash
# Build with AddressSanitizer
cmake .. -DENABLE_MEMLEAK_DETECTION=ON
make
ctest

# Output if leak detected:
# =================================================================
# ==12345==ERROR: LeakSanitizer: detected memory leaks
#
# Direct leak of 100 byte(s) in 1 object(s) allocated from:
#     #0 0x7f8b9c in malloc
#     #1 0x401234 in my_function src/my_file.c:42
#
# SUMMARY: AddressSanitizer: 100 byte(s) leaked in 1 allocation(s).
```

**CppUTest built-in leak detection:**
- Tracks `new`/`delete` (C++) and `malloc`/`free` (C)
- Reports leaks per test
- Fails test if leak detected

**A production embedded framework approach:**
```bash
# Run tests with leak detection
inv test --leak-detection

# Or directly:
cmake .. -DENABLE_MEMLEAK_DETECTION=ON && make && ctest
```

### 6.6 Continuous Integration

**Bitbucket Pipelines (a production embedded framework CI):**

```yaml
# bitbucket-pipelines.yml
image: organization/firmware:latest

pipelines:
  default:
    - step:
        name: Build and Test
        script:
          - mkdir build && cd build
          - cmake .. -DCOMPILE_TESTS=ON -DENABLE_COVERAGE=ON
          - make -j4
          - ctest --output-on-failure
          - lcov --capture --directory . --output-file coverage.info
          - lcov --remove coverage.info '/usr/*' --output-file coverage_filtered.info
          - genhtml coverage_filtered.info --output-directory coverage_report
        artifacts:
          - build/coverage_report/**
```

**Gating criteria:**
- ✅ All tests must pass
- ✅ Coverage must be >80%
- ✅ No memory leaks detected
- ✅ Code formatting checked (clang-format)

**Benefits:**
- ✅ Catches regressions before merge
- ✅ Ensures code quality standards met
- ✅ Coverage reports guide testing efforts
- ✅ Automated (no manual testing needed)

### 6.7 Benefits & Trade-offs

**Benefits:**
- ✅ **Fast feedback:** 2-second tests vs 60-second flash + test
- ✅ **Deterministic:** Fake HAL eliminates flaky hardware-dependent tests
- ✅ **CI/CD:** Run tests in Docker on every commit
- ✅ **Regression prevention:** Test suite guards against breaking changes
- ✅ **No hardware needed:** Develop/test without physical device

**Trade-offs:**
- ❌ **Upfront investment:** HAL + fake + test infrastructure takes time to set up
- ❌ **Tests can diverge:** Fake HAL may not perfectly match real hardware
- ❌ **Integration testing still required:** Unit tests don't catch hardware issues (timing, electrical)
- ❌ **Maintenance:** Tests need updating when code changes

**Best practices:**
- ✅ **Complement, don't replace:** Use native tests for business logic, hardware tests for integration
- ✅ **Test public APIs:** Focus on module interfaces, not internal implementation
- ✅ **Keep tests fast:** Each test <100ms (fake HAL enables this)
- ✅ **Use descriptive names:** Test names document expected behaviour

### 6.8 Applicability

**Highly recommended:**
- Production firmware (regression prevention critical)
- Long-lifetime projects (>6 months)
- Team development (tests prevent breaking changes)
- CI/CD environments

**Optional:**
- Prototypes/one-off projects (hardware testing may suffice)
- Very simple firmware (<1KLOC)

**Real-world example:** A production embedded framework has 100+ unit tests covering:
- FSM state transitions (15 tests)
- Event bus publish/subscribe (12 tests)
- Filesystem operations (20 tests)
- HTTP client (18 tests)
- Audio playback logic (25 tests)
- Power management (10 tests)

Test suite runs in **2.5 seconds** on laptop vs **30+ minutes** if testing on hardware. This enables:
- ✅ Running tests on every code change (instant feedback)
- ✅ CI/CD (automated tests on every commit)
- ✅ Regression prevention (tests catch breaking changes)

---

## 7. Static Allocation Patterns

### 7.1 Pattern Description

**What it is:** Avoiding `malloc`/`free` in favour of compile-time allocation using `static`, stack, or `const` storage.

**Why it exists:**
- **Deterministic memory:** Total RAM usage known at compile time
- **No fragmentation:** Heap fragmentation causes mysterious failures in long-running embedded systems
- **Faster:** Zero runtime allocation cost (no malloc overhead)
- **Safer:** No out-of-memory errors at runtime

**How it works:**
1. Define context structs with fixed-size buffers
2. Use CREATE_CTX macros to declare static instances
3. Pass context pointers to functions (no globals)

**When to use:**
- Resource-constrained systems (<128KB RAM): **Highly recommended**
- Safety-critical systems: **Highly recommended**
- Systems with abundant RAM (>1MB): **Optional** (malloc acceptable)

### 7.2 CREATE_CTX Macro Pattern

**Problem:** Declaring static contexts is verbose and error-prone:

```c
// Verbose manual declaration
static fsm_state_t game_fsm_states[8];
static fsm_ctx_t game_fsm = {
    .states = game_fsm_states,
    .num_states = 8,
    .current_state = NULL,
    .state_entry_time = 0,
    .user_ctx = NULL,
};
```

**Solution:** CREATE_CTX macro hides boilerplate:

```c
// Macro definition
#define SERVICE_BS_FSM_CREATE_CTX(name, num_states) \
    static fsm_state_t name##_states[num_states]; \
    static fsm_ctx_t name = { \
        .states = name##_states, \
        .num_states = num_states, \
    }

// Usage (much cleaner!)
SERVICE_BS_FSM_CREATE_CTX(game_fsm, 8);  // Creates game_fsm context with 8 states

// Equivalent to the verbose version above
```

**Benefits:**
- ✅ **Cleaner syntax:** One line instead of 8+ lines
- ✅ **Less error-prone:** Macro ensures correct initialisation
- ✅ **Consistent:** All services use same pattern

**Examples from a production embedded framework:**

```c
// Event bus context
#define SERVICE_BS_EVENT_BUS_CREATE_CTX(name, queue_depth) \
    static event_t name##_queue[queue_depth]; \
    static event_bus_ctx_t name = { \
        .queue = name##_queue, \
        .queue_depth = queue_depth, \
        .head = 0, \
        .tail = 0, \
        .count = 0, \
    }

SERVICE_BS_EVENT_BUS_CREATE_CTX(global_event_bus, 8);  // 8-event queue

// Logger context
#define SERVICE_BS_LOGGER_CREATE_CTX(name) \
    static fsm_logger_ctx_t name = { \
        .global_level = LOG_LEVEL_INFO, \
    }

SERVICE_BS_LOGGER_CREATE_CTX(system_logger);
```

### 7.3 Static Buffers

**Common pattern:** Pre-allocate fixed-size buffers for known use cases.

**String formatting:**
```c
// Static buffer for log messages (256 bytes)
static char log_buffer[256];

void log_temperature(float temp) {
    snprintf(log_buffer, sizeof(log_buffer), "Temperature: %.2f°C", temp);
    HAL_LOG(log_buffer);
}
```

**Audio decode buffer:**
```c
// Static 20KB decode buffer for MP3 decoding
static uint8_t audio_decode_buffer[20480];

void audio_decode_frame(const uint8_t* mp3_data) {
    mp3_decode(mp3_data, audio_decode_buffer, sizeof(audio_decode_buffer));
    dac_write(audio_decode_buffer);
}
```

**Network packet buffer:**
```c
// Static 1500-byte buffer for Ethernet frames
static uint8_t network_rx_buffer[1500];

void network_receive_packet(void) {
    size_t len = ethernet_read(network_rx_buffer, sizeof(network_rx_buffer));
    process_packet(network_rx_buffer, len);
}
```

**Benefits:**
- ✅ **Fast:** Zero allocation cost (buffer already exists)
- ✅ **Predictable:** RAM usage known at compile time
- ✅ **Safe:** No out-of-memory errors

**Trade-offs:**
- ❌ **Fixed size:** Can't grow at runtime
- ❌ **Memory waste:** Buffer always allocates max size (even if unused)

**Best practices:**
- ✅ **Size conservatively:** Oversize wastes RAM; undersize causes truncation
- ✅ **Document sizes:** Comment why buffer size chosen (e.g., "1500 = max Ethernet frame")
- ✅ **Use sizeof():** Prevents buffer overruns (e.g., `snprintf(buf, sizeof(buf), ...)`)

### 7.4 Const Tables

**Pattern:** Store read-only data in Flash (const) instead of RAM (static).

**State handler table (Flash):**
```c
// State handlers stored in Flash (const = read-only)
static const StateHandler state_handlers[NUM_STATES] = {
    {idle_enter, idle_update, idle_exit},
    {active_enter, active_update, active_exit},
    {paused_enter, paused_update, paused_exit},
    {complete_enter, complete_update, complete_exit},
};

// sizeof(state_handlers) = 0 bytes RAM, ~48 bytes Flash
// (Function pointers stored in Flash, only current_state pointer in RAM)
```

**Configuration tables (Flash):**
```c
// Pin configuration (read-only)
static const struct {
    uint8_t pin;
    const char* name;
} gpio_config[] = {
    {GPIO2, "LED_STATUS"},
    {GPIO5, "LED_DATA"},
    {GPIO21, "I2C_SDA"},
    {GPIO22, "I2C_SCL"},
};

// sizeof(gpio_config) = 0 bytes RAM, ~64 bytes Flash
```

**Lookup tables (Flash):**
```c
// Sine wave lookup table (256 entries)
static const uint8_t sine_table[256] = {
    128, 131, 134, 137, 140, 143, ...
};

// sizeof(sine_table) = 0 bytes RAM, 256 bytes Flash
```

**Benefits:**
- ✅ **Saves RAM:** Const data stored in Flash (cheap) instead of RAM (expensive)
- ✅ **Fast:** Direct Flash access on ARM Cortex-M (no performance penalty)
- ✅ **Safe:** Read-only prevents accidental modification

**Trade-offs:**
- ❌ **Immutable:** Can't modify at runtime (by design)
- ❌ **Flash wear:** If data changes frequently, Flash may wear out (not an issue for const data)

**Best practice:** Default to `const` for read-only data. Only use `static` (RAM) if data must change at runtime.

### 7.5 Benefits & Trade-offs

**Benefits:**
- ✅ **Deterministic memory usage:** Total RAM known at compile time (linker reports usage)
- ✅ **No fragmentation:** Eliminates mysterious "out of memory" failures after hours/days of operation
- ✅ **Faster allocation:** Zero runtime cost (malloc can be slow)
- ✅ **Safer:** No malloc failures at runtime
- ✅ **Easier to analyse:** Memory map shows exact RAM usage

**Trade-offs:**
- ❌ **Fixed capacity:** Buffers can't grow at runtime
- ❌ **Memory waste:** Buffers always allocate max size (even if underutilised)
- ❌ **Requires upfront sizing:** Must predict max usage (oversize = waste, undersize = bugs)
- ❌ **Less flexible:** Can't adapt to varying workloads

**Best practices:**
- ✅ **Profile memory usage:** Measure actual usage, size buffers accordingly
- ✅ **Add headroom:** Oversize by 20% to handle edge cases
- ✅ **Monitor at runtime:** Log warnings if buffers >75% full
- ✅ **Document sizing:** Comment why buffer size chosen

### 7.6 Applicability

**Highly recommended:**
- Resource-constrained systems (<128KB RAM)
- Safety-critical systems (deterministic = safer)
- Long-running systems (fragmentation risk)

**Optional:**
- Systems with abundant RAM (>1MB)
- Short-lived systems (fragmentation unlikely)
- Prototypes (flexibility > optimisation)

**Real-world example:** this system uses static allocation for:
- Event bus queues (1.4KB per bus × 5 buses = 7KB)
- Audio decode buffer (20KB)
- Network buffers (3KB)
- Filesystem cache (16KB)
- Total static: ~50KB RAM (predictable, no fragmentation)

Alternative (dynamic allocation):
- Same buffers with malloc = 50KB heap
- Fragmentation risk after days of operation
- malloc failures possible (out of memory)

---

## 8. Error Handling

### 8.1 Status Code Pattern

**Pattern:** Functions return standardised status codes instead of booleans or void.

**Enum definition:**
```c
// File: hal/include/hal/hal_types.h

typedef enum {
    STATUS_OK = 0,                  // Success

    // Generic errors (1-19)
    STATUS_ERROR_INVALID_ARG = 1,
    STATUS_ERROR_NULL_POINTER = 2,
    STATUS_ERROR_OUT_OF_BOUNDS = 3,
    STATUS_ERROR_TIMEOUT = 4,
    STATUS_ERROR_NO_MEMORY = 5,

    // FSM errors (20-29)
    STATUS_ERROR_FSM_INVALID_STATE = 20,
    STATUS_ERROR_FSM_TRANSITION_FAILED = 21,

    // Event bus errors (30-39)
    STATUS_ERROR_EVENT_QUEUE_FULL = 30,
    STATUS_ERROR_EVENT_INVALID_TYPE = 31,

    // LED errors (40-49)
    STATUS_ERROR_LED_INIT_FAILED = 40,

    // Hardware errors (50-59)
    STATUS_ERROR_HW_NOT_READY = 50,
    STATUS_ERROR_HW_FAILURE = 51,

    // NFC errors (60-69) - Reserved
    // Audio errors (70-79) - Reserved
    // Storage errors (80-89) - Reserved
    // ... up to ~40 subsystems (ranges of 10)
} status_t;
```

**Function signatures:**
```c
// Before (unclear error handling)
void audio_play(const char* filename);  // How do I know if it failed?
bool nfc_read_uid(uint32_t* uid);       // What kind of failure? Timeout? Hardware error?

// After (explicit error codes)
status_t audio_play(const char* filename);
status_t nfc_read_uid(uint32_t* uid);
```

### 8.2 Error Propagation Macros

**Problem:** Checking return values is verbose:

```c
status_t my_function(void) {
    status_t status;

    status = init_subsystem_a();
    if (status != STATUS_OK) {
        LOG_ERROR("INIT", "Failed to init subsystem A");
        return status;
    }

    status = init_subsystem_b();
    if (status != STATUS_OK) {
        LOG_ERROR("INIT", "Failed to init subsystem B");
        return status;
    }

    status = init_subsystem_c();
    if (status != STATUS_OK) {
        LOG_ERROR("INIT", "Failed to init subsystem C");
        return status;
    }

    return STATUS_OK;
}
```

**Solution:** SERVICE_CHECK macro (early return on error):

```c
#define SERVICE_CHECK(expr) do { \
    status_t _status = (expr); \
    if (_status != STATUS_OK) { \
        LOG_ERROR("CHECK", "Error in " #expr); \
        return _status; \
    } \
} while(0)

// Refactored (much cleaner!)
status_t my_function(void) {
    SERVICE_CHECK(init_subsystem_a());
    SERVICE_CHECK(init_subsystem_b());
    SERVICE_CHECK(init_subsystem_c());
    return STATUS_OK;
}
```

**SERVICE_WARN_IF_ERROR macro (log but continue):**

```c
#define SERVICE_WARN_IF_ERROR(expr) do { \
    status_t _status = (expr); \
    if (_status != STATUS_OK) { \
        LOG_WARN("CHECK", "Warning in " #expr); \
    } \
} while(0)

// Usage:
void loop() {
    SERVICE_WARN_IF_ERROR(led_update());        // Log if fails, but continue
    SERVICE_WARN_IF_ERROR(event_bus_process()); // Don't halt entire loop
    SERVICE_WARN_IF_ERROR(fsm_run());
}
```

### 8.3 Error Grouping

**Pattern:** Group status codes by subsystem (ranges of 10):

| Range | Subsystem | Examples |
|-------|-----------|----------|
| 0 | Success | STATUS_OK |
| 1-19 | Generic | INVALID_ARG, TIMEOUT, NO_MEMORY |
| 20-29 | FSM | FSM_INVALID_STATE, FSM_TRANSITION_FAILED |
| 30-39 | Event bus | EVENT_QUEUE_FULL, EVENT_INVALID_TYPE |
| 40-49 | LED | LED_INIT_FAILED |
| 50-59 | Hardware | HW_NOT_READY, HW_FAILURE |
| 60-69 | NFC (future) | NFC_TAG_NOT_FOUND, NFC_READ_ERROR |
| 70-79 | Audio (future) | AUDIO_DECODE_ERROR, AUDIO_NO_FILE |
| 80-89 | Storage (future) | STORAGE_FULL, STORAGE_CORRUPTED |

**Benefits:**
- ✅ **Quick identification:** Error code 35 → Event bus error
- ✅ **Scalable:** Each subsystem owns a range (no collisions)
- ✅ **Organised:** Related errors grouped together

**Example error handling:**

```c
status_t status = nfc_read_uid(&uid);

if (status == STATUS_OK) {
    // Success path
    process_uid(uid);
} else if (status >= 60 && status < 70) {
    // NFC-specific error
    LOG_ERROR("NFC", "NFC read failed: %d", status);
    led_set_animation(LED_ERROR_NFC);
} else if (status == STATUS_ERROR_TIMEOUT) {
    // Generic timeout
    LOG_WARN("NFC", "NFC read timeout (retrying)");
    retry_nfc_read();
} else {
    // Unexpected error
    LOG_ERROR("NFC", "Unexpected error: %d", status);
}
```

### 8.4 Benefits & Trade-offs

**Benefits:**
- ✅ **Explicit error handling:** No hidden failures (vs void functions)
- ✅ **Consistent across codebase:** Every function uses same pattern
- ✅ **Enables error recovery:** Caller can distinguish timeout vs hardware failure
- ✅ **Better logging:** Error codes aid debugging

**Trade-offs:**
- ❌ **Verbose:** Must check return value every time
- ❌ **Easy to ignore:** Compiler doesn't enforce checking (C limitation)
- ❌ **Boilerplate:** SERVICE_CHECK macro helps, but still more code

**Best practices:**
- ✅ **Enable compiler warnings:** `-Wunused-result` warns if status ignored
- ✅ **Use macros:** SERVICE_CHECK reduces boilerplate
- ✅ **Document status codes:** Doxygen `@return` lists possible errors
- ✅ **Test error paths:** Unit tests should cover failure cases

### 8.5 Applicability

**Recommended for:**
- All embedded systems (errors are common)
- Production firmware (explicit error handling improves reliability)
- Team development (status codes document failure modes)

**Optional for:**
- Prototypes (can use asserts/panics for simplicity)
- Very simple firmware (few failure modes)

**Real-world example:** A production embedded framework uses status codes throughout:
- FSM transitions can fail (invalid state, null pointer)
- Event bus can overflow (queue full)
- Audio playback can fail (file not found, decode error)
- NFC reads can timeout

Status codes enable graceful recovery:
- FSM: Log error, stay in current state
- Event bus: Drop event, log warning
- Audio: Show error UI, return to menu
- NFC: Retry read, show "tap again" message

---

## 9. Code Quality

### 9.1 Clang-Format

**Purpose:** Eliminate formatting debates, ensure consistency across codebase.

**A production embedded framework style (.clang-format):**
```yaml
BasedOnStyle: Google
Language: Cpp
ColumnLimit: 120         # Max line length
IndentWidth: 4           # Spaces per indent level
UseTab: Never            # Always use spaces
PointerAlignment: Left   # int* ptr (not int *ptr)
IncludeBlocks: Preserve  # Don't reorder includes
SortIncludes: false      # Manual include ordering
AllowShortFunctionsOnASingleLine: Inline
AlignConsecutiveAssignments: false
BreakBeforeBraces: Attach   # Opening brace on same line
```

**Automation:**
```bash
# Format all files
find services/ hal/ ports/ -name "*.h" -o -name "*.c" -o -name "*.cpp" | xargs clang-format -i

# Check formatting (CI)
clang-format --dry-run --Werror services/**/*.{h,c,cpp}
```

**Pre-commit hook (.git/hooks/pre-commit):**
```bash
#!/bin/bash
git diff --cached --name-only --diff-filter=ACM | grep -E '\.(h|c|cpp)$' | xargs clang-format -i
git add -u  # Re-add formatted files
```

**Benefits:**
- ✅ **Zero debates:** Code formatting automatic
- ✅ **Consistent:** All code looks the same
- ✅ **Clean diffs:** Formatting changes don't clutter PRs

### 9.2 Doxygen Documentation

**Purpose:** Generate browsable HTML documentation from code comments.

**Function documentation:**
```c
/**
 * @file fsm.h
 * @brief Hierarchical finite state machine framework
 * @copyright © the development team, 2025. All rights reserved
 */

/**
 * @brief Transition state machine to new state
 *
 * Calls exit() on current state, updates state pointer, then calls entry()
 * on new state. Updates state_entry_time to current millis().
 *
 * @param ctx [IN] State machine context (must not be NULL)
 * @param new_state [IN] Target state pointer (must not be NULL)
 * @return STATUS_OK on success
 * @return STATUS_ERROR_NULL_POINTER if ctx or new_state is NULL
 * @return STATUS_ERROR_FSM_INVALID_STATE if new_state->id >= ctx->num_states
 *
 * @note This function is not re-entrant. Do not call from interrupt context.
 * @warning If entry() or exit() functions are slow, this will block.
 *
 * @see fsm_init
 * @see fsm_run
 */
status_t fsm_transition(fsm_ctx_t* ctx, fsm_state_t* new_state);
```

**Struct documentation:**
```c
/**
 * @brief FSM state definition
 *
 * Defines a single state in the state machine with entry/run/exit lifecycle.
 */
typedef struct fsm_state {
    const char* name;            ///< State name (for debugging)
    void (*entry)(void* ctx);    ///< Called once on state entry
    void (*run)(void* ctx);      ///< Called periodically (or manually)
    void (*exit)(void* ctx);     ///< Called once on state exit
    struct fsm_state* parent;  ///< Parent state (NULL if root)
    uint32_t run_period_ms;      ///< Run frequency (0 = manual mode)
    uint8_t id;                  ///< Unique state ID
} fsm_state_t;
```

**Generating documentation:**
```bash
# Create Doxyfile
doxygen -g Doxyfile

# Edit Doxyfile:
# PROJECT_NAME = "the Services"
# INPUT = services/ hal/ ports/
# RECURSIVE = YES
# GENERATE_HTML = YES
# OUTPUT_DIRECTORY = docs/doxygen

# Generate docs
doxygen Doxyfile
open docs/doxygen/html/index.html  # View in browser
```

**Benefits:**
- ✅ **Browsable:** Navigate documentation in browser
- ✅ **Up-to-date:** Generated from code (can't get stale)
- ✅ **Cross-references:** Links between related functions

### 9.3 Compiler Warnings

**Strict warning flags:**
```cmake
# CMakeLists.txt
add_compile_options(-Wall -Wextra -Werror)
```

| Flag | Catches |
|------|---------|
| `-Wall` | Most common bugs (unused variables, implicit conversions) |
| `-Wextra` | Additional checks (signed/unsigned comparison) |
| `-Werror` | Treat warnings as errors (CI fails on warnings) |

**Example warnings caught:**
```c
// Warning: unused variable
void my_function(void) {
    int unused_var = 0;  // ⚠️ warning: unused variable 'unused_var'
}

// Warning: implicit conversion
void process_data(uint8_t data) {
    int16_t value = -data;  // ⚠️ warning: implicit conversion changes signedness
}

// Warning: missing return
int get_value(void) {
    // ⚠️ warning: control reaches end of non-void function
}
```

**Benefits:**
- ✅ **Catches bugs early:** Compiler finds issues before runtime
- ✅ **Enforces quality:** CI fails on warnings (forces fixes)

### 9.4 Applicability

**Recommended for all projects:**
- Clang-format: Low cost, high benefit (eliminates formatting debates)
- Compiler warnings: Free bug detection
- Doxygen: Valuable for projects >3KLOC or team development

---

## 10. Build Systems

### 10.1 CMake Pattern

**A production embedded framework uses CMake** for multi-platform builds (native tests, ESP32, simulator).

**Custom functions (cmake/services_defs.cmake):**

```cmake
# Register a service (adds library + includes)
function(service NAME)
    add_subdirectory(services/${NAME})
endfunction()

# Add service library (standard pattern)
function(add_service_library)
    set(options "")
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES PUBLIC_HEADERS DEPENDENCIES)
    cmake_parse_arguments(SVC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_library(${SVC_NAME} ${SVC_SOURCES})
    target_include_directories(${SVC_NAME} PUBLIC include)
    target_link_libraries(${SVC_NAME} PUBLIC ${SVC_DEPENDENCIES})
    install(FILES ${SVC_PUBLIC_HEADERS} DESTINATION include/${SVC_NAME})
endfunction()

# Auto-discover tests
function(register_test)
    file(GLOB_RECURSE TEST_FILES "tests/test_*.cpp")
    foreach(TEST_FILE ${TEST_FILES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_FILE})
        target_link_libraries(${TEST_NAME} CppUTest ${SERVICE_NAME} hal_native)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
endfunction()

# Auto-discover examples
function(register_example)
    file(GLOB EXAMPLE_FILES "examples/*.c")
    foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
        get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
        add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE})
        target_link_libraries(${EXAMPLE_NAME} ${SERVICE_NAME})
    endforeach()
endfunction()
```

**Build variants:**
```bash
# Native tests (PC/Mac)
cmake .. -DCOMPILE_TESTS=ON
make
ctest

# Simulator (SDL2 + LVGL)
cmake .. -DCOMPILE_SIMULATOR=ON
make
./simulator

# Examples
cmake .. -DCOMPILE_EXAMPLES=ON
make
./fsm_demo

# Coverage
cmake .. -DENABLE_COVERAGE=ON
make
ctest
lcov --capture --directory . --output-file coverage.info
```

**Benefits:**
- ✅ **Multi-platform:** Same CMake builds for native, ESP32, simulator
- ✅ **Auto-discovery:** Tests/examples automatically found
- ✅ **Consistent:** All services use same build pattern

### 10.2 Python Invoke Automation

**A production embedded framework uses Python `invoke` tasks** for common workflows (tasks.py):

```python
from invoke import task

@task
def test(c, leak_detection=False, network=False):
    """Run unit tests"""
    flags = []
    if leak_detection:
        flags.append("-DENABLE_MEMLEAK_DETECTION=ON")
    if network:
        flags.append("-DCOMPILE_TESTS_NETWORK=ON")

    c.run(f"mkdir -p build && cd build && cmake .. {' '.join(flags)} && make -j4 && ctest --output-on-failure")

@task
def coverage(c):
    """Generate coverage report"""
    c.run("inv test")
    c.run("cd build && lcov --capture --directory . --output-file coverage.info")
    c.run("cd build && genhtml coverage.info --output-directory coverage_report")
    print("Coverage report: build/coverage_report/index.html")

@task
def format(c):
    """Format code with clang-format"""
    c.run("find services/ hal/ ports/ -name '*.h' -o -name '*.c' -o -name '*.cpp' | xargs clang-format -i")

@task
def simulator(c):
    """Build and run simulator"""
    c.run("cmake .. -DCOMPILE_SIMULATOR=ON && make simulator && ./simulator")

@task
def clean(c):
    """Clean build directory"""
    c.run("rm -rf build")
```

**Usage:**
```bash
# Run tests
inv test

# Run tests with leak detection
inv test --leak-detection

# Generate coverage report
inv coverage

# Format code
inv format

# Run simulator
inv simulator

# Clean
inv clean
```

**Benefits:**
- ✅ **Simple commands:** `inv test` instead of long cmake/make/ctest sequence
- ✅ **Automation:** Common workflows scripted
- ✅ **Discoverable:** `inv --list` shows available tasks

### 10.3 Benefits & Trade-offs

**Benefits:**
- ✅ **Unified build:** Same CMake builds for multiple platforms
- ✅ **Automation:** Python tasks reduce manual steps
- ✅ **Consistency:** All developers use same commands

**Trade-offs:**
- ❌ **Learning curve:** CMake is complex
- ❌ **Maintenance:** Build system needs updates as project evolves

### 10.4 Applicability

**CMake:**
- Multi-platform projects: **Highly recommended**
- Single-platform projects: **Optional** (PlatformIO, Makefile, Arduino IDE acceptable)

**Invoke:**
- Team projects: **Recommended** (simplifies workflows)
- Solo projects: **Optional** (shell scripts may suffice)

---

## 11. When to Adopt These Patterns

This section provides concrete guidance on which patterns to adopt based on project size, team size, and lifetime.

### 11.1 Small Projects

**Profile:** <3KLOC, 1 developer, <6 months

**Recommended patterns:**
- ✅ **Enter/exit/update state machines** (low overhead, prevents bugs)
- ✅ **Basic HAL for time/GPIO** (minimal effort, enables future testing)
- ✅ **Compiler warnings** (`-Wall -Wextra`, free bug detection)

**Optional patterns:**
- ⏸️ **Layered services** (flat structure acceptable for small projects)
- ⏸️ **Event bus** (direct function calls simpler)
- ⏸️ **Native testing** (hardware testing may suffice)
- ⏸️ **Static allocation** (malloc acceptable if RAM abundant)

**Skip patterns:**
- ❌ **Hierarchical FSM** (overkill for 2-4 states)
- ❌ **Event bus** (coupling not a problem yet)
- ❌ **CI/CD** (manual testing sufficient)

**Example:** Arduino-based temperature logger (read sensor, log to SD card, sleep). Use simple state machine (idle → read → log → sleep), basic HAL for millis(), direct function calls. Total effort: 2-3 days.

### 11.2 Medium Projects

**Profile:** 3-10KLOC, 2-3 developers, 6-24 months

**Recommended patterns:**
- ✅ **All small project patterns** (see above)
- ✅ **Layered services** (base/middleware/app)
- ✅ **Hierarchical FSM** (complexity justifies it)
- ✅ **HAL abstraction** (full GPIO/I2C/SPI/time)
- ✅ **Native testing** (ROI positive at this scale)
- ✅ **Clang-format** (consistency across team)
- ✅ **Status codes** (explicit error handling)

**Optional patterns:**
- ⏸️ **Event bus** (if single-threaded, direct calls may suffice)
- ⏸️ **Static allocation** (if RAM >128KB, malloc acceptable)
- ⏸️ **CI/CD** (valuable but not critical yet)

**Skip patterns:**
- ❌ **None** (all patterns provide value at this scale)

**Example:** IoT device with WiFi, sensors, actuators, display. Use layered services (base: FSM/logger, middleware: WiFi/sensor drivers, app: business logic), native tests for business logic, HAL for portability. Total effort: 2-3 weeks upfront (architecture), ongoing maintenance minimal.

### 11.3 Large Projects

**Profile:** >10KLOC, 3+ developers, >24 months

**Recommended patterns:**
- ✅ **All patterns** (every pattern provides value at this scale)
- ✅ **Layered services** (essential for team coordination)
- ✅ **Hierarchical FSM** (complex state management)
- ✅ **Event bus** (critical for decoupling)
- ✅ **Full HAL** (portability, testability)
- ✅ **Native testing + CI/CD** (regression prevention)
- ✅ **Static allocation** (determinism, safety)
- ✅ **Status codes** (error handling, logging)
- ✅ **Clang-format + Doxygen** (code quality)
- ✅ **CMake + invoke** (build automation)

**Optional patterns:**
- **None** (all patterns recommended)

**Example:** Commercial product (e.g., this system) with audio, WiFi, BLE, display, storage, battery management, OTA updates. Use all patterns for maximum maintainability, testability, and quality. Total effort: 4-6 weeks upfront (architecture), but pays off over 2+ years of development.

### 11.4 Decision Matrix

| Pattern | Small (<3KLOC) | Medium (3-10KLOC) | Large (>10KLOC) |
|---------|----------------|-------------------|-----------------|
| Enter/exit/update FSM | ✅ Required | ✅ Required | ✅ Required |
| Layered services | ⏸️ Optional | ✅ Recommended | ✅ Required |
| Hierarchical FSM | ❌ Skip | ✅ Recommended | ✅ Required |
| Event bus | ❌ Skip | ⏸️ Optional | ✅ Required |
| HAL abstraction | ⏸️ Basic | ✅ Full | ✅ Full |
| Native testing | ❌ Skip | ✅ Recommended | ✅ Required |
| Static allocation | ⏸️ Optional | ⏸️ Optional | ✅ Recommended |
| Status codes | ⏸️ Optional | ✅ Recommended | ✅ Required |
| Clang-format | ⏸️ Optional | ✅ Required | ✅ Required |
| Doxygen | ❌ Skip | ⏸️ Optional | ✅ Recommended |
| CMake + invoke | ❌ Skip | ⏸️ Optional | ✅ Recommended |
| CI/CD | ❌ Skip | ⏸️ Optional | ✅ Required |

**Key:**
- ✅ **Required:** Must adopt (high value, manageable cost)
- ✅ **Recommended:** Should adopt (clear benefits)
- ⏸️ **Optional:** Adopt if time permits
- ❌ **Skip:** Not worth effort at this scale

### 11.5 Adoption Strategy

**Incremental adoption:**
1. **Start simple:** Begin with enter/exit/update FSM, basic HAL
2. **Add as needed:** Adopt layered services when >3KLOC, event bus when coupling becomes a problem
3. **Don't over-engineer:** Avoid patterns you don't need yet

**Refactoring timing:**
- **Best:** Adopt patterns upfront (easier than refactoring)
- **Good:** Refactor when crossing thresholds (3KLOC, 10KLOC)
- **Acceptable:** Refactor when pain points emerge (coupling, untestable code)
- **Avoid:** Refactoring >10KLOC codebase is very difficult

**Team discussion:**
- Present this document to team
- Discuss project profile (size, lifetime, team size)
- Choose patterns collaboratively
- Document decisions (README or CLAUDE.md)

---

## 12. Summary & Further Reading

### 12.1 Key Takeaways

**1. Professional embedded systems use layered architecture:**
- Base services (FSM, event bus, logger)
- Middleware (hardware drivers, protocols)
- Application logic (business logic, UI)

**2. Hierarchical state machines simplify complex logic:**
- Enter/run/exit lifecycle prevents missed init/cleanup
- Parent/child relationships reduce duplication
- Table-driven dispatch scales cleanly

**3. Event buses decouple components:**
- Publish/subscribe enables loose coupling
- Static allocation avoids fragmentation
- Thread-safe with FreeRTOS integration

**4. HAL enables portability and testability:**
- Abstract interfaces + platform implementations
- Compile firmware for ESP32, STM32, native (PC simulator)
- Inject fake HAL for unit tests

**5. Native testing provides fast feedback loops:**
- 2-second tests vs 60-second flash + test
- Fake HAL enables deterministic timing
- CI/CD integration catches regressions

**6. Static allocation prevents fragmentation:**
- CREATE_CTX macros for compile-time allocation
- Const tables store read-only data in Flash
- Deterministic memory usage

**7. Status codes enable explicit error handling:**
- Functions return status_t instead of void/bool
- Error grouping by subsystem (ranges of 10)
- SERVICE_CHECK macro simplifies error propagation

**8. Code quality tools reduce debates:**
- Clang-format eliminates formatting debates
- Doxygen generates browsable documentation
- Compiler warnings catch bugs early

**9. CMake + Python automation simplifies builds:**
- Multi-platform builds (native, ESP32, simulator)
- Invoke tasks automate common workflows
- Auto-discovery finds tests/examples

**10. Adopt patterns incrementally:**
- Small projects (<3KLOC): Enter/exit/update FSM, basic HAL
- Medium projects (3-10KLOC): + Layered services, native testing
- Large projects (>10KLOC): All patterns (event bus, CI/CD, full HAL)

### 12.2 When to Adopt

| Pattern | Small | Medium | Large |
|---------|-------|--------|-------|
| Enter/exit/update FSM | ✅ | ✅ | ✅ |
| Layered services | ⏸️ | ✅ | ✅ |
| Hierarchical FSM | ❌ | ✅ | ✅ |
| Event bus | ❌ | ⏸️ | ✅ |
| HAL abstraction | ⏸️ | ✅ | ✅ |
| Native testing | ❌ | ✅ | ✅ |
| Static allocation | ⏸️ | ⏸️ | ✅ |
| Status codes | ⏸️ | ✅ | ✅ |
| Code quality tools | ⏸️ | ✅ | ✅ |
| CMake + automation | ❌ | ⏸️ | ✅ |

### 12.3 Further Reading

**Books:**
- **"Design Patterns for Embedded Systems in C"** by Bruce Powel Douglass
  - Comprehensive coverage of FSM, observer, command patterns
  - UML-based design methodology
- **"Making Embedded Systems"** by Elecia White
  - Practical advice on embedded architecture
  - Debugging, testing, optimization
- **"Test Driven Development for Embedded C"** by James W. Grenning
  - Unit testing strategies for embedded systems
  - CppUTest framework guidance

**Documentation:**
- **FreeRTOS documentation**: https://www.freertos.org/
  - Event groups, queues, task notifications
  - Real-time scheduling principles
- **CppUTest documentation**: https://cpputest.github.io/
  - Mocking, fakes, test fixtures
  - Memory leak detection
- **CMake documentation**: https://cmake.org/documentation/
  - Custom functions, cross-compilation
  - Multi-platform builds

**Articles:**
- **"Hierarchical State Machines"** by Miro Samek
  - Detailed explanation of HSM benefits
  - QP framework (commercial HSM implementation)
- **"Embedded Artistry: HAL Design"**
  - Best practices for HAL design
  - Function pointers vs compile-time selection

**Repositories:**
- **ESP-IDF**: ESP32 framework with excellent HAL design
- **Zephyr RTOS**: Modern RTOS with layered architecture
- **FreeRTOS**: Real-time operating system with proven patterns

### 12.4 Contributing to This Document

This document is a living reference. If you find errors, improvements, or want to add examples:
1. Suggest edits via pull request
2. Include code examples (simplified, commented)
3. Cite sources (books, articles, repos)

---

**End of document**

© 2026 Industry best practices compilation. All patterns documented here are established best practices in embedded systems development.
