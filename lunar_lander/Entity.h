#pragma once
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


enum EntityType { PLAYER, WIN_PLATFORM, LOSE_PLATFORM, NONE };

class Entity {
public:
    EntityType entityType;
    bool isActive = true;

    bool collidedTop = false;
    bool collidedBottom = false;
    bool collidedRight = false;
    bool collidedLeft = false;
    EntityType lastCollision = NONE;

    glm::vec3 position;
    glm::vec3 movement;
    glm::vec3 acceleration;
    glm::vec3 velocity;
    float speed;

    float width = 1.0f;
    float height = 1.0f;

    bool jump = false;
    float jumpPower = 0.0f;

    GLuint textureID;

    glm::mat4 modelMatrix;

    Entity();

    bool checkCollision(Entity *other);
    void checkCollisionsY(Entity *objects, int objCount);
    void checkCollisionsX(Entity *objects, int objCount);
    void Update(float deltaTime, Entity *platforms, int platformCount);
    void Render(ShaderProgram *program);
    void DrawSpriteFromTextureAtlas(ShaderProgram *program, GLuint textureID, int index);
};

