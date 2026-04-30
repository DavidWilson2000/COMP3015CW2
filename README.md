# Dredge-Style Fishing Prototype — COMP3015 Coursework 2

## Overview

This project is a **Dredge-inspired OpenGL fishing game prototype** built in C++ using GLFW, GLAD, GLM, custom GLSL shaders, and irrKlang audio.

The project was designed as a playable game scene rather than a static shader showcase. The player sails a small boat between fishing zones, catches and sells fish, upgrades the boat, survives dangerous waters, collects three hidden keys, unlocks the Lost Island, and reaches the final shrine to win.

The main aim of the coursework was to combine advanced shader features with an original interactive scene. The final project integrates rendering, procedural world generation, post-processing, shadows, particles, UI, audio, settings, and gameplay progression into one complete prototype.

---

## Public repository

GitHub repository:  
**https://github.com/DavidWilson2000/COMP3015CW2**

---

## Video report

Video walkthrough and explanation:  
**[PASTE YOUR UNLISTED YOUTUBE LINK HERE BEFORE SUBMISSION]**

The video should demonstrate:

- Running the executable from the submitted build/project.
- Core boat movement and camera controls.
- Sailing between different fishing zones.
- Fishing with and without the optional minigame.
- Cargo collection, selling, upgrades, and hull repair.
- Fish journal pages and catch feedback.
- Settings menu brightness and audio controls.
- Dangerous water and hull damage.
- Three-key quest progression.
- Lost Island unlock and final victory state.
- Procedural islands, imported tree models, island identifiers, and improved dock setpiece.
- Shadow mapping and **F9** PCF/PCSS toggle.
- Dynamic sun cycle with **F10**.
- God rays with **F11**.
- Image processing modes with **F5–F8**.
- Pause/help screen, start screen, and victory screen.

---

## How to run

1. Open the Visual Studio solution/project.
2. Make sure the following `.cpp` files are included in the Visual Studio project:
   - `main.cpp`
   - `WorldGen.cpp`
   - `WorldRenderer.cpp`
   - `SoundManager.cpp`
   - `UIOverlay.cpp`
   - `GameSettingsMenu.cpp`
   - `PostProcess.cpp`
   - `FishingMinigame.cpp`
   - `IslandQuest.cpp`
   - `LostIslandSetpiece.cpp`
3. Build the project in the correct configuration for your machine.
4. Run the executable from Visual Studio, or run the built `.exe` from the output folder.
5. Keep the required resource folders in their expected relative locations.

Required folders include:

```text
shader/
media/models/
media/sounds/
media/fish/
textures/
```

The project uses relative paths, so models, textures, sounds, fish images, and shader files must remain in the correct folders.

---

## Controls

### Core movement

| Control | Action |
|---|---|
| **W / S** | Move boat forward / backward |
| **A / D** | Steer left / right |
| **C** | Toggle camera mode |
| **Arrow Keys** | Look around in free-look camera mode |
| **Esc** | Quit |

### Fishing, economy, and upgrades

| Control | Action |
|---|---|
| **E** | Fish / cast |
| **M** | Toggle fishing minigame on/off |
| **Space** | Hook during fishing minigame |
| **R** | Sell cargo at the dock |
| **1** | Buy rod upgrade |
| **2** | Buy engine upgrade |
| **3** | Buy cargo upgrade |
| **4** | Repair hull at the dock |

### Menus and progression

| Control | Action |
|---|---|
| **Enter** | Start game |
| **P** | Pause / help screen |
| **J** | Open fish journal |
| **Left / Right Arrow** | Change fish journal page |
| **O** | Open / close settings menu |
| **Up / Down** | Move through settings menu |
| **Left / Right** | Adjust selected settings value |
| **R** inside settings menu | Reset settings to defaults |

### Shader and rendering controls

| Control | Action |
|---|---|
| **F5** | Edge detection post-process |
| **F6** | Blur post-process |
| **F7** | Night vision post-process |
| **F8** | Normal rendering |
| **F9** | Toggle shadow filtering between PCF and PCSS |
| **F10** | Toggle dynamic sun cycle |
| **F11** | God rays / light shafts post-process |

### Development / testing control

| Control | Action |
|---|---|
| **G** | Debug/test gold and progression helper used during development |

For final marking, this should either be clearly explained as a debug helper or removed/disabled if not wanted in the submitted build.

---

## Main gameplay loop

The project has a complete game loop:

1. The player starts at the dock.
2. The player sails into different fishing zones.
3. Each zone has its own fish pool, colour tint, fog, danger level, and ambience.
4. The player catches fish either instantly or using the optional timing minigame.
5. Fish are stored in cargo.
6. The player returns to the dock to sell cargo for gold.
7. Gold is spent on rod, engine, cargo, and hull repair upgrades.
8. Dangerous water damages the hull and affects the boat.
9. Higher rod progression allows the player to collect three hidden keys.
10. Collecting all three keys unlocks the Lost Island.
11. Reaching the Lost Island triggers the final victory state.

This makes the scene function as a game rather than only a graphics demonstration.

---

## Full feature list

### Core game features

- Controllable boat movement.
- Follow camera and free-look camera mode.
- Multiple fishing zones.
- Zone-specific fish pools.
- Optional fishing minigame.
- Cargo inventory.
- Selling system at the dock.
- Gold economy.
- Rod upgrade.
- Engine upgrade.
- Cargo upgrade.
- Hull damage and repair.
- Dangerous water system.
- Engine/hull penalty behaviour.
- Three hidden key objectives.
- Lost Island unlock.
- Final shrine objective.
- Victory screen and celebration feedback.

### World and level features

- Improved dock setpiece with planks, beams, support posts, rail detail, ladder, crates, barrels, side jetty, and a clean shed.
- Procedural mini-island generation using radial falloff and noise.
- Surface-following grass patches on islands.
- Imported `tree.obj` model placement on islands.
- Clean island indicators so zones can be identified from far away.
- Lost Island altar setpiece.
- Imported sword model / fallback sword transform.
- Zone-based atmosphere and water mood.
- Hidden/fogged Lost Island progression behaviour before unlock.

### UI and feedback features

- Start screen.
- Pause/help overlay.
- Main HUD.
- Cargo and gold display.
- Hull integrity display.
- Upgrade cost display.
- Zone name display.
- Quest text display.
- Fishing minigame UI.
- Catch message feedback.
- Catch banner feedback.
- Fish journal.
- Journal page navigation.
- Victory screen.
- Settings menu.
- Brightness control.
- Master/music/SFX volume controls.

### Audio features

- irrKlang audio integration.
- Background ambience loop.
- Boat motor loop that responds to movement.
- Splash and reel/catch sound effects.
- Zone-based ambience layers.
- Lost Island audio mood.
- Volume controls for master, music, and SFX.
- Audio feedback tied to gameplay events.

### Rendering and shader features

- Multiple custom GLSL shader programs.
- Lit material shader.
- Shadow depth shader.
- Water shader.
- Particle shader.
- Skybox shader.
- Post-process shader.
- Shadow mapping.
- PCF shadow filtering.
- PCSS-style soft shadow filtering.
- Dynamic sun cycle.
- Time-varying sun direction.
- Dynamic light colour/intensity.
- Fog and atmosphere blending.
- Animated water surface.
- Water tint based on current zone.
- Water lighting and specular response.
- Skybox rendering.
- Particle effects.
- Edge detection post-processing.
- Blur post-processing.
- Night vision post-processing.
- God rays / screen-space light shafts.
- Brightness adjustment through settings.

---

## Rendering pipeline

The rendering pipeline is split into several passes.

### 1. Shadow depth pass

The world is rendered from the light’s point of view into a depth texture. This depth map is later sampled by the main shader and the water shader to calculate shadows.

Relevant files:

```text
depth.vert
depth.frag
main.cpp
basic.frag
water.frag
```

### 2. Main lit world pass

The world scene is rendered using the main material shader. This includes the dock, generated islands, imported tree models, island markers, boat, Lost Island setpiece, and props.

Relevant files:

```text
basic.vert
basic.frag
WorldRenderer.cpp
RenderTypes.h
```

### 3. Animated water pass

The water is rendered separately using its own shader. It uses time-based animation, zone tint, fog, lighting, and shadow sampling.

Relevant files:

```text
water.vert
water.frag
main.cpp
```

### 4. Skybox pass

A skybox is rendered around the scene to give the game a stronger sense of place and atmosphere.

Relevant files:

```text
skybox.vert
skybox.frag
main.cpp
```

### 5. Particle pass

Particles are used for wake, splash, ambience, Lost Island aura, and win-state celebration effects.

Relevant files:

```text
particle.vert
particle.frag
main.cpp
```

### 6. Post-process pass

The rendered scene is passed through a fullscreen post-processing pipeline. Different modes can be toggled live.

Relevant files:

```text
PostProcess.h
PostProcess.cpp
postprocess.vert
postprocess.frag
```

---

## Advanced graphics features

### Shadow mapping

The scene uses a dedicated shadow depth pass. The light-space depth texture is sampled during the main rendering pass and the water pass.

This improves the visual depth of the scene and demonstrates a multi-pass rendering pipeline.

### PCF and PCSS shadow filtering

The project includes a live **F9** toggle between:

- **PCF**: percentage-closer filtering for softened shadow sampling.
- **PCSS-style filtering**: a blocker search and variable filter radius to imitate contact-hardening soft shadows.

This is one of the clearest advanced shader demonstration features because the difference can be shown directly during the video.

### Dynamic sun cycle

The sun direction, colour, intensity, and ambient contribution change over time. The sun cycle can be toggled with **F10**.

This affects:

- world lighting
- water lighting
- shadow direction
- fog mood
- post-process god rays

### God rays / light shafts

The post-process shader includes a god rays mode toggled with **F11**. It uses the sun’s screen-space position and radial sampling to create stylised light shafts.

This was implemented as a coursework-appropriate screen-space effect rather than a full volumetric lighting simulation.

### Image processing modes

The post-processing pipeline supports:

- edge detection
- blur
- night vision
- god rays
- normal rendering reset

These are controlled with **F5–F8** and **F11**.

### Animated water

The water shader uses time-based motion and surface variation. It reacts to lighting, fog, and zone tint so different parts of the map feel visually distinct.

### Particle effects

Particles are used for both atmosphere and gameplay feedback:

- boat wake
- splash effects
- ambient zone particles
- Lost Island aura
- victory celebration particles

---

## Procedural world generation

The island system was split into `WorldGen.h/.cpp`.

The generated islands use:

- radial falloff
- boundary noise
- rocky height variation
- surface-following grass patches
- generated vertex/normal/UV data
- OpenGL VAO/VBO mesh upload

This replaced earlier block-style islands with more natural mini-islands.

Relevant files:

```text
WorldGen.h
WorldGen.cpp
WorldRenderer.cpp
```

The generated mesh layout matches the main shader:

```text
location 0 = position
location 1 = normal
location 2 = texture coordinate
```

---

## Static world rendering

The static world rendering was split into `WorldRenderer.h/.cpp`.

This system handles:

- improved dock rendering
- generated island drawing
- grass patch rendering
- imported tree model placement
- clean island indicators
- Lost Island altar rendering
- sword model/fallback rendering

The boat remains in `main.cpp` because it depends directly on player movement, water bobbing, cargo display, and gameplay state.

Relevant files:

```text
WorldRenderer.h
WorldRenderer.cpp
RenderTypes.h
WorldGen.h
WorldGen.cpp
```

---

## Main code structure

The project is now split into clearer systems.

| File | Purpose |
|---|---|
| `main.cpp` | Window setup, input, main loop, gameplay state, render pass orchestration |
| `WorldGen.h/.cpp` | Procedural island mesh generation |
| `WorldRenderer.h/.cpp` | Static world drawing: dock, islands, trees, markers, Lost Island |
| `RenderTypes.h` | Shared render/model types such as `ModelMesh` and `MaterialType` |
| `FishingMinigame.h/.cpp` | Optional timing-based fishing minigame |
| `IslandQuest.h/.cpp` | Key collection, Lost Island unlock, final win state |
| `SoundManager.h/.cpp` | Music, SFX, ambient layers, volume control |
| `UIOverlay.h/.cpp` | HUD, start/pause/victory screens, journal, catch feedback |
| `GameSettingsMenu.h/.cpp` | Settings menu and brightness/audio options |
| `PostProcess.h/.cpp` | Fullscreen post-processing pipeline |
| `LostIslandSetpiece.h/.cpp` | Final altar and sword transforms |
| `SimpleOBJLoader.h` | OBJ model loading |

This structure makes `main.cpp` more of a coordinator rather than a single file containing every system.

---

## Important shader files

| Shader | Purpose |
|---|---|
| `basic.vert` / `basic.frag` | Main lit material shader with fog, shadows, PCF/PCSS, material response |
| `water.vert` / `water.frag` | Animated water with lighting, fog, tint, shadowing, and danger mood |
| `depth.vert` / `depth.frag` | Shadow map depth pass |
| `postprocess.vert` / `postprocess.frag` | Edge detection, blur, night vision, god rays, brightness/presentation pass |
| `particle.vert` / `particle.frag` | Point sprite particles |
| `skybox.vert` / `skybox.frag` | Skybox rendering |

---

## Feature mapping against the coursework rubric

### Passing requirements

The project is intended to satisfy the basic pass requirements:

- Software compiles using the module framework.
- Public Git repository exists.
- Video/report explanation submitted.
- Scene contains custom models and original scene composition.
- Shader features from the lecture topics are implemented.

### 40–60 range: advanced shader topics

The project demonstrates multiple advanced shader/rendering topics:

- shadow mapping
- PCF/PCSS shadow filtering
- post-processing
- edge detection
- blur
- night vision
- god rays
- animated water
- particles
- skybox
- dynamic lighting/sun cycle

### 60–90 range: aesthetics and gamification

The project includes a playable game loop and game design systems:

- fishing zones
- fish pools
- cargo
- selling
- upgrades
- hull damage
- danger levels
- repair
- hidden keys
- final objective
- victory state
- journal
- HUD feedback
- settings menu
- audio feedback
- atmospheric zone design

### 90–100 range: research-style advanced features

The strongest research-style / advanced features are:

1. **PCSS-style soft shadow filtering**
   - Based on the idea of contact-hardening soft shadows.
   - Implemented as a live comparison against PCF.
   - Integrated into both the scene shader and water shader.

2. **Screen-space god rays / light shafts**
   - Implemented as a fullscreen post-process.
   - Uses the sun’s screen position and radial sampling.
   - Tied to the dynamic sun cycle.

3. **Procedural island mesh generation**
   - Uses generated geometry rather than only static cube props.
   - Produces irregular island silhouettes and surface-following grass.

---

## How the project addresses previous weaknesses

The earlier version of the project had a simpler shrine/objective structure. This version expands the coursework into a fuller playable prototype by adding:

- deeper game loop
- fishing economy
- upgrades
- hull danger
- repair
- key collection
- Lost Island progression
- fish journal
- custom HUD and menus
- audio layers
- settings menu
- procedural islands
- improved dock and environment set dressing
- stronger shader integration
- cleaner code organisation

The goal was to make the project feel complete rather than just a small graphics demo.

---

## Audio design

Audio is handled through `SoundManager`.

The audio system includes:

- background ambience
- boat motor sound
- splash/catch feedback
- reel feedback
- zone ambience layers
- Lost Island ambience
- master volume
- music volume
- SFX volume

The audio is designed to support the mood of each zone and provide feedback for gameplay actions.

---

## UI design

The custom UI is handled through `UIOverlay` and `GameSettingsMenu`.

The UI includes:

- start overlay
- pause/help screen
- HUD
- cargo and gold display
- hull display
- quest text
- catch card/banner
- fish journal
- settings menu
- victory screen

The UI is built around a `HUDState` structure so the renderer receives the game state it needs without directly controlling the gameplay systems.

---

## Settings menu

The settings menu provides:

- brightness adjustment
- master volume
- music volume
- SFX volume
- reset to defaults

This helps the project feel more like a complete game prototype and improves usability.

---

## Models and assets

Important model assets include:

```text
media/models/boat.obj
media/models/tree.obj
media/models/tree.mtl
media/models/treecolorpallet.png
media/sword.obj
media/models/sword.obj
```

Important texture/resource folders include:

```text
media/fish/
media/sounds/
textures/
shader/
```

If a texture fails to load, some materials use procedural fallback textures so the project remains more robust during testing.

---

## Software engineering decisions

### Modular systems

The project is split into separate systems for rendering, procedural generation, UI, settings, audio, post-processing, fishing, quest logic, and setpiece placement.

This improves:

- readability
- debugging
- maintainability
- report explanation
- marking clarity

### Refactored world generation

Procedural island generation was moved into `WorldGen.h/.cpp` to keep mesh creation separate from the main loop.

### Refactored world rendering

Static world rendering was moved into `WorldRenderer.h/.cpp` to keep dock/island/tree/marker drawing out of `main.cpp`.

### Shared render types

`RenderTypes.h` contains shared rendering structures and enums used across `main.cpp` and `WorldRenderer.cpp`.

### Relative paths

The project uses relative paths for coursework simplicity, so the submitted repo must keep assets in the expected folders.

### Fallbacks

Where possible, fallback textures and fallback geometry are used to keep the scene running if an asset fails to load.

---

## Performance and design trade-offs

This project balances visual quality with coursework scope.

Examples:

- Shadow mapping improves depth but requires a depth pass.
- PCSS-style filtering looks better than hard shadows but is more expensive than simple PCF.
- Post-processing adds style but requires a fullscreen pass.
- Animated water and particles improve atmosphere but add update/render cost.
- Custom UI gives full control but takes more implementation time.
- The project uses some global state for practical coursework integration, while key systems have been separated into dedicated files.

---

## Repository contents

Important files include:

```text
main.cpp
RenderTypes.h
WorldGen.h
WorldGen.cpp
WorldRenderer.h
WorldRenderer.cpp
FishingMinigame.h
FishingMinigame.cpp
IslandQuest.h
IslandQuest.cpp
SoundManager.h
SoundManager.cpp
GameSettingsMenu.h
GameSettingsMenu.cpp
UIOverlay.h
UIOverlay.cpp
PostProcess.h
PostProcess.cpp
LostIslandSetpiece.h
LostIslandSetpiece.cpp
SimpleOBJLoader.h
```

Important folders include:

```text
shader/
media/models/
media/sounds/
media/fish/
textures/
```

---

## Notes for marker

- The project uses multiple shader programs and render passes rather than a single global shader.
- The project combines shader features with a playable fishing game loop.
- The scene includes procedural islands, improved dock set dressing, imported tree models, and island identifiers.
- The UI, journal, settings menu, and victory screens are custom rendered.
- Audio is handled through irrKlang with layered ambience and volume controls.
- The final objective is to collect three keys, unlock the Lost Island, and reach the final shrine.
- The most important advanced rendering features are PCSS-style soft shadows, shadow mapping, dynamic sun cycle, god rays, post-processing, animated water, and particles.
- `G` is a development/debug key and should be treated as a testing helper if left enabled.

---

## Generative AI use declaration

This assessment is marked as Partnered Work for GenAI use.

Generative AI was used for:

- code assistance
- debugging support
- shader/pipeline planning support
- README/report drafting support
- refactoring support
- wording and structure suggestions

All final integration, testing, editing, feature selection, and submission decisions were made by me.

An AI prompts/transcript file or appendix should be included with the final submission if required by the coursework instructions.

---

## Evaluation

The final project is a complete interactive prototype rather than only a collection of disconnected graphics features.

The strongest elements are:

- the integration of shader features into gameplay
- the fishing/economy/progression loop
- the custom UI and journal
- zone-based atmosphere
- dynamic lighting and shadows
- procedural island generation
- audio layering
- settings menu
- Lost Island objective and victory state
- improved modular code structure

If I had more time, I would improve:

1. save/load persistence
2. more fish behaviour differences by biome
3. more bespoke audio events
4. more polished boat/dock collision
5. cleaner removal of remaining global state
6. deeper research write-up with direct source references for PCSS and god rays
