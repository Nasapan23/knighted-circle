# Game Structure Documentation

## Overview
This is a simple 2D OpenGL-based C++ game called "Medieval Fantasy Fight". Players control a character and must avoid or defeat enemies while shooting arrows.

## Technical Stack
- **Graphics API**: OpenGL
- **Window Management**: GLFW
- **OpenGL Loader**: GLEW
- **Math Library**: GLM
- **Dependencies Directory**: `/dependente/` contains:
  - glew/
  - glfw/
  - glm/
  - freeglut/
  - stb-master/

## Core Files Structure
- **Main Entry Point**: `main.cpp`
- **Game Logic**: `Game.h` and `Game.cpp`
- **Player Class**: `Player.h` and `Player.cpp`
- **Enemy Class**: `Enemy.h` and `Enemy.cpp`
- **Shader Management**: `Shader.h` and `Shader.cpp`
- **Shader Files**: `vertex_shader.glsl` and `fragment_shader.glsl`

## Game Mechanics
1. **Player Control**:
   - Movement using WASD keys
   - Player is represented as a green circle
   - Has collision detection with screen boundaries

2. **Enemies**:
   - Represented as red circles
   - Spawn from screen edges
   - Move toward the player
   - Implemented with simple AI that follows the player
   - Collision detection with each other and the player

3. **Combat System**:
   - Player shoots arrows with left mouse click
   - Arrows are represented as small yellow circles
   - Arrows travel in the direction of mouse click
   - Arrows destroy enemies on contact

4. **Render System**:
   - Uses orthographic projection for 2D view
   - All game entities (player, enemies, arrows) are rendered as circles
   - Different colors identify different entities
   - Uses a common vertex and fragment shader for all entities

## Implementation Details

### Game Loop
1. Process input (keyboard and mouse)
2. Update game state (positions, collisions)
3. Render game objects
4. Poll for window events

### Rendering Approach
- Creates circle vertices based on segments
- Uses a single VAO/VBO for rendering all circles
- Adjusts scale and position via shader uniforms
- Uses a simple color shader for rendering

### Collision System
- Simple distance-based collision detection
- Resolves collisions by pushing objects apart
- Handles player-enemy, enemy-enemy, and arrow-enemy collisions

### Player Implementation
- Position (x, y) updated based on keyboard input
- Has a defined radius and movement speed
- Constrained to screen boundaries

### Enemy Implementation
- Simple AI that moves toward the player
- Spawns from random locations at screen edges
- Has collision detection with other enemies and the player

### Technical Details
- Full-screen application using primary monitor resolution
- Uses OpenGL 3.3 Core Profile
- Standard shader-based rendering pipeline 