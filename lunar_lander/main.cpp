#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Entity.h"

#define PLATFORM_COUNT 26

#include <vector>

enum GameMode { PLAYING, WIN, LOSE };
GameMode mode = PLAYING;

struct GameState {
    Entity *player;
    Entity *platforms;
};

GameState state;

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint LoadTexture(const char* filePath) {
  int w, h, n;
  unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);
  
  if (image == NULL) {
    std::cout << "Unable to load image. Make sure the path is correct\n";
    assert(false);
  }
  
  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  
  stbi_image_free(image);
  return textureID;
}

GLuint SHIP_TEXTURES[3];
GLuint *fontTexID;

void DrawText(ShaderProgram *program, GLuint fontTextureID, std::string text,
              float size, float spacing, glm::vec3 position)
{
  float width = 1.0f / 16.0f;
  float height = 1.0f / 16.0f;

  std::vector<float> vertices;
  std::vector<float> texCoords;

  for(size_t i = 0; i < text.size(); i++) {
    int index = (int)text[i];

    float offset = (size + spacing) * i;

    float u = (float)(index % 16) / 16.0f;
    float v = (float)(index / 16) / 16.0f;

     vertices.insert(vertices.end(), {
         offset + (-0.5f * size), 0.5f * size,
         offset + (-0.5f * size), -0.5f * size,
         offset + (0.5f * size), 0.5f * size,
         offset + (0.5f * size), -0.5f * size,
         offset + (0.5f * size), 0.5f * size,
         offset + (-0.5f * size), -0.5f * size,
     });

     texCoords.insert(texCoords.end(), {u, v, u, v + height,u + width, v, u + width, v + height, u + width,
                      v, u, v + height
                      });
  }
  glm::mat4 modelMatrix = glm::mat4(1.0f);
  modelMatrix = glm::translate(modelMatrix, position);
  program->SetModelMatrix(modelMatrix);
  glUseProgram(program->programID);
  glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
  glEnableVertexAttribArray(program->positionAttribute);
  glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
  glEnableVertexAttribArray(program->texCoordAttribute);
  glBindTexture(GL_TEXTURE_2D, fontTextureID);
  glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));
  glDisableVertexAttribArray(program->positionAttribute);
  glDisableVertexAttribArray(program->texCoordAttribute);
}

void Initialize() {
  SDL_Init(SDL_INIT_VIDEO);
  displayWindow = SDL_CreateWindow("Lunar Lander", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
  SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
  SDL_GL_MakeCurrent(displayWindow, context);
  
#ifdef _WINDOWS
  glewInit();
#endif
  
  glViewport(0, 0, 640, 480);
  
  program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");
  
  viewMatrix = glm::mat4(1.0f);
  modelMatrix = glm::mat4(1.0f);
  projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
  
  program.SetProjectionMatrix(projectionMatrix);
  program.SetViewMatrix(viewMatrix);
  
  glUseProgram(program.programID);
  
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_BLEND);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
 
  // Initialize Game Objects
  
  // Initialize Player
  state.player = new Entity();
  state.player->position = glm::vec3(0.0f, 5.0f, 0.0f);
  state.player->movement = glm::vec3(0);
  state.player->acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
  state.player->speed = 1.5f;
  state.player->entityType = PLAYER;
  state.player->velocity.y = -1.0f;


  SHIP_TEXTURES[0] = LoadTexture("blue_ship.png");
  SHIP_TEXTURES[1] = LoadTexture("red_ship.png");
  SHIP_TEXTURES[2] = LoadTexture("green_ship.png");
  state.player->textureID = SHIP_TEXTURES[0];
  
  state.player->height = 1.0f;
  state.player->width = 1.0f;

  state.player->jumpPower = 5.0f;

  fontTexID = new GLuint(LoadTexture("font.png"));

  state.platforms = new Entity[PLATFORM_COUNT];

  GLuint winPlatformTexID = LoadTexture("win_tile.png");
  GLuint losePlatformTexID = LoadTexture("lose_tile.png");

  //floor
  state.platforms[0].textureID = losePlatformTexID;
  state.platforms[0].position = glm::vec3(-4.5f, -3.25f, 0.0f);
  state.platforms[0].entityType = LOSE_PLATFORM;

  state.platforms[1].textureID = winPlatformTexID;
  state.platforms[1].position = glm::vec3(-3.5f, -3.25f, 0.0f);
  state.platforms[1].entityType = WIN_PLATFORM;

  state.platforms[2].textureID = losePlatformTexID;
  state.platforms[2].position = glm::vec3(-2.5f, -3.25f, 0.0f);
  state.platforms[2].entityType = LOSE_PLATFORM;
  
  state.platforms[3].textureID = losePlatformTexID;
  state.platforms[3].position = glm::vec3(-1.5f, -3.25f, 0.0f);
  state.platforms[3].entityType = LOSE_PLATFORM;

  state.platforms[4].textureID = losePlatformTexID;
  state.platforms[4].position = glm::vec3(-0.5f, -3.25f, 0.0f);
  state.platforms[4].entityType = LOSE_PLATFORM;

  state.platforms[5].textureID = losePlatformTexID;
  state.platforms[5].position = glm::vec3(0.5f, -3.25f, 0.0f);
  state.platforms[5].entityType = LOSE_PLATFORM;

  state.platforms[6].textureID = losePlatformTexID;
  state.platforms[6].position = glm::vec3(1.5f, -3.25f, 0.0f);
  state.platforms[6].entityType = LOSE_PLATFORM;

  state.platforms[7].textureID = losePlatformTexID;
  state.platforms[7].position = glm::vec3(2.5f, -3.25f, 0.0f);
  state.platforms[7].entityType = LOSE_PLATFORM;

  state.platforms[8].textureID = losePlatformTexID;
  state.platforms[8].position = glm::vec3(3.5f, -3.25f, 0.0f);
  state.platforms[8].entityType = LOSE_PLATFORM;

  state.platforms[9].textureID = losePlatformTexID;
  state.platforms[9].position = glm::vec3(4.5f, -3.25f, 0.0f);
  state.platforms[9].entityType = LOSE_PLATFORM;
  
  //left wall
  state.platforms[10].textureID = losePlatformTexID;
  state.platforms[10].position = glm::vec3(-4.5f, -2.25f, 0.0f);
  state.platforms[10].entityType = LOSE_PLATFORM;

  state.platforms[11].textureID = losePlatformTexID;
  state.platforms[11].position = glm::vec3(-4.5f, -1.25f, 0.0f);
  state.platforms[11].entityType = LOSE_PLATFORM;
  
  state.platforms[12].textureID = losePlatformTexID;
  state.platforms[12].position = glm::vec3(-4.5f, -0.25f, 0.0f);
  state.platforms[12].entityType = LOSE_PLATFORM;

  state.platforms[13].textureID = losePlatformTexID;
  state.platforms[13].position = glm::vec3(-4.5f, 0.25f, 0.0f);
  state.platforms[13].entityType = LOSE_PLATFORM;
  
  state.platforms[14].textureID = losePlatformTexID;
  state.platforms[14].position = glm::vec3(-4.5f, 1.25f, 0.0f);
  state.platforms[14].entityType = LOSE_PLATFORM;

  state.platforms[15].textureID = losePlatformTexID;
  state.platforms[15].position = glm::vec3(-4.5f, 2.25f, 0.0f);
  state.platforms[15].entityType = LOSE_PLATFORM;

  state.platforms[16].textureID = losePlatformTexID;
  state.platforms[16].position = glm::vec3(-4.5f, 3.25f, 0.0f);
  state.platforms[16].entityType = LOSE_PLATFORM;

  //right wall
  state.platforms[17].textureID = losePlatformTexID;
  state.platforms[17].position = glm::vec3(4.5f, -2.25f, 0.0f);
  state.platforms[17].entityType = LOSE_PLATFORM;

  state.platforms[18].textureID = losePlatformTexID;
  state.platforms[18].position = glm::vec3(4.5f, -1.25f, 0.0f);
  state.platforms[18].entityType = LOSE_PLATFORM;
  
  state.platforms[19].textureID = losePlatformTexID;
  state.platforms[19].position = glm::vec3(4.5f, -0.25f, 0.0f);
  state.platforms[19].entityType = LOSE_PLATFORM;

  state.platforms[20].textureID = losePlatformTexID;
  state.platforms[20].position = glm::vec3(4.5f, 0.25f, 0.0f);
  state.platforms[20].entityType = LOSE_PLATFORM;
  
  state.platforms[21].textureID = losePlatformTexID;
  state.platforms[21].position = glm::vec3(4.5f, 1.25f, 0.0f);
  state.platforms[21].entityType = LOSE_PLATFORM;

  state.platforms[22].textureID = losePlatformTexID;
  state.platforms[22].position = glm::vec3(4.5f, 2.25f, 0.0f);
  state.platforms[22].entityType = LOSE_PLATFORM;

  state.platforms[23].textureID = losePlatformTexID;
  state.platforms[23].position = glm::vec3(4.5f, 3.25f, 0.0f);
  state.platforms[23].entityType = LOSE_PLATFORM;

  //obstacles
  state.platforms[24].textureID = losePlatformTexID;
  state.platforms[24].position = glm::vec3(-2.5f, -0.25f, 0.0f);
  state.platforms[24].entityType = LOSE_PLATFORM;

  state.platforms[25].textureID = losePlatformTexID;
  state.platforms[25].position = glm::vec3(-3.5f, -0.25f, 0.0f);
  state.platforms[25].entityType = LOSE_PLATFORM;
  
  for (int i = 0; i < PLATFORM_COUNT; i++) {
    state.platforms[i].Update(0, NULL, 0);
  }
  

}

void ProcessInput() {
  SDL_Event event;
  switch(mode) {
    case WIN:
    case LOSE:
      while (SDL_PollEvent(&event)) {
        switch (event.type) {
          case SDL_QUIT:
          case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;
        }
      }
      break;
    case PLAYING:
      state.player->movement = glm::vec3(0);
      
      while (SDL_PollEvent(&event)) {
        switch (event.type) {
          case SDL_QUIT:
          case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;
            
          case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
              case SDLK_LEFT:
                // Move the player left
                break;
                
              case SDLK_RIGHT:
                // Move the player right
                break;
                
              case SDLK_SPACE:
                if (state.player->collidedBottom) {
                  state.player->jump = true;
                }
                break;
              }
            break; // SDL_KEYDOWN
        }
      }
      
      const Uint8 *keys = SDL_GetKeyboardState(NULL);

      if (keys[SDL_SCANCODE_LEFT]) {
        state.player->movement.x = -1.0f;
      }
      else if (keys[SDL_SCANCODE_RIGHT]) {
        state.player->movement.x = 1.0f;
      }
      

      if (glm::length(state.player->movement) > 1.0f) {
        state.player->movement = glm::normalize(state.player->movement);
      }
      break;
  }
}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;

void Update() {
  float ticks = (float)SDL_GetTicks() / 1000.0f;
  float deltaTime = ticks - lastTicks;
  lastTicks = ticks;

  switch (mode) {
    case WIN:
    case LOSE:
      break;
    case PLAYING:
      deltaTime += accumulator;

      if (deltaTime < FIXED_TIMESTEP) { accumulator = deltaTime; return; }

      // Update using fixed time step
      while (deltaTime >= FIXED_TIMESTEP) {
        //if bottom collision update and check if collided with win or lose platform
        if (state.player->collidedBottom) {
          state.player->Update(FIXED_TIMESTEP, state.platforms, PLATFORM_COUNT);
          if (state.player->lastCollision == WIN_PLATFORM) {
            mode = WIN;
          } else {
            mode = LOSE;
          }
        } else {
          state.player->Update(FIXED_TIMESTEP, state.platforms, PLATFORM_COUNT);
        }
        deltaTime -= FIXED_TIMESTEP;
      }
      accumulator = deltaTime;
      break;
  }

}

void Render() {
  glClear(GL_COLOR_BUFFER_BIT);

  switch (mode) {
    case WIN:
      DrawText(&program, *fontTexID, "GREAT SUCCESS!!", 0.5f, -0.25f, glm::vec3(-2.0f, 1.0f, 0.0f));
      state.player->textureID = SHIP_TEXTURES[2];
      break;
    case LOSE:
      DrawText(&program, *fontTexID, "MISSION FAILED", 0.5f, -0.25f, glm::vec3(-2.0f, 1.0f, 0.0f));
      state.player->textureID = SHIP_TEXTURES[1];
      break;
  }

  for (int i = 0; i < PLATFORM_COUNT; i++) {
    state.platforms[i].Render(&program);
  }

  state.player->Render(&program);
  
  SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {
  SDL_Quit();
}

int main(int argc, char* argv[]) {
  Initialize();
  
  while (gameIsRunning) {
    ProcessInput();
    Update();
    Render();
  }
  
  Shutdown();
  return 0;
}

