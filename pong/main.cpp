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

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

float getDeltaTime();

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
  Object(glm::vec3 player_pos, GLuint textureID);
  virtual void update(float deltaTime) = 0;
  virtual void draw();
  float getX();
  float getY();
protected:
  GLuint textureID;
  glm::mat4 modelMatrix;
  glm::vec3 player_pos;
  float player_speed;
};

class Player: public Object {
public:
  Player(glm::vec3 player_pos, GLuint textureID);
  void update(float deltaTime);
};

void Initialize(std::vector<Object*> *objs) {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Textured", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
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
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    
    glUseProgram(program.programID);
    
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    //enable blending and set transparency settings
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //we can try nearest neighbord instead

    //load texture ids
    Texture playerTex("player.png");

    //create objects
    Player *p1 = new Player(glm::vec3(-4.5f, -2.0f, 0.0f), playerTex.getTextureID());
    objs->push_back(p1);
}

void ProcessInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            gameIsRunning = false;
        }
    }
}

void Update(std::vector<Object*> *objs) {
  float deltaTime = getDeltaTime();
  for (size_t i = 0; i < objs->size(); i++) {
    ((*objs)[i])->update(deltaTime);
  }
}

void Render(std::vector<Object*> *objs) {
    float vertices[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
    float textCoords[] = {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };

    glClear(GL_COLOR_BUFFER_BIT);

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, textCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);

    //draw objects
    for (size_t i = 0; i < objs->size(); i++) {
      ((*objs)[i])->draw();
    }

    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    
    SDL_GL_SwapWindow(displayWindow);
}

void Shutdown(std::vector<Object*> *objs) {
    for (size_t i = 0; i < objs->size(); i++) {
      free((*objs)[i]);
    }
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    std::vector<Object*> objs;
    Initialize(&objs);
    
    while (gameIsRunning) {
        ProcessInput();
        Update(&objs);
        Render(&objs);
    }
    
    Shutdown(&objs);
    return 0;
}

float getDeltaTime() {
  static float lastTicks = 0;
  float ticks = (float)SDL_GetTicks() / 1000.0f;
  float deltaTime = ticks - lastTicks;
  lastTicks = ticks;
  return deltaTime;
}

Object::Object(glm::vec3 player_pos, GLuint textureID)
       : player_pos(player_pos), textureID(textureID)
{
  //set model matrix to initial x and y
  modelMatrix = glm::translate(glm::mat4(1.0f), player_pos);
}

void Object::draw() {
  program.SetModelMatrix(modelMatrix);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

float Object::getX() { return player_pos.x; }
float Object::getY() { return player_pos.y; }

Player::Player(glm::vec3 player_pos, GLuint textureID)
      : Object(player_pos, textureID) { }

void Player::update(float deltaTime) {
  /*
  if (std::fabs(dist) > 1.0f) {
    dir *= -1.0f;
    dist = 0;
  }
  x += dir * deltaTime;
  dist += dir * deltaTime;
  */

  modelMatrix = glm::translate(glm::mat4(1.0f), player_pos);
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

