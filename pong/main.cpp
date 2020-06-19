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

#include <vector>

SDL_Window* displayWindow;
bool gameIsRunning = true;
const float ORTHO_WIDTH = 5.0f;
const float ORTHO_HEIGHT = 3.75f;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;


// I guess we don't really need this class because we only have one useful method
// Though I'm going to keep it in case the future maybe extending it comes in handy
class Texture {
public:
  Texture(const char *filePath);
  GLuint loadTexture(const char* filePath);
  GLuint getTextureID();
private:
  GLuint textureID;
};


class Object {
public:
  Object(glm::vec3 position, GLuint textureID);
  virtual void update(float deltaTime) = 0;
  virtual void draw();
  float getX();
  float getY();
  float getW();
  float getH();
  void setMoveX(float x);
  void setMoveY(float y);
protected:
  GLuint textureID;
  glm::mat4 modelMatrix;
  glm::vec3 position;
  glm::vec3 movement;
  float speed;
  float width;
  float height;
};

class Player: public Object {
public:
  Player(glm::vec3 position, GLuint textureID);
  void update(float deltaTime);
  void checkCollisions(glm::vec3 *new_pos);
};

class Ball: public Object {
public:
  Ball(glm::vec3 position, GLuint textureID);
  void update(float deltaTime);
  void checkCollisions(glm::vec3 *new_pos);
  void start();
private:
  bool moving;
};

std::vector<Object*> objs;

float getDeltaTime();
bool isColliding(glm::vec3 *new_pos, float width, float height, Object *obj);

void Initialize() {
  SDL_Init(SDL_INIT_VIDEO);
  displayWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   640, 480, SDL_WINDOW_OPENGL);
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

  //enable blending and set transparency settings
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //we can try nearest neighbord instead

  //load texture ids
  Texture playerTex("player.png");
  Texture ballTex("ball.png");

  //create objects
  Player *p1 = new Player(glm::vec3(-4.5f, 0.0f, 0.0f), playerTex.getTextureID());
  objs.push_back(p1);

  Player *p2 = new Player(glm::vec3(4.5f, 0.0f, 0.0f), playerTex.getTextureID());
  objs.push_back(p2);
  
  Ball *ball = new Ball(glm::vec3(0.0f, 0.0f, 0.0f), ballTex.getTextureID());
  objs.push_back(ball);
}

void ProcessInput() {
  //proccess event
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case(SDL_QUIT):
      case(SDL_WINDOWEVENT_CLOSE):
        gameIsRunning = false;
        break;
      case(SDL_KEYDOWN):
        //key was pressed, which one?
        switch(event.key.keysym.sym) {
          case SDLK_SPACE:
            //start ball when space pressed
            Ball *ball = static_cast<Ball*>(objs[2]);
            ball->start();
        }
        break;
    }
  }
  //check for keys being held down
  const Uint8 *keys = SDL_GetKeyboardState(NULL);
  //move p1
  if(keys[SDL_SCANCODE_W]) {
    objs[0]->setMoveY(1.0f);
  } else if(keys[SDL_SCANCODE_S]) {
    objs[0]->setMoveY(-1.0f);
  }
  //move p2
  if(keys[SDL_SCANCODE_UP]) {
    objs[1]->setMoveY(1.0f);
  } else if(keys[SDL_SCANCODE_DOWN]) {
    objs[1]->setMoveY(-1.0f);
  }

}

void Update() {
  float deltaTime = getDeltaTime();
  for (size_t i = 0; i < objs.size(); i++) {
    objs[i]->update(deltaTime);
  }
}

void Render() {
  float vertices[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
  float textCoords[] = {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };

  glClear(GL_COLOR_BUFFER_BIT);

  glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
  glEnableVertexAttribArray(program.positionAttribute);
  glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, textCoords);
  glEnableVertexAttribArray(program.texCoordAttribute);

  //draw objects
  for (size_t i = 0; i < objs.size(); i++) {
    objs[i]->draw();
  }

  glDisableVertexAttribArray(program.positionAttribute);
  glDisableVertexAttribArray(program.texCoordAttribute);
  
  SDL_GL_SwapWindow(displayWindow);
}

void Shutdown() {
  for (size_t i = 0; i < objs.size(); i++) {
    free(objs[i]);
  }
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

float getDeltaTime() {
  static float lastTicks = 0;
  float ticks = (float)SDL_GetTicks() / 1000.0f;
  float deltaTime = ticks - lastTicks;
  lastTicks = ticks;
  return deltaTime;
}

Object::Object(glm::vec3 position, GLuint textureID)
       : position(position), textureID(textureID)
{
  //set model matrix to initial position
  modelMatrix = glm::translate(glm::mat4(1.0f), position);

  //init movement vector to 0
  movement = glm::vec3(0.0f, 0.0f, 0.0f);
}

void Object::draw() {
  program.SetModelMatrix(modelMatrix);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

float Object::getX() { return position.x; }
float Object::getY() { return position.y; }
float Object::getW() { return width; }
float Object::getH() { return height; }

void Object::setMoveX(float x) { movement.x = x; }
void Object::setMoveY(float y) { movement.y = y; }


Player::Player(glm::vec3 position, GLuint textureID)
      : Object(position, textureID) { speed = 5.0f; width = 0.25f; height = 0.9f; }

void Player::update(float deltaTime) {
  //normalize movement vector
  if (glm::length(movement) > 1.0f) {
    movement = glm::normalize(movement);
  }
  glm::vec3 new_pos = position + (movement * speed * deltaTime);
  checkCollisions(&new_pos);

  position = new_pos;
  modelMatrix = glm::translate(glm::mat4(1.0f), position);

  //reset player_movement
  movement = glm::vec3(0.0f, 0.0f, 0.0f);
}

void Player::checkCollisions(glm::vec3 *new_pos) {
  float cutoff = ORTHO_HEIGHT - (height / 2);
  if ( (*new_pos).y > cutoff) { (*new_pos).y = cutoff; }
  else if ( (*new_pos).y < -cutoff) { (*new_pos).y = -cutoff; }
}

Ball::Ball(glm::vec3 position, GLuint textureID) : Object(position, textureID) {
  moving = false;
  speed = 7.0f;
  height = width = 0.2f;
}

void Ball::update(float deltaTime) {
  if (moving == false) { return; }
  if (glm::length(movement) > 1.0f) {
    movement = glm::normalize(movement);
  }
  glm::vec3 new_pos = position + (movement * speed * deltaTime);
  checkCollisions(&new_pos);

  position = new_pos;
  modelMatrix = glm::translate(glm::mat4(1.0f), position);

}

void Ball::checkCollisions(glm::vec3 *new_pos) {
  //check if hits top or bottom
  float cutoff = ORTHO_HEIGHT - (height / 2);
  if ( (*new_pos).y > cutoff) { (*new_pos).y = cutoff; movement.y = -movement.y; }
  else if ( (*new_pos).y < -cutoff) { (*new_pos).y = -cutoff; movement.y = -movement.y; }

  //check if hits player
  if (isColliding(new_pos, width, height, objs[0])) {
    movement.x = 1.0f;
  } else if (isColliding(new_pos, width, height, objs[1])) {
    movement.x = -1.0f;
  }

  //check if hits wall
  cutoff = ORTHO_WIDTH - (width / 2);
  if ((*new_pos).x > cutoff) {
    (*new_pos).x = cutoff;
    gameIsRunning = false;
  } else if ((*new_pos).x < -cutoff) {
    (*new_pos).x = -cutoff;
    gameIsRunning = false;
  }
}

void Ball::start() {
  moving = true;
  movement = glm::vec3(1.0f, 0.2f, 0.0f);
}

Texture::Texture(const char *filePath) {
  textureID = loadTexture(filePath);
}

GLuint Texture::loadTexture(const char* filePath) {
  int w, h, n;
  unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);
  
  if (image == NULL) {
    std::cout << "Unable to load image. Make sure the path is correct\n";
    assert(false);
  }

  //init texture id
  GLuint textureID;
  glGenTextures(1, &textureID);

  //bind texture to id
  glBindTexture(GL_TEXTURE_2D, textureID);

  //set texture pixel data & send image over to graphics card
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

  //Texture Filtering settings
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  //free image from ram now that its in graphics card ram
  stbi_image_free(image);

  //return id
  return textureID;
}

GLuint Texture::getTextureID() { return textureID; }

bool isColliding(glm::vec3 *new_pos, float width, float height, Object *obj) {
  float xdist = std::fabs((*new_pos).x - obj->getX()) - ((width + obj->getW()) / 2.0f);
  float ydist = std::fabs((*new_pos).y - obj->getY()) - ((height + obj->getH()) / 2.0f);
  if (xdist < 0 && ydist < 0) { return true; }
  return false;
}
