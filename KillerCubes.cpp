#include "TL-Engine.h"
using namespace tl;

// Constants
const int kNumBlocksPerRow = 5;           // Number of blocks in one row
const int kMaxMarbles = 3;                // Maximum number of marbles allowed at once
const float kMarbleSpeed = 0.5f;          // Speed at which marbles travel forward
const float kBlockWidth = 2.0f;           // Distance between blocks

// Enum for game state
enum GameState { MENU, GAME, OVER };

// Enum to track the state of each block
enum BlockState { NORMAL, HIT_ONCE, DEAD };

// Structure to keep track of each marble
struct Marble {
    IModel* model;
    bool active;
};

// Global engine and game elements
I3DEngine* engine;
ICamera* camera;
IMesh* marbleMesh, * blockMesh, * floorMesh;
IModel* floor;
Marble marbles[kMaxMarbles];
IModel* blocks[2][kNumBlocksPerRow];         // 2 rows of blocks
BlockState blockStates[2][kNumBlocksPerRow]; // State of each block
IText* gameOverText;

GameState currentState = MENU; // Start in MENU state

// Sets up the entire game scene (camera, models, etc.)
void SetupScene() {
    engine = New3DEngine(kTLX);
    engine->StartWindowed();
    engine->AddMediaFolder("media");
    engine->SetWindowCaption("Marble Shooting Game");

    // Position the camera to view the scene
    camera = engine->CreateCamera(kManual);
    camera->MoveTo(0, 10, -25);
    camera->RotateX(20);

    // Load all meshes
    marbleMesh = engine->LoadMesh("marble.obj");
    blockMesh = engine->LoadMesh("block.obj");
    floorMesh = engine->LoadMesh("floor.x");

    // Create the floor
    floor = floorMesh->CreateModel(0, -1, 0);

    // Create 2 rows of blocks and set their initial state to NORMAL
    for (int row = 0; row < 2; row++) {
        for (int i = 0; i < kNumBlocksPerRow; i++) {
            blocks[row][i] = blockMesh->CreateModel(i * kBlockWidth - 5.0f, 0, row * 5.0f + 10.0f);
            blockStates[row][i] = NORMAL;
        }
    }

    // Create inactive marbles and hide them initially
    for (int i = 0; i < kMaxMarbles; i++) {
        marbles[i].model = marbleMesh->CreateModel(0, 0, 0);
        marbles[i].active = false;
        marbles[i].model->SetVisible(false);
    }

    // Create the GAME OVER text (invisible at start)
    gameOverText = engine->CreateText("GAME OVER - Press ESC to Quit or 0 to Restart", 200, 350, kWhite, kCentre);
    gameOverText->SetVisible(false);
}

// Resets all game elements to their original state
void RestartGame() {
    // Deactivate and reset all marbles
    for (int i = 0; i < kMaxMarbles; i++) {
        marbles[i].active = false;
        marbles[i].model->SetVisible(false);
        marbles[i].model->SetPosition(0, 0, 0);
    }

    // Reset all blocks to their original positions and skins
    for (int row = 0; row < 2; row++) {
        for (int i = 0; i < kNumBlocksPerRow; i++) {
            blocks[row][i]->SetPosition(i * kBlockWidth - 5.0f, 0, row * 5.0f + 10.0f);
            blocks[row][i]->SetSkin("default.jpg");
            blocks[row][i]->SetVisible(true);
            blockStates[row][i] = NORMAL;
        }
    }

    gameOverText->SetVisible(false);
    currentState = GAME;
}

// Shoots a marble if one is available
void ShootMarble() {
    for (int i = 0; i < kMaxMarbles; i++) {
        if (!marbles[i].active) {
            marbles[i].model->SetPosition(0, 0, 0);
            marbles[i].model->SetVisible(true);
            marbles[i].active = true;
            break;
        }
    }
}

// Checks for simple sphere-sphere collision between two models
bool CheckCollision(IModel* m1, IModel* m2, float radius = 1.0f) {
    float dx = m1->GetX() - m2->GetX();
    float dz = m1->GetZ() - m2->GetZ();
    float distanceSq = dx * dx + dz * dz;
    float threshold = radius * 2.0f;
    return distanceSq < threshold * threshold;
}

// Updates all game logic during the GAME state
void UpdateGame() {
    // Shoot a marble when SPACE is pressed
    if (engine->KeyHit(Key_Space)) {
        ShootMarble();
    }

    // Move each active marble forward and check for collisions
    for (int i = 0; i < kMaxMarbles; i++) {
        if (marbles[i].active) {
            marbles[i].model->MoveZ(kMarbleSpeed);

            // Remove marble if it goes out of bounds
            if (marbles[i].model->GetZ() > 30) {
                marbles[i].active = false;
                marbles[i].model->SetVisible(false);
                continue;
            }

            // Check for collisions with each block
            for (int row = 0; row < 2; row++) {
                for (int j = 0; j < kNumBlocksPerRow; j++) {
                    if (blockStates[row][j] != DEAD && CheckCollision(marbles[i].model, blocks[row][j])) {
                        marbles[i].active = false;
                        marbles[i].model->SetVisible(false);

                        // Update block state: NORMAL , HIT_ONCE , DEAD
                        if (blockStates[row][j] == NORMAL) {
                            blocks[row][j]->SetSkin("red.jpg");
                            blockStates[row][j] = HIT_ONCE;
                        }
                        else if (blockStates[row][j] == HIT_ONCE) {
                            blockStates[row][j] = DEAD;
                            blocks[row][j]->SetVisible(false);
                        }
                    }
                }
            }
        }
    }

    // Restart the game if key '0' is pressed
    if (engine->KeyHit(Key_0)) {
        RestartGame();
    }

    // Check if all blocks have been destroyed
    bool allDead = true;
    for (int row = 0; row < 2; row++) {
        for (int j = 0; j < kNumBlocksPerRow; j++) {
            if (blockStates[row][j] != DEAD) {
                allDead = false;
                break;
            }
        }
    }

    // If all blocks are gone, show game over message
    if (allDead) {
        currentState = OVER;
        gameOverText->SetVisible(true);
    }
}

// Manages transitions between game states
void Update() {
    switch (currentState) {
    case MENU:
        // Start the game from menu when SPACE is pressed
        if (engine->KeyHit(Key_Space)) {
            currentState = GAME;
        }
        break;

    case GAME:
        // Main game logic
        UpdateGame();
        break;

    case OVER:
        // Allow restart or quit after game is over
        if (engine->KeyHit(Key_0)) {
            RestartGame();
        }
        if (engine->KeyHit(Key_Escape)) {
            engine->Stop();
        }
        break;
    }
}

// Main entry point
int main() {
    SetupScene();

    while (engine->IsRunning()) {
        engine->DrawScene();
        Update();
    }

    engine->Delete();
    return 0;
}
