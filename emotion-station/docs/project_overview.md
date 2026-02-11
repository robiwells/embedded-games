# Emotion Check-In Station

## Project Description

A screen-free, audio-first emotional regulation device for children aged 4-10 that uses physical NFC mood discs to provide instant, guided support when feelings become overwhelming.

---

## The Problem

**Children struggle with emotional regulation**, and parents often feel helpless during meltdowns, anxiety spirals, or bedtime worries. Existing solutions fall short:

- **Apps require screens** - Adding stimulation when calm is needed
- **Therapy is expensive** - £200+/hour and limited availability
- **Books require reading** - Not helpful when overwhelmed
- **Physical tools lack guidance** - Stress balls and fidgets don't teach coping skills

**What children need**: Instant, non-judgmental support that requires zero cognitive effort when emotions run high.

---

## The Solution

The Emotion Check-In Station is a beautiful, river-stone shaped device that sits on a child's bedside table or in a classroom calm corner. When a child feels a big emotion, they simply:

1. **Choose a mood disc** (happy, sad, angry, worried, frustrated, or calm)
2. **Place it on the device**
3. **That's it** - The device automatically selects and plays an appropriate calming activity

No buttons. No screens. No decisions. Just instant help.

---

## How It Works

### The Hardware

**Main Device:**
- Smooth, organic form (120mm diameter, 30mm tall)
- NFC reader embedded in the top surface
- 16 LED ring around the edge with breathing light patterns
- High-quality speaker for clear, gentle voice guidance
- Rechargeable battery (8-12 hours runtime)
- USB-C charging

**Mood Discs (6 pieces):**
- Beautiful wooden discs (60mm diameter)
- Each disc represents one emotion: 😊 Happy, 😢 Sad, 😠 Angry, 😰 Worried, 😤 Frustrated, 😌 Calm
- NFC chip embedded invisibly inside
- Engraved icon and word for easy identification
- Colour-coded edge accents
- Tactile, satisfying weight

### The Experience

**Scenario 1: After-School Anger**

```
8-year-old Jake comes home frustrated after a bad day at school.

1. He picks up the "Angry" disc and places it on the device
2. Device validates: "I hear you're feeling angry. Let's work through that together."
3. LEDs pulse from red → orange → yellow → green (calming progression)
4. Activity auto-plays: "Let's do volcano breaths together..."
5. Jake follows the 2-minute guided breathing exercise
6. Device: "Good job. You can take the disc away when you're ready."
7. Jake removes the disc feeling calmer
```

**Scenario 2: Bedtime Worries**

```
6-year-old Emma can't sleep due to worries about tomorrow.

1. She places the "Worried" disc on the device
2. Device: "It sounds like you're feeling worried. I'm here to help."
3. LEDs show gentle purple breathing pattern
4. Device plays "Night Guard" - a 4-minute protective visualisation story
5. Emma falls asleep with the disc still on the device
6. When she removes it in the morning, the check-in is logged
```

**Scenario 3: Quick Check-In**

```
5-year-old Mia just wants to acknowledge her feelings.

1. She places the "Happy" disc on the device
2. Device: "I can tell you're feeling happy! Let's celebrate that."
3. After 2 seconds, she removes the disc
4. Device: "I see you're feeling happy. I'm here when you need me."
5. No full activity plays - just validation
```

### The Intelligence

**Smart Activity Selection:**

The device has a library of **44 evidence-based activities**:
- 8 activities for Angry (volcano breaths, stomp it out, squeeze and release...)
- 8 activities for Worried (worry box, grounding exercise, safe place visualisation...)
- 8 activities for Sad (hug story, gentle breaths, rainbow visualisation...)
- 6 activities for Happy (dance party, gratitude game, celebration song...)
- 8 activities for Frustrated (push through story, problem-solving steps...)
- 6 activities for Calm (stay calm, nature sounds, gentle movement...)

**Selection algorithm considers:**
1. **Time of day** - Energetic activities in morning, calming at bedtime
2. **Recent history** - Avoids repeating the same activity twice in a row
3. **Random variation** - Keeps engagement high with variety

**LED breathing patterns:**
- Each mood has a unique colour (red for angry, blue for sad, purple for worried...)
- Breathing exercises sync LED pulses with inhale/exhale timing
- Idle state shows gentle breathing pattern (inviting, not demanding)

### The Data

Every check-in is logged to help parents understand patterns:
- Timestamp (when did the emotion occur?)
- Mood selected
- Activity played
- Completion status
- Duration of session

**Parent companion app** (future phase) will show:
- Weekly emotion frequency charts
- Peak times for specific emotions ("anger after school pattern")
- Activity effectiveness
- Conversation starters ("Your child felt frustrated 3 times this week...")

---

## Why This Matters

### For Children
- ✅ **Autonomy** - They can self-regulate without adult intervention
- ✅ **Non-judgmental** - All feelings are validated
- ✅ **Instant help** - No waiting when emotions are big
- ✅ **Skill building** - Learns evidence-based coping techniques
- ✅ **Privacy** - No recording, no microphone, no surveillance
- ✅ **Empowerment** - Builds confidence in managing emotions

### For Parents
- ✅ **Peace of mind** - Child has tool when parent unavailable
- ✅ **Insight** - Understand emotional patterns
- ✅ **Conversation starters** - Data helps open dialogue
- ✅ **Consistency** - Same techniques every time
- ✅ **Break from screens** - Audio-first, no added screen time
- ✅ **Accessible therapy** - Reinforces therapeutic techniques at home

### For Educators
- ✅ **Calm corner tool** - Self-guided regulation in classroom
- ✅ **Reduces disruption** - Children can self-soothe quickly
- ✅ **SEL integration** - Supports social-emotional learning curriculum
- ✅ **Multiple children** - One device serves whole class
- ✅ **No supervision needed** - Works independently

---

## The Design Philosophy

### Radical Simplicity

**One input method**: Place disc on device
**Zero decisions**: Device automatically selects activity
**No cognitive load**: Perfect for overwhelmed children
**Instant help**: No menus, no navigation, no waiting

### Audio-First Experience

**No screens**: Pure audio guidance with calming voice
**High-quality speaker**: Clear, gentle voice quality
**Background music**: Supports activities when appropriate
**LED breathing light**: Only visual feedback needed

### Emotional Safety

**Non-judgmental**: All feelings are valid and accepted
**Private**: No recording, no microphone, no data harvesting
**Calming design**: Organic form, warm materials, inviting aesthetic
**Always available**: No setup, no login, no barriers

### Beautiful Object

Inspired by **Yoto Player's** design philosophy - this is a device that belongs in your home:
- Warm wood or soft-touch materials
- Organic, river-stone shape (feels natural, not technological)
- Diffused LED glow (not harsh or attention-grabbing)
- Weighted feel (substantial, premium quality)
- Retail-ready aesthetic (living room or bedroom worthy)

---

## The Technology

### Hardware Components

**Core System:**
- **ESP32 microcontroller** - WiFi/Bluetooth enabled
- **PN532 NFC reader** - Reads NTAG215 tags
- **MAX98357A I2S DAC** - High-quality audio output
- **5W speaker** - Clear voice reproduction
- **WS2812B LED ring** - 16 programmable RGB LEDs
- **MicroSD card** - Stores all audio content
- **2000mAh LiPo battery** - 8-12 hour runtime
- **TP4056 charging module** - USB-C charging

**Estimated BOM**: £35-40 at scale, £60 for prototype

### Software Architecture

**State Machine Design** (proven from previous embedded projects):
```
IDLE → DETECTED → PLAYING → COMPLETE → IDLE
```

**Key Features:**
- Non-blocking architecture (watchdog timer safe)
- Enter/exit/update pattern for clean state management
- Parallel LED and audio animations
- Activity history tracking to prevent repeats
- CSV data logging to SD card
- WiFi sync capability (future)

**Activity Library Structure:**
- JSON-defined activity metadata
- MP3 audio files on SD card
- Time-of-day flags per activity
- Duration and type tagging
- Easy to add new content via SD card update

### Development Stack

- **Firmware**: Arduino/PlatformIO (C++)
- **Libraries**:
  - Adafruit PN532 (NFC)
  - Adafruit NeoPixel (LEDs)
  - ESP32-audioI2S (MP3 playback)
  - ArduinoJson (data structures)
- **Parent App** (future): React Native or Flutter
- **Cloud Backend** (future): Node.js + PostgreSQL

---

## Market Opportunity

### Target Markets

**Primary: Parents (Home Use)**
- Parents of children 4-10 years old
- Concern about screen time
- Interest in emotional intelligence
- Willingness to pay: £60-100
- Market size: 10M+ families (UK/US)

**Secondary: Schools (Counsellor Offices)**
- School counsellors, SEL coordinators
- Need for calm corner tools
- Budget for wellness resources
- Willingness to pay: £80-150 (bulk)
- Market size: 100K+ schools

**Tertiary: Therapists (Clinical Use)**
- Child psychologists, OTs
- Teaching coping skills homework
- Clinical validation important
- Willingness to pay: £100-200
- Market size: 50K+ practitioners

### Competitive Advantage

**vs Yoto Player**: Different use case (wellness vs entertainment), complementary
**vs Apps**: Screen-free, instant access, no distractions
**vs Therapy**: 24/7 availability, affordable, reinforces learned techniques
**vs Calm Corner Kits**: Guided activities, audio instruction, consistent support

**Unique Value Proposition:**
"The only screen-free, audio-first emotional regulation device that provides instant, automatic support when children need it most."

---

## Why Now?

### Market Trends

- **Mental health crisis** - Children's anxiety/depression at record highs
- **Screen time concerns** - Parents seeking non-digital alternatives
- **SEL in schools** - Social-emotional learning mandated in many districts
- **Audio renaissance** - Success of Yoto, audiobooks, podcasts
- **Physical digital products** - Yoto, Toniebox proving market exists

### Enabling Technology

- **ESP32 availability** - Powerful, affordable microcontrollers
- **NFC ubiquity** - Mature, reliable, low-cost
- **Audio processing** - I2S libraries make high-quality audio easy
- **Manufacturing access** - Low MOQs for injection molding, PCB fabrication

### Personal Motivation

This project emerged from a deep interest in:
- **Audio-first interaction design** (inspired by work at Yoto)
- **Physical computing** (embedded systems, tangible interfaces)
- **Child wellness** (making therapy techniques accessible)
- **Beautiful product design** (technology that belongs in homes)

The goal: Create something that genuinely helps children and families while exploring the intersection of physical interaction, audio design, and emotional intelligence.

---

### Technical
- [ ] WiFi vs Bluetooth for parent app sync?

