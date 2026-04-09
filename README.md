# Dredge-Style Fishing Prototype (Coursework 2)

## Overview

This project is a Dredge-inspired OpenGL fishing game prototype built in C++ using GLFW, GLAD, GLM, custom GLSL shaders, and irrKlang for audio. The scene combines graphics features from the shader lectures with game mechanics such as fishing, cargo management, upgrades, a key-based quest, failure states, UI/HUD systems, and a final win condition.

The project starts as a small coastal fishing scene, but it is designed as a complete playable loop rather than only a rendering demo. The player sails between fishing zones, catches fish, sells cargo at the dock, upgrades the boat, survives dangerous water, collects three hidden keys, unlocks the Lost Island, and then reaches the final shrine to win.

The main goal of Coursework 2 was not only to show shader work, but to integrate it into an original scene and make it behave like a game. My project therefore combines multiple shader and rendering techniques with clear mechanics, progression, feedback, and UI systems.

---

## How to run the executable

1. Open the Visual Studio solution for the project.
2. Build the project in the correct configuration for your machine.
3. Run the executable from Visual Studio, or run the built `.exe` from the output folder.
4. Make sure the resource folders remain in the correct relative paths, especially:
   - `shader/`
   - `media/models/`
   - `media/sounds/`
   - `media/fish/`
   - `textures/`

The program expects shader files to be loaded from the `shader` folder and also loads models, textures, sounds, and fish PNGs from relative paths. If those folders are missing or moved, the program may compile but parts of the scene will fail to render or play correctly.

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

### Shader / rendering modes
- **F5** = edge detection post-process
- **F6** = blur post-process
- **F7** = night vision post-process
- **F8** = return to normal rendering

### Other
- **Esc** = quit
- **G** = debug gold hotkey used during testing

---

## High-level design

I designed this as a playable atmospheric fishing game rather than a single-scene shader test. The project is built around three layers working together:

1. **Rendering layer**  
   Handles models, shaders, textures, water, sky, shadows, particles, and post-processing.

2. **Gameplay layer**  
   Handles fishing, upgrades, selling, hull damage, quest progression, journal progression, and victory conditions.

3. **Feedback / interface layer**  
   Handles HUD panels, quest display, menus, caught fish cards, journal pages, catch banners, and overall player communication.

The goal was to make the rendering choices support the gameplay loop. For example, zone fog and water tint are not only decorative; they help make each area feel distinct and communicate mood and danger.

---

## How the code works

## Main program flow

The project is orchestrated from `main.cpp`. This file creates the OpenGL window, loads all shader programs, loads textures and models, sets up post-processing and shadow mapping, initialises the UI, registers fish journal data, and runs the main update/render loop.

Inside the game loop, the code:
- reads input
- updates game state
- updates audio
- updates particles
- updates quest logic
- updates danger / hull systems
- builds the HUD state
- renders the world
- renders post-processing
- renders the UI

This makes `main.cpp` the central coordinator for the project.

---

## Main classes / systems

### `FishingMinigame`
This class manages the optional fishing timing minigame. It handles:
- whether the minigame is enabled
- whether it is currently active
- marker movement
- success/failure timing
- “good hook” / “perfect hook” result messages

This class is deliberately separated from `main.cpp` so the fishing challenge logic stays modular and reusable.

### `IslandQuest`
This class manages the game’s key quest and final win condition. It tracks:
- the three collectible keys
- whether the Lost Island is unlocked
- whether the player has won
- the final goal position and radius
- quest summary text for the HUD
- fog behaviour that hides the Lost Island before it is unlocked

This class is important because it turns the prototype into a goal-driven game rather than an infinite fishing sandbox.

### `SoundManager`
This class wraps irrKlang and keeps audio management separate from gameplay logic. It:
- initialises the sound engine
- loads audio from `media/sounds`
- plays looping background audio
- plays boat motor audio
- plays reel and splash effects
- scales the boat motor sound based on player speed

Separating this logic into its own class made the rest of the code cleaner and easier to debug.

### `UIOverlay`
This system renders the HUD and menu screens. It stores a `HUDState` structure containing everything the UI needs, such as:
- gold
- cargo
- rod / engine levels
- repair cost
- quest state
- keys collected
- hull integrity
- speed and danger
- menu flags
- fish journal entries
- caught fish card data
- victory screen data

This means the UI is not directly reading game systems itself. Instead, the game builds a snapshot of state and passes it into the renderer, which is a cleaner design.

### `PostProcessor`
This handles the full-screen post-processing pipeline and applies the edge detect, blur, and night vision effects after the 3D scene has already been rendered.

### `LostIslandSetpiece`
This file is responsible for the final altar / sword scene at the Lost Island. It builds the transforms for the stepped shrine layout and the sword placement.

---

## How the gameplay systems fit together

The gameplay loop works as follows:

1. The player starts at the dock.
2. They sail into fishing zones.
3. Each zone has its own fish pool, colour tint, fog values, and danger level.
4. The player catches fish either instantly or through the optional minigame.
5. Fish are stored as cargo.
6. The player returns to the dock to sell cargo and earn gold.
7. Gold is used for rod, engine, and cargo upgrades.
8. Dangerous areas damage the hull and can cause engine stutter or forced return to the dock.
9. At high enough rod level, the player can fish up hidden keys in each major zone.
10. Once all three keys are collected, the Lost Island unlocks.
11. Reaching the Lost Island triggers the win state and final celebration.

This structure gave the project real progression and persistence, which I felt was necessary for the rubric’s gamification requirement.

---

## How the rendering and shader systems fit together

The rendering pipeline is split into several shader programs rather than relying on one generic shader.

### 1. Basic lit shader
Used for most world geometry such as islands, dock elements, the boat, and set pieces.

### 2. Water shader
Used only for the animated water plane. This shader uses time, fog, tint, danger, and lighting inputs to make the ocean feel alive and responsive to the current zone.

### 3. Depth shader
Used for the shadow map pass. The scene is rendered once from the light’s point of view into a depth texture, then that texture is sampled in the main scene pass.

### 4. Skybox shader
Used for the sky and atmospheric background. This helps the scene feel less flat and gives a stronger mood than a plain clear colour.

### 5. Particle shader
Used for splashes, wake particles, aura particles, and other point-based effects.

### 6. Post-process shader
Used after the main scene is rendered, allowing the entire frame to be processed as an image effect.

This structure is important because it shows that the project is not a single-shader scene. It is a small graphics pipeline with multiple specialised passes.

---

## What makes the shader program special

The most distinctive part of this project is that the shader work is not isolated from the gameplay. Instead, the rendering features are tied directly into the player’s experience.

### Examples of this integration
- Fishing zones are not only different in fish types; they also change fog, water tint, and danger, making each area visually distinct.
- The Lost Island uses quest-based fog logic, so the atmosphere changes based on progression.
- Shadows are used in both the main lit pass and the water pass.
- Post-processing can be toggled during play so the effects are inspectable in real time.
- Particle systems support both atmosphere and feedback, such as wakes, splashes, shrine aura, and win effects.
- The HUD communicates all core systems live, including quest progress, danger, hull, upgrades, and journal progression.

A simpler version of this coursework could have been “one object + one shader + a camera.” My project instead behaves like a coordinated game scene where graphics, controls, mechanics, and feedback all support each other.

---

## Which advanced shader topics are demonstrated

This project intentionally mixes multiple topics from the shader rubric rather than only implementing one isolated effect.

### Image processing
- Edge detection
- Blur
- Night vision
- Full-screen post-processing pipeline

### Geometry / particle style work
- Point-sprite style particle rendering for wakes, splashes, aura effects, and atmosphere

### Vertex / animated surface work
- Animated water surface driven by time-based wave motion

### Shadows
- Shadow mapping with a dedicated depth pass

### Noise / atmospheric visual variation
- Procedural texture generation for fallback materials
- Zone-based fog, colour tint, and sky/water mood variation

Because these techniques are combined in one project, the visual result is much richer than a basic lab exercise.

---

## Why I chose this design

I wanted the project to feel cohesive rather than disconnected. The Dredge-inspired theme gave me a strong reason to combine:
- moody lighting
- fog
- water animation
- audio atmosphere
- UI feedback
- risk/reward mechanics
- progression
- a mystery ending

That made it easier to justify technical choices. For example, fog was not added only because it looks good; it also supports navigation, tension, and the hidden-island reveal. The same applies to the danger system, the quest system, and the journal.

---

## Software engineering decisions

## Modularity
I separated the code into multiple files/classes instead of placing everything directly in the render loop. This improved readability and made debugging easier. The most useful separations were:
- fishing minigame logic
- quest logic
- sound logic
- UI/HUD rendering
- Lost Island setpiece generation

## Clear state passing into UI
The HUD does not read directly from every system. Instead, the game builds a `HUDState` object and passes it into the UI renderer. This is cleaner and makes it easier to expand the interface without creating tight coupling.

## Fallback systems
For several textures, the project uses fallback procedural generation if the external image cannot be loaded. This helped keep the project robust during development.

## Resource path trade-off
The project currently relies on relative file paths. This is convenient during development, but it also means the executable depends on the repo structure being preserved correctly. I would keep this in mind when packaging the final submission.

---

## Performance / engineering trade-offs

A key trade-off in this project is visual polish versus simplicity.

### Examples
- Real-time shadows add visual depth but require an extra render pass.
- Post-processing improves the look of the scene but adds a screen-space pass after rendering.
- Particle effects improve feedback and atmosphere but increase per-frame update/render work.
- The UI system is entirely custom and lightweight, but it takes more effort to maintain than using a library.
- Relative asset loading is simple for coursework use, but less robust than a fully packaged asset pipeline.

For a coursework project, I think these trade-offs were worthwhile because they made the final scene much more convincing while staying manageable.

---

## Evaluation

I believe the strongest part of this project is that it goes beyond “graphics feature collection” and becomes a playable experience. The final version includes:
- a full movement and fishing loop
- a dock economy
- upgrades
- danger and hull damage
- a timing minigame
- fish progression and journal tracking
- shader-based presentation and post-processing
- an unlockable final area
- a win state with feedback and presentation

I am also pleased with how the UI grew over time. It now communicates far more than the earliest prototype: progression, quest state, hull state, journal data, fish cards, and menu overlays.

If I had more time, the main things I would improve are:
1. a more robust asset and packaging system
2. more bespoke audio per zone
3. even more distinct fish behaviour per region
4. a cleaner persistence/save system
5. a deeper final pass on documentation inside the codebase itself

Even so, I think the submitted version successfully demonstrates both technical shader work and game design thinking.

---

## What I would do differently

Knowing what I know now, I would:
- lock the merged main branch earlier to avoid integration conflicts
- define the full UI/menu scope earlier
- keep a stricter asset naming convention from the start
- document each shader pass as it was added, rather than reconstructing some of that explanation later

Those lessons were useful, because one of the real challenges of this coursework was not just writing shaders, but integrating many systems without breaking previous work.

---

## Video report

Video walkthrough and explanation:  
**[PASTE YOUR VIDEO LINK HERE]**

The video should demonstrate:
- opening and running the executable
- all controls
- sailing between zones
- fishing with and without the minigame
- post-process toggles
- shadows / water / particles / sky
- cargo selling and upgrades
- danger / hull / repair
- journal pages
- key collection and Lost Island unlock
- reaching the Lost Island and the win state

---

## Final reflection

Overall, this project started from the lab template and lecture material, but I developed it into an original Dredge-style prototype with its own loop, progression, atmosphere, and final objective. My aim was to make the rendering techniques feel purposeful inside a game-like structure rather than detached from it. I believe that is the project’s main strength.

---

## Repository contents

Suggested important folders/files:

- `main.cpp`
- `FishingMinigame.h / .cpp`
- `IslandQuest.h / .cpp`
- `SoundManager.h / .cpp`
- `UIOverlay.h / .cpp`
- `LostIslandSetpiece.h / .cpp`
- `shader/`
- `media/models/`
- `media/sounds/`
- `media/fish/`
- `textures/`

---

## Notes for marker

- The project uses multiple shader programs rather than a single global shader.
- The project includes both graphics features and gameplay systems.
- The UI and menu systems are custom-rendered.
- Audio is handled through irrKlang.
- The final objective is to collect three keys, unlock the Lost Island, and reach it.
