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

#include <vector>

#define BULLET_COUNT 3
#define ENEMY_COUNT 10
#define ENEMY_BULLET_COUNT 50

enum GameMode { PLAYING, WIN, LOSE };
GameMode mode = PLAYING;

float ORTHO_WIDTH = 20.0f;
float ORTHO_HEIGHT = 15.0f;

struct GameState {
    Entity *player;
    Entity *enemies;
    Entity *bullets;
    Entity *enemyBullets;
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

GLuint *fontTexID;
bool BOSS_TEXT = false;

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

int WIDTH = 640;
int HEIGHT = 480;

void Initialize() {
  SDL_Init(SDL_INIT_VIDEO);
  
  displayWindow = SDL_CreateWindow("Rise of AI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
  SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
  SDL_GL_MakeCurrent(displayWindow, context);
  
#ifdef _WINDOWS
  glewInit();
#endif
  
  glViewport(0, 0, 640, 480);
  
  program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");
  
  viewMatrix = glm::mat4(1.0f);
  modelMatrix = glm::mat4(1.0f);
  projectionMatrix = glm::ortho(-ORTHO_WIDTH, ORTHO_WIDTH, -ORTHO_HEIGHT, ORTHO_HEIGHT, -1.0f, 1.0f);
  
  program.SetProjectionMatrix(projectionMatrix);
  program.SetViewMatrix(viewMatrix);
  
  glUseProgram(program.programID);
  
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  glEnable(GL_BLEND);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
 
  // Initialize Game Objects
  //load textures
  GLuint playerTex = LoadTexture("player.png");
  GLuint sniperTex = LoadTexture("goon2.png");
  GLuint bomberTex = LoadTexture("goon1.png");
  GLuint bossTex= LoadTexture("boss.png");
  GLuint bulletTex = LoadTexture("bullet.png");
  GLuint enemyBulletTex = LoadTexture("enemy_bullet.png");
  fontTexID = new GLuint(LoadTexture("font.png"));
  
  // Initialize Player
  state.player = new Entity();
  state.player->isActive = true;
  state.player->position = glm::vec3(0.0f, -10.0f, 0.0f);
  state.player->movement = glm::vec3(0);
  state.player->acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
  state.player->speed = 8.0f;
  state.player->entityType = PLAYER;
  state.player->health = 3;

  state.player->textureID = playerTex;
  
  state.player->height = 0.95f;
  state.player->width = 0.95f;

  state.player->shotPower = 1.0f;

  //init bullets
  state.bullets = new Entity[BULLET_COUNT];
  for (int i = 0; i < BULLET_COUNT; i++) {
    state.bullets[i].entityType = BULLET;
    state.bullets[i].textureID = bulletTex;
    state.bullets[i].speed = 16.0f;
    state.bullets[i].height = 0.3f;
    state.bullets[i].width = 0.3f;
  }

  //init enemy bullets
  state.enemyBullets = new Entity[ENEMY_BULLET_COUNT];
  for (int i = 0; i < ENEMY_BULLET_COUNT; i++) {
    state.enemyBullets[i].entityType = ENEMY_BULLET;
    state.enemyBullets[i].textureID = enemyBulletTex;
    state.enemyBullets[i].speed = 16.0f;
    state.enemyBullets[i].height = 0.3f;
    state.enemyBullets[i].width = 0.3f;
  }

  //init snipers
  state.enemies = new Entity[ENEMY_COUNT];
  for (int i = 0; i < 4; i++) {
    state.enemies[i].entityType = ENEMY;
    state.enemies[i].enemyState = ENTERING;
    state.enemies[i].isActive = true;
    state.enemies[i].enemyType = SNIPER;
    state.enemies[i].textureID = sniperTex;
    state.enemies[i].shotPower = 1;
    state.enemies[i].height = 0.95f;
    state.enemies[i].width = 0.95f;
    state.enemies[i].speed = 2.0f + i;
    state.enemies[i].position = glm::vec3(0.0f, 40.0f + i, 0.0f);
    state.enemies[i].velocity.x = 1.0f;
  }

  //init bombers
  for (int i = 4; i < 9; i++) {
    state.enemies[i].entityType = ENEMY;
    state.enemies[i].enemyState = ENTERING;
    state.enemies[i].isActive = true;
    state.enemies[i].enemyType = BOMBER;
    state.enemies[i].textureID = bomberTex;
    state.enemies[i].shotPower = 1;
    state.enemies[i].height = 0.95f;
    state.enemies[i].width = 0.95f;
    state.enemies[i].speed = 15.0f;
    state.enemies[i].position = glm::vec3(-19.5f, -10 + i*27, 0.0f);
  }

  //init boss
  state.enemies[9].entityType = ENEMY;
  state.enemies[9].enemyType = BOSS;
  state.enemies[9].enemyState = IDLE;
  state.enemies[9].isActive = true;
  state.enemies[9].textureID = bossTex;
  state.enemies[9].shotPower = 3;
  state.enemies[9].height = 0.95f;
  state.enemies[9].width = 0.95f;
  state.enemies[9].speed = 4.0f;
  state.enemies[9].position = glm::vec3(0.0f, 18.0f, 0.0f);

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
              case SDLK_SPACE:
                state.player->shot = true;
                break;
              }
            break;
          }
      }
      
      const Uint8 *keys = SDL_GetKeyboardState(NULL);

      if (keys[SDL_SCANCODE_LEFT]) {
        state.player->movement.x = -1.0f;
      } else if (keys[SDL_SCANCODE_RIGHT]) {
        state.player->movement.x = 1.0f;
      }

      if (keys[SDL_SCANCODE_UP]) {
        state.player->movement.y = 1.0f;
      } else if (keys[SDL_SCANCODE_DOWN]) {
        state.player->movement.y = -1.0f;
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

        //update bullets
        for (int i = 0; i < BULLET_COUNT; i++) {
          state.bullets[i].Update(FIXED_TIMESTEP, NULL, NULL, 0, NULL, 0, NULL, 0);
        }

        //update enemy bullets
        for (int i = 0; i < ENEMY_BULLET_COUNT; i++) {
          state.enemyBullets[i].Update(FIXED_TIMESTEP, NULL, NULL, 0, NULL, 0, NULL, 0);
        }

        //update enemies
        int deadCount = 0;
        for (int i = 0; i < ENEMY_COUNT; i++) {
          if (state.enemies[i].collided) {
            switch (state.enemies[i].lastCollision->entityType) {
              case PLAYER:
                state.enemies[i].health = 0;
                break;
              case BULLET:
                state.enemies[i].health -= state.enemies[i].lastCollision->shotPower;
                state.enemies[i].lastCollision->isActive = false; //deactivate bullet that hit us
                break;
            }
            state.enemies[i].collided = false;
            state.enemies[i].lastCollision = NULL;
          }

          if (state.enemies[i].health <= 0) { state.enemies[i].isActive = false; state.enemies[i].enemyState = DEAD; }
          state.enemies[i].Update(FIXED_TIMESTEP, state.player, NULL, 0, state.enemyBullets, ENEMY_BULLET_COUNT, state.bullets, BULLET_COUNT);

          //check if boss should enter
          if (state.enemies[9].enemyState == IDLE) {
            if (state.enemies[i].enemyState == DEAD) { deadCount++;} 
            if (deadCount >= 4) {
              state.enemies[9].enemyState = ENTERING;
              BOSS_TEXT = true;
            }
          }
        }

        //update player
        if (state.player->collided) {
          switch (state.player->lastCollision->entityType) {
            case ENEMY:
              state.player->health = 0;
              break;
            case ENEMY_BULLET:
              state.player->health -= state.player->lastCollision->shotPower; 
              state.player->lastCollision->isActive = false; //deactive bullet that hit us
          }
          state.player->collided = false;
          state.player->lastCollision = NULL;
        }
        if (state.player->health <= 0) { mode = LOSE; }
        state.player->Update(FIXED_TIMESTEP, state.player, state.enemies, ENEMY_COUNT, state.enemyBullets, ENEMY_BULLET_COUNT, state.bullets, BULLET_COUNT);

        deltaTime -= FIXED_TIMESTEP;
      }
      accumulator = deltaTime;


      //if boss dies, victory
      if (state.enemies[9].enemyState == DEAD) {mode = WIN;}
      break;
  }

}

void Render() {
  glClear(GL_COLOR_BUFFER_BIT);

  switch (mode) {
    case WIN:
      DrawText(&program, *fontTexID, "VICTORY!", 2.0f, -0.25f, glm::vec3(-7.0f, 1.0f, 0.0f));
      break;
    case LOSE:
      DrawText(&program, *fontTexID, "YOU DIED", 2.0f, -0.25f, glm::vec3(-5.0f, 1.0f, 0.0f));
      break;
  }
  //draw health
  DrawText(&program, *fontTexID, "HEALTH:" + std::to_string(state.player->health), 1.5f, -0.25f, glm::vec3(-19.0f, -14.5f, 0.0f));

  //draw boss health
  if (BOSS_TEXT) {
    DrawText(&program, *fontTexID, "BOSS HEALTH:" + std::to_string(state.enemies[9].health), 1.5f, -0.25f, glm::vec3(-19.0f, 14.0f, 0.0f));
  }

  //render bullets
  for (int i = 0; i < BULLET_COUNT; i++) {
    state.bullets[i].Render(&program);
  }

  //render enemy bullets
  for (int i = 0; i < ENEMY_BULLET_COUNT; i++) {
    state.enemyBullets[i].Render(&program);
  }

  //render enemies
  for (int i = 0; i < ENEMY_COUNT; i++) {
    state.enemies[i].Render(&program);
  }

  //render player
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

