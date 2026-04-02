Mystical Shrine – OpenGL Graphics Prototype
\
\
\
How to Run

1. Extract the project zip folder.

2. Open the Game folder.

3. Run Project_Template.exe.

Ensure the following folders are located beside the executable:

- media
- shader

These contain textures, models, and GLSL shader programs required by the application.
\
\
\
Mystical Shrine is an interactive OpenGL graphics prototype demonstrating a custom GLSL shading pipeline, dynamic lighting, textured models, and post-processing effects.

The project implements a small atmospheric scene where the player explores a forest shrine, collects magical orbs, and unlocks a relic sword. The goal is to demonstrate modern real-time rendering techniques while incorporating simple game mechanics.

The prototype showcases:

Custom GLSL shader programs
Multiple lighting techniques
Textured 3D models
Normal mapping
Bloom post-processing
Skybox environment rendering
Interactive camera controls
Simple gameplay mechanics

Development Environment

Operating System: Windows 11
IDE: Visual Studio 2022
Graphics API: OpenGL 4.6 Core Profile

Libraries Used

GLFW – window creation and input
GLEW – OpenGL function loading
GLM – mathematics and transformations
stb_image – image loading


Project Structure
\
<img width="593" height="629" alt="image" src="https://github.com/user-attachments/assets/56865a86-d92d-47d1-97d6-6af46437c388" />


Rendering Pipeline

The application uses a multi-pass rendering pipeline.

1. Scene Rendering Pass

The 3D scene is rendered into a framebuffer containing:
Main colour buffer
Bright colour buffer (for bloom extraction)
Lighting calculations are performed in the fragment shader using a modified Blinn-Phong model.
Lighting sources include:
Directional moonlight
Point light from the altar
Spotlight activated when the sword is pulled
<img width="801" height="629" alt="image" src="https://github.com/user-attachments/assets/696e7607-d2ad-4598-adcf-aad3b0d1e94b" />

2. Bloom Post-Processing
A Gaussian blur is applied to bright fragments using a ping-pong framebuffer technique.

Steps:
Bright areas extracted
Horizontal blur pass
Vertical blur pass
Blurred result combined with original scene

This creates glowing highlights on emissive objects such as:
magical orbs
the relic sword
\
\
\
3. Final Screen Pass
The final pass combines:
main scene texture
bloom blur texture
gamma correction
gameplay overlay (win screen)
\
\
\
Lighting Model
The project implements a Blinn-Phong lighting model inside the fragment shader.
\
Components:
Ambient lighting
Diffuse lighting
Specular highlights
\
\
\
The lighting system supports:
\
Directional Light
Simulates moonlight illuminating the forest.
\
Point Light
Placed above the altar to illuminate the shrine.
\
Spot Light
Activated when the relic sword is unlocked.
\
Fog is also applied to simulate atmospheric depth in the forest.
\
\
\
Texturing Techniques
Multiple texturing techniques are implemented:
\
Diffuse Mapping
Used for base material colour on shrine geometry.
\
Example: stone.jpg, moss.jpg, Texture Mixing
\
Stone and moss textures are blended to add environmental variation.
\
Normal Mapping
A normal map adds additional surface detail to the shrine geometry without increasing polygon count.
\
Alpha Blending
Used for transparent flame effects around collectible orbs.
\
Post Processing Effects
The project implements multiple post-processing techniques:
\
Bloom
Bright fragments are extracted and blurred to create glow.
\
Gamma Correction
Corrects colour brightness for realistic display.
\
Fullscreen Overlay
A win screen texture is rendered after the final pass.
\
\
\
Gameplay Mechanics
The prototype includes simple interactive mechanics:
\
The player explores the shrine using first-person controls.
\
Magical orbs must be collected.
\
When all orbs are collected the relic sword unlocks.
\
Approaching the sword triggers the victory screen.
\
\
\
Orb objects include animated:
\
floating motion
\
glow effects
\
rotating key models
\
\
\
Controls
\
\
Key	Action
\
W A S D	Move camera
\
Mouse	Look around
\
ESC	Unlock mouse cursor
\
Left Mouse	Lock cursor
\
R	Restart scene
\
F	Toggle fog
\
M	Toggle moonlight
\
L	Toggle altar light
\
N	Toggle normal mapping
\
G	Toggle gamma correction
\
B	Toggle bloom
\
SPACE	Toggle animation
\
\
\
Debug Logging
\
The program includes structured debug logging to help track rendering and gameplay events.
\
Examples of logged information:
\
Shader compilation status
\
Texture loading success
\
Framebuffer setup
\
Model loading
\
Gameplay events such as restart and win conditions
\
\
\
Example output:
\
[Shader] Linked basic shader program
\
[Init] Loaded texture: assets/stone.jpg
\
[FBO] Main framebuffer complete
\
[Gameplay] Scene reset complete
\
<img width="626" height="400" alt="image" src="https://github.com/user-attachments/assets/adee7adc-ed04-46eb-9071-5835521b6db8" />



Coursework Objectives Demonstrated

This project demonstrates several techniques required by the coursework specification:
Custom GLSL shading model
Multiple lighting techniques
Texture sampling
Normal mapping
Framebuffer rendering
Skybox environment
Post-processing effects
Interactive camera system

Future Improvements:
Potential improvements include:
shadow mapping
HDR rendering
physically based shading
particle systems for magical effects
improved environment modelling

Author
Student: David Wilson
Module: COMP3015 Graphics Programming
\
\
\
AI Usage
Generative AI tools were used to help in drafting the README documentation for this project. The AI was used to help structure explanations of the rendering pipeline, graphics techniques, and gameplay mechanics, as well as to improve clarity and grammar in the written description.



AI was used to assist with the format of the code itself (double checking for general errors and linking all of the files together) as well as generating ideas for what to make the project at the beginning 

<img width="928" height="745" alt="image" src="https://github.com/user-attachments/assets/e127d88b-4efc-442b-a8ee-5f233be97e72" />

AI was used to help create in game models (mainly the fire around the keys)
<img width="862" height="417" alt="image" src="https://github.com/user-attachments/assets/241b1308-c8ed-4321-b7f5-2469f5482ea4" />


AI was used to create a victory screen due to time constrants 
<img width="971" height="507" alt="image" src="https://github.com/user-attachments/assets/bbd8e915-4cf4-4223-a3c8-3baef3732123" />

AI was mainly useful for checking my work against the marking rubric to ensure that i had ticked all boxes and had not forgotten to add any features


<img width="1081" height="577" alt="image" src="https://github.com/user-attachments/assets/f4d7a8a7-947d-4d03-8163-a0bbeba42aaa" />
