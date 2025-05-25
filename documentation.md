# Medieval Fantasy Fight - Technical Documentation

## Overview
A 2D medieval fantasy action game built with OpenGL, featuring real-time combat, AI enemies, projectile systems, and procedural terrain generation. The game implements a complete game loop with win/loss conditions, health systems, and visual effects.

## Architecture

### Core Components

#### 1. Game Class (`Game.h`, `Game.cpp`)
The main game controller that manages all systems and rendering.

**Key Responsibilities:**
- OpenGL context management and rendering pipeline
- Game state management (playing, won, dead)
- Entity management (player, enemies, projectiles)
- Input processing and game loop coordination

**Core Systems:**
```cpp
class Game {
    // Rendering systems
    Shader* shaderProgram;
    Shader* textShader;
    Font* gameFont;
    
    // Game entities
    Player* player;
    std::vector<Enemy> enemies;
    
    // Weapon systems
    struct Sword { /* sword properties */ };
    struct Arrow { /* projectile properties */ };
    
    // Terrain system
    std::vector<TerrainElement> terrainElements;
    
    // Game state
    bool gameWon;
    int totalEnemiesKilled;
    int enemiesToKill; // Set to 5 for quick gameplay
};
```

#### 2. Player System (`Player.h`, `Player.cpp`)
Handles player movement, health, and collision detection.

**Features:**
- WASD movement with delta-time based smooth motion
- Health system (100 HP) with damage/healing
- Invulnerability frames after taking damage
- Death state management
- Collision detection with boundaries and enemies

#### 3. Enemy AI System (`Enemy.h`, `Enemy.cpp`)
Advanced AI with multiple behavioral states and combat capabilities.

**AI States:**
- **WANDERING**: Random movement around spawn area
- **DETECTING**: Player spotted, transitioning to combat
- **FOLLOWING**: Actively pursuing the player
- **ATTACKING**: In combat range, using weapons
- **FLEEING**: Low health retreat behavior

**Combat Features:**
- Ranged attacks with arrow projectiles
- Melee attacks when in close range
- Smart collision avoidance between enemies
- Health system with visual health bars
- Predictive movement and line-of-sight detection

## Weapon Systems

### 1. Sword Combat
**Implementation:** `initSword()`, `updateSword()`, `renderSword()`

**Features:**
- Orbital movement around player when idle
- 3-phase attack animation:
  1. **Preparation** (33%): Move to attack position
  2. **Strike** (33%): Swing around player with damage detection
  3. **Recovery** (33%): Return to orbital position
- Collision detection with configurable hitbox radius
- 1-second cooldown between attacks
- 25 damage per hit

**Technical Details:**
```cpp
struct Sword {
    float offsetX, offsetY;    // Position relative to player
    float angle;               // Current rotation
    float length, width;       // Physical dimensions
    float hitboxRadius;        // Collision detection radius
    bool isSwinging;           // Animation state
    float swingProgress;       // Animation progress (0.0-1.0)
    float damage;              // Damage dealt (25)
    float cooldown;            // Attack cooldown (1.0s)
};
```

### 2. Arrow Projectile System
**Implementation:** `initArrow()`, `renderArrow()`, arrow update logic

**Features:**
- Mouse-aimed shooting with left-click
- Realistic arrow shape with multiple components:
  - Metallic arrowhead (silver/gray)
  - Wooden shaft (brown)
  - Feathered fletching (reddish-brown)
- Proper rotation based on velocity direction
- Collision detection with enemies
- 10 damage per hit

**Technical Implementation:**
```cpp
struct Arrow {
    float x, y;           // World position
    float vx, vy;         // Velocity vector
    float radius;         // Collision radius
    float angle;          // Rotation for visual alignment
};
```

## Terrain System

### Procedural Background Generation
**Implementation:** `initTerrain()`, `generateTerrain()`, `renderTerrain()`

**Design Philosophy:**
- Grayscale pixel art aesthetic
- Scattered, non-overlapping elements
- Static generation (no animation)
- Black background for contrast

**Terrain Types:**
1. **GRASS_BLADE**: Thin vertical elements (medium gray)
2. **STONE_ROCK**: Irregular multi-part shapes (light gray)
3. **DIRT_PATCH**: Small square patches (dark gray)
4. **COBBLE_STONE**: Rectangular stones with border lines (medium-light gray)
5. **SAND_GRAIN**: Tiny scattered dots (lightest gray)

**Generation Algorithm:**
```cpp
// 150 total elements scattered across world
// Minimum distance check prevents overlap
// Random rotation for visual variety
// Grayscale color variation (0.7-1.0 multiplier)
```

## Rendering Pipeline

### OpenGL Setup
- **Version**: OpenGL 3.3 Core Profile
- **Resolution**: 1920x1080 fullscreen
- **Blending**: Enabled for transparency effects
- **Projection**: Orthographic with aspect ratio correction

### Shader System
**Vertex Shader** (`vertex_shader.glsl`):
- Handles 2D transformations
- Model-View-Projection matrix multiplication
- Supports scaling and offset for different object types

**Fragment Shader** (`fragment_shader.glsl`):
- Uniform color rendering
- Alpha blending support

### Rendering Order
1. **Background**: Black clear color
2. **Terrain**: Scattered grayscale elements
3. **Player**: Green circle (red when dead, flashing when invulnerable)
4. **Enemies**: Color-coded by AI state
5. **Weapons**: Sword and arrow projectiles
6. **UI**: Health bars, kill counter
7. **Overlays**: Win/death screens

## Game State Management

### Win Condition
- **Objective**: Kill 5 enemies
- **Tracking**: `totalEnemiesKilled` counter
- **Trigger**: Automatic when count reaches `enemiesToKill`
- **Effects**: 
  - Stop enemy spawning
  - Freeze game updates
  - Display victory screen with gold text
  - Show final kill count

### Death Condition
- **Trigger**: Player health reaches 0
- **Effects**:
  - 3-second delay before death screen
  - Semi-transparent black overlay
  - Red "YOU DIED!" text
  - Exit instruction

### UI Elements
1. **Health Bar**: Top-left, red fill with background
2. **Kill Counter**: Top-right, "Kills: X/5" format
3. **Enemy Health Bars**: Above each enemy
4. **Screen Overlays**: Win/death screens with text

## Input System

### Controls
- **WASD**: Player movement
- **Left Mouse**: Fire arrow (aim with cursor)
- **Right Mouse**: Sword attack
- **ESC**: Exit game
- **T**: Debug damage (10 HP)
- **H**: Debug heal (5 HP)
- **K**: Debug instant kill

### Input Processing
- Delta-time based movement for frame-rate independence
- Mouse position converted to world coordinates
- Debounced input to prevent spam
- State-based input handling (disabled when dead/won)

## Performance Considerations

### Optimization Techniques
1. **Object Pooling**: Reuse enemy objects instead of constant allocation
2. **Culling**: Only render visible elements
3. **Static Terrain**: Generate once, render repeatedly
4. **Efficient Collision**: Simple circle-circle distance checks
5. **Limited Entities**: Cap enemy count at 4 simultaneous

### Memory Management
- Proper cleanup in `Game::cleanup()`
- VAO/VBO deletion for all graphics objects
- Smart pointer usage where applicable
- Vector management for dynamic collections

## Build System

### Dependencies
- **OpenGL 3.3+**
- **GLFW**: Window management and input
- **GLEW**: OpenGL extension loading
- **GLM**: Mathematics library
- **FreeType**: Font rendering (optional)

### Compilation
```bash
make
```

### File Structure
```
knighted-circle/
├── Game.h/.cpp          # Main game class
├── Player.h/.cpp        # Player entity
├── Enemy.h/.cpp         # Enemy AI and combat
├── Shader.h/.cpp        # OpenGL shader management
├── Font.h/.cpp          # Text rendering system
├── vertex_shader.glsl   # Vertex shader
├── fragment_shader.glsl # Fragment shader
├── text_vertex.glsl     # Text vertex shader
├── text_fragment.glsl   # Text fragment shader
├── Makefile            # Build configuration
└── documentation.md    # This file
```

## Technical Specifications

### Performance Targets
- **Frame Rate**: 60 FPS
- **Resolution**: 1920x1080
- **Max Enemies**: 4 simultaneous
- **Terrain Elements**: 150 static pieces

### Coordinate System
- **World Space**: Normalized coordinates (-aspect to +aspect, -1 to +1)
- **Screen Space**: Pixel coordinates for UI (0 to 1920, 0 to 1080)
- **Aspect Ratio**: Automatically calculated and maintained

### Collision Detection
- **Method**: Circle-circle distance comparison
- **Precision**: Floating-point with configurable radii
- **Optimization**: Early exit on distance checks

## Future Enhancement Opportunities

### Potential Improvements
1. **Audio System**: Sound effects and background music
2. **Particle Effects**: Blood, sparks, magic effects
3. **Level System**: Multiple stages with increasing difficulty
4. **Weapon Variety**: Different sword types, bows, magic
5. **Enemy Types**: Varied AI behaviors and appearances
6. **Save System**: Progress persistence
7. **Multiplayer**: Network-based cooperative play

### Code Extensibility
The modular design allows for easy extension:
- New enemy types via inheritance
- Additional weapons through similar struct patterns
- Enhanced terrain through new element types
- UI improvements via the existing font system

## Debugging and Testing

### Debug Features
- Health manipulation (T/H/K keys)
- Console output for game events
- Visual state indicators (enemy colors, player flashing)
- Performance monitoring through frame timing

### Testing Scenarios
1. **Combat Testing**: Verify damage, cooldowns, collision detection
2. **AI Testing**: Observe state transitions and behaviors
3. **Win/Loss Testing**: Trigger end conditions
4. **Performance Testing**: Monitor frame rate under load
5. **Input Testing**: Verify all control schemes
