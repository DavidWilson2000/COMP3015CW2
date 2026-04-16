# Dredge-Style Fishing Prototype (Coursework 2)

## Overview

This project is a Dredge-inspired OpenGL fishing game prototype built in C++ using GLFW, GLAD, GLM, custom GLSL shaders, and irrKlang for audio.

The project combines graphics programming techniques from the shader lectures with a playable game loop. Rather than presenting isolated shader demos, the work integrates rendering, post-processing, shadows, particles, UI, audio, and progression systems into a complete fishing game experience.

The player sails between fishing zones, catches fish, sells cargo at the dock, upgrades the boat, survives dangerous water, collects three hidden keys, unlocks the Lost Island, and reaches the final shrine to win.

The main goal of Coursework 2 was to combine multiple graphics features into an original scene that behaves like a game rather than a static rendering showcase.

---

## How to run the executable

1. Open the Visual Studio solution for the project.
2. Build the project in the correct configuration for your machine.
3. Run the executable from Visual Studio, or run the built `.exe` from the output folder.
4. Keep the resource folders in the correct relative paths, especially:
   - `shader/`
   - `media/models/`
   - `media/sounds/`
   - `media/fish/`
   - `textures/`

The project relies on relative asset paths, so shaders, textures, sounds, models, and fish PNGs must remain in the expected folders.

---

## Controls

### Core movement
- **W / S** = move forward / backward
- **A / D** = steer left / right

### Fishing and economy
- **E** = fish / cast
- **M** = toggle fishing minigame on or off
- **Space** = hook during fishing minigame
- **R** = sell cargo at the dock
- **1** = buy rod upgrade
- **2** = buy engine upgrade
- **3** = buy cargo upgrade
- **4** = repair hull at the dock

### Menus and progression
- **Enter** = start game
- **P** = pause / help screen
- **J** = open fish journal
- **Left / Right Arrow** = change fish journal page
- **O** = open / close settings menu
- **Up / Down** = move through settings menu
- **Left / Right** = adjust selected setting
- **R** (inside settings menu) = reset settings to defaults

### Camera
- **C** = toggle camera mode
- **Arrow Keys** = look around in free-look camera mode

### Shader / rendering controls
- **F5** = edge detection
- **F6** = blur
- **F7** = night vision
- **F8** = normal rendering
- **F9** = toggle shadow filter (**PCF / PCSS**)
- **F10** = toggle sun cycle
- **F11** = god rays

### Other
- **Esc** = quit
- **G** = debug gold hotkey used during testing

---

## High-level design

I designed this as a playable atmospheric fishing game rather than a single-scene shader test.

The project has three main layers:

1. **Rendering layer**  
   Handles shaders, textures, shadow mapping, post-processing, water, skybox, particles, lighting, and fullscreen effects.

2. **Gameplay layer**  
   Handles fishing, upgrades, cargo, hull damage, danger, key collection, Lost Island unlock, and victory conditions.

3. **Feedback / interface layer**  
   Handles the HUD, fish journal, pause/help screens, catch cards, banners, quest display, victory presentation, and settings menu.

The main design goal was to make the rendering work support the gameplay loop. Zone fog, water tint, danger, audio layering, shadow presentation, and post-processing are all tied to player experience rather than being disconnected technical demos.

---

## Main program flow

The project is orchestrated from `main.cpp`.

This file:
- creates the OpenGL window
- loads shaders
- loads models and textures
- sets up shadow mapping
- sets up the post-processing pipeline
- initialises audio and UI
- registers fish journal data
- runs the main update/render loop

Inside the loop, the game:
- processes input
- updates the boat and fishing systems
- updates danger, hull damage, and quest progression
- updates particles and audio
- renders a shadow depth pass
- renders the world scene
- applies post-processing
- builds and renders the UI
- renders the settings menu when opened

This makes `main.cpp` the central coordinator for the project, while individual systems remain separated into their own files to keep responsibilities clearer.

---

## Main classes / systems

### `FishingMinigame`
This class manages the optional fishing timing minigame. It handles:
- whether the minigame is enabled
- whether it is active
- marker timing
- success/failure logic
- “good hook” and “perfect hook” style outcomes

### `IslandQuest`
This class manages progression through the three key objectives and the final Lost Island unlock. It tracks:
- collected keys
- unlock state
- win state
- final island goal position
- quest text shown in the HUD
- fog behaviour that hides the Lost Island before it is unlocked

### `SoundManager`
This wraps irrKlang and handles:
- sound system setup
- background loop
- boat motor audio
- splash and reel effects
- zone-layer ambience
- audio category controls for:
  - master volume
  - music volume
  - SFX volume

The background ambience remains active, while biome-specific loops fade in and out as the player approaches or leaves different island regions.

### `UIOverlay`
This system renders the custom HUD and menu screens. It uses a `HUDState` structure to receive all data needed for:
- gold
- cargo
- upgrades
- hull integrity
- danger
- quest state
- caught fish card
- journal pages
- start, pause, and victory overlays

### `GameSettingsMenu`
This is a separate settings system used to keep the main file cleaner. It stores and renders:
- brightness
- master volume
- music volume
- SFX volume

This improves usability and presentation while also making the project feel more like a complete game rather than only a prototype scene.

### `PostProcessor`
This handles the fullscreen post-process pass and applies:
- edge detection
- blur
- night vision
- god rays

### `LostIslandSetpiece`
This builds the final altar / shrine transforms and sword placement for the Lost Island.

---

## Gameplay loop

The gameplay loop works as follows:

1. The player starts at the dock.
2. They sail into fishing zones.
3. Each zone has a different fish pool, water tint, fog values, danger level, and audio mood.
4. The player catches fish either instantly or through the optional fishing minigame.
5. Fish are stored as cargo.
6. The player returns to the dock to sell cargo for gold.
7. Gold is used for rod, engine, cargo, and repair progression.
8. Dangerous water damages the hull and can cause engine stutter or forced return to dock.
9. At sufficient rod progression, the player can collect three hidden keys.
10. Once all keys are collected, the Lost Island unlocks.
11. Reaching the Lost Island triggers the final win state and celebration.

This was designed to make the coursework feel like a real game rather than a graphics-only sandbox.

---

## Rendering pipeline

The rendering pipeline is split into multiple passes and shader programs.

### 1. Depth pass
The scene is rendered from the light’s point of view into a depth texture.  
This depth texture is then sampled during the main scene pass for shadow calculation.

### 2. Main lit scene pass
The world geometry is rendered using the main lit shader.  
This includes:
- dock geometry
- island geometry
- boat
- Lost Island setpiece
- buoys
- world props

### 3. Water pass
The animated water plane is rendered separately using a dedicated water shader.

### 4. Skybox pass
The skybox is rendered to provide atmosphere and make the scene feel less flat.

### 5. Particle pass
Point-based particles are rendered for:
- boat wake
- splash feedback
- ambient zone particles
- Lost Island aura particles
- win-state celebration effects

### 6. Post-process pass
The rendered scene is passed through a fullscreen post-process stage which can apply image-space effects.

---

## Advanced graphics features demonstrated

This project combines multiple techniques from the shader rubric into one pipeline rather than implementing only one isolated effect.

### Shadow mapping
The scene uses a dedicated depth pass and shadow map texture so shadows can be sampled in the main lit pass.

### PCF / PCSS shadow filtering
The shadow system supports a live toggle between:
- **PCF** for a simpler filtered shadow result
- **PCSS** for softer penumbra-style shadows

This makes the shadows more visually convincing than a single hard shadow compare and gives a clear demonstration feature for the video.

### Dynamic sun cycle
The lighting system includes a time-driven sun cycle that changes:
- light direction
- light colour
- intensity
- ambient contribution

This means the scene lighting changes over time rather than remaining static.

### God rays
A fullscreen post-process mode computes light shafts based on the sun’s screen position and visibility.  
This gives the scene a stronger atmospheric effect when the camera faces toward the bright sun direction.

### Additional image processing
The post-process system also supports:
- edge detection
- blur
- night vision

### Animated water
The water surface is animated over time and is lit separately from the rest of the world.  
It also responds to zone tint, fog, and mood settings.

### Particle effects
Particles are used both as atmosphere and gameplay feedback:
- wake particles behind the boat
- splash particles on fish catch
- aura particles around the Lost Island
- victory particles on completion

### Skybox and atmosphere
The project includes a skybox and dynamic fog/water colour blending to make each zone feel visually distinct.

### Adjustable brightness
A settings menu allows the player to adjust overall scene brightness without changing the underlying shader modes. This improves usability and also shows consideration for presentation and accessibility.

---

## Audio design

Audio is no longer only a single background loop.

The project now layers audio in the following way:
- a main background ambience remains active
- the boat motor fades based on speed
- splash and reel SFX play during interaction
- biome-specific ambient layers fade in and out as the player approaches or leaves island zones
- Lost Island uses its own audio mood
- the settings menu allows:
  - master volume control
  - music volume control
  - SFX volume control

This improves atmosphere and helps address one of the weaknesses from the earlier coursework feedback, where more background music/audio would have helped.

---

## What makes this project special

The strongest part of the project is that the shader work is not isolated from the gameplay.

Examples:
- different fishing zones also change fog, tint, danger, and ambience
- the Lost Island is hidden by progression-based fog until unlocked
- shadows affect both the world and the water presentation
- post-processing can be toggled during live play for direct inspection
- settings menu controls brightness and audio
- UI feedback makes the gameplay systems readable instead of hidden
- atmosphere, progression, rendering, and sound are all tied together

A simpler coursework solution could have shown one object with one shader.  
This project instead behaves like a coordinated game scene with progression, feedback, atmosphere, and multiple interacting systems.

---

## Research sources and originality

This coursework uses ideas inspired by shader research/tutorial material, but the implementation was adapted to fit this specific project and integrated into the provided lab-style framework.

The two key research-style features I focused on are:

### 1. PCSS soft shadows
I used the idea of moving from simple filtered shadow mapping toward contact-hardening style soft shadows.  
My implementation adapts that idea into this coursework project so it can be toggled live against a simpler filtered version, making the difference visible during demonstration.

### 2. God rays / light shafts
I used the idea of radial light sampling in screen space to create sun shafts.  
Rather than building a physically complete volumetric lighting system, I implemented a coursework-appropriate fullscreen version that fits the existing pipeline and works with the game’s sun direction logic.

What makes the project original is not just the isolated shader ideas, but the fact that they are integrated into:
- a fishing game loop
- a quest system
- a full UI
- a settings menu
- audio layering
- danger and damage systems
- zone-based atmosphere
- a final unlockable objective

### Starting point and adaptation
The project began as a continuation of my CW1 work, but it was expanded well beyond the earlier shrine scene. My CW1 feedback noted that the earlier project had basic gameplay, limited UI, and a report that did not explain the implementation deeply enough. In this version I intentionally addressed those points by building a much fuller gameplay loop, stronger HUD/menu systems, better atmosphere/audio, and more advanced rendering features. :contentReference[oaicite:2]{index=2}

---

## Software engineering decisions

### Modularity
The project is split into systems for:
- fishing minigame logic
- quest logic
- audio
- settings menu
- UI/HUD rendering
- post-processing
- Lost Island setpiece transforms

This made the project easier to maintain and debug.

### UI state passing
The UI does not directly query all game systems itself.  
Instead, the game builds a `HUDState` object and passes it into the UI renderer.  
This keeps the rendering layer cleaner and reduces tight coupling.

### Separate settings menu file
The settings menu was deliberately built in its own file to avoid overloading `main.cpp` even further. That kept input handling and rendering integration cleaner than mixing all menu code into the main loop.

### Fallback resources
Some textures use procedural fallback generation when file loading fails.  
This improved robustness during development.

### Relative resource paths
The project relies on relative paths, which is simple for coursework use but requires careful packaging when submitting.

---

## Performance / design trade-offs

This project balances visual quality with practical coursework scope.

Examples:
- shadow mapping adds depth but requires an additional depth render pass
- PCSS produces better-looking shadows but costs more than simpler PCF
- post-processing improves scene style but adds a fullscreen pass
- particles improve feedback and atmosphere but add update/render overhead
- layered biome audio improves atmosphere but adds extra looping sounds to manage
- custom UI and settings menus give full control but take more development effort than using a library

For this coursework, these trade-offs were worthwhile because they made the final result more convincing and game-like.

---

## Evaluation

The strongest part of the project is that it now works as a complete prototype rather than only a collection of graphics features.

The final version includes:
- sailing and navigation
- multiple fishing zones
- optional fishing minigame
- cargo and selling
- upgrades
- hull damage and repair
- quest progression
- journal and fish tracking
- dynamic lighting and shadows
- post-processing
- particle feedback
- layered biome audio
- settings for brightness and volume
- a final Lost Island objective
- a victory state

I am especially happy with how the UI and feedback systems developed, because they now communicate much more than the earliest prototype.

If I had more time, I would improve:
1. packaging and asset handling
2. persistence / save systems
3. deeper fish behaviour differences by biome
4. more bespoke audio events within each zone
5. even stronger inline shader documentation with direct source references

---

## What I would do differently

Knowing what I know now, I would:
- lock the final merged gameplay/render branch earlier
- document each render pass as it was added
- keep a stricter asset naming convention
- keep a more formal record of which research techniques were adapted and how
- define the final settings/menu scope earlier in development

One of the biggest challenges of this coursework was not writing one shader, but integrating many systems without breaking the earlier working build.

---

## Video report

Video walkthrough and explanation:  
**[PASTE YOUR UNLISTED YOUTUBE LINK HERE]**

The video should show:
- running the executable
- all controls
- sailing between zones
- fishing with and without the minigame
- cargo selling and upgrades
- hull danger and repair
- fish journal pages
- settings menu use
- brightness adjustment
- volume controls
- key collection and Lost Island unlock
- reaching the Lost Island
- **F9** PCF vs PCSS
- **F10** sun cycle
- **F11** god rays
- **F5–F8** post-process modes
- **C** camera toggle and free-look

---

## Public GitHub repository

Public GitHub repository:  
**https://github.com/DavidWilson2000/COMP3015CW2**

---

## Generative AI use declaration

This assessment is marked as Partnered Work for GenAI use.  
Generative AI was used as support for:
- code assistance
- debugging support
- programming testing support
- README/report drafting support

All final integration, selection, testing, and submission decisions were made by me.

---

## Repository contents

Important files and folders include:
- `main.cpp`
- `FishingMinigame.h / .cpp`
- `IslandQuest.h / .cpp`
- `SoundManager.h / .cpp`
- `GameSettingsMenu.h / .cpp`
- `UIOverlay.h / .cpp`
- `PostProcess.h / .cpp`
- `LostIslandSetpiece.h / .cpp`
- `shader/`
- `media/models/`
- `media/sounds/`
- `media/fish/`
- `textures/`

---

## Notes for marker

- The project uses multiple shader programs and render passes rather than a single global shader.
- The project combines graphics features with gameplay systems.
- The UI, menu, and settings systems are custom rendered.
- Audio is handled through irrKlang with layered biome ambience.
- The final objective is to collect three keys, unlock the Lost Island, and reach it.
- The most important advanced rendering additions are PCSS soft shadows, the sun cycle, and god rays.