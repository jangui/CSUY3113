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

enum EntityType { PLAYER, ENEMY, BULLET, ENEMY_BULLET, NONE };
enum EnemyType { BOMBER, SNIPER, BOSS };
enum EnemyState { IDLE, ENTERING, DEFAULT, DEAD }; 

class Entity {
public:
    EntityType entityType = NONE;
    EnemyType enemyType;
    EnemyState enemyState = IDLE;

    bool isActive = false;

    bool collided = false;
    Entity *lastCollision = NULL;

    glm::vec3 position;
    glm::vec3 movement;
    glm::vec3 acceleration;
    glm::vec3 velocity;
    float speed;
    int health = 1.0f;

    float width = 1.0f;
    float height = 1.0f;
    float scale = 1.0f;

    float timer = 0.0f;
    float timer2 = 0.0f;

    bool shot = false;
    int shotPower = 0;

    GLuint textureID;

    glm::mat4 modelMatrix;

    Entity();

    bool checkCollision(Entity *other);
    void checkCollisions(Entity *objects, int objCount);
    void Update(float deltaTime, Entity *player, Entity *enemies, int enemyCount, Entity *enemyBullets, int enemyBulletCount, Entity *bullets, int bulletCount);
    void Render(ShaderProgram *program);
    void DrawSpriteFromTextureAtlas(ShaderProgram *program, GLuint textureID, int index);
    void AI(float deltaTime, Entity *player, Entity *enemyBullets, int enemyBulletCount, Entity *enemies, int enemyCount);
    void AISniper(float deltaTime, Entity *player, Entity *enemyBullets, int enemyBulletCount);
    void AIBomber(float deltaTime, Entity *enemyBullets, int enemyBulletCount);
    void AIBoss(float deltaTime, Entity *enemyBullets, int enemyBulletCount, Entity *enemies, int enemyCount);
};

