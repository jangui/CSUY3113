#include "Entity.h"

Entity::Entity() {
  position = glm::vec3(0);
  acceleration = glm::vec3(0);
  velocity = glm::vec3(0);
  speed = 0;
  
  modelMatrix = glm::mat4(1.0f);
}

bool Entity::checkCollision(Entity *other) {
  if (isActive == false || other->isActive == false) { return false; }

  float xdist = std::fabs(position.x - other->position.x) - ((width + other->width)/2);
  float ydist = std::fabs(position.y - other->position.y) - ((height + other->height)/2);

  if (xdist < 0 && ydist < 0) {
    lastCollision = other;
    return true;
  }
  return false;
}

void Entity::checkCollisions(Entity *objects, int objCount) {
  for (int i = 0; i < objCount; i++) {
    Entity *object = &objects[i];

    if (checkCollision(object)) {
      collided = true;
    }
  }
}

void Entity::Update(float deltaTime, Entity *player, Entity *enemies, int enemyCount, Entity *enemyBullets, int enemyBulletCount, Entity *bullets, int bulletCount) {
  if (isActive == false) { return; }

  collided = false;

  float new_y;
  float new_x;
  switch (entityType) {
    case PLAYER:
      velocity.x = movement.x * speed;
      velocity.y = movement.y * speed;
      velocity += acceleration * deltaTime;

      //check bounds + collisions and move
      new_y = position.y + velocity.y * deltaTime;
      if (new_y > 14.5f) {
        position.y = 14.5f;
      } else if (new_y < -14.5f) {
        position.y = -14.5f;
      } else {
        position.y += velocity.y * deltaTime;       //move on Y
      }

      new_x = position.x + velocity.x * deltaTime;
      if (new_x > 19.5f) {
        position.x = 19.5f;
      } else if (new_x < -19.5f) {
        position.x = -19.5f;
      } else {
        position.x += velocity.x * deltaTime;       //move on X
      }

      checkCollisions(enemies, enemyCount);
      checkCollisions(enemyBullets, enemyBulletCount);

      //shoot bullet
      if (shot) {
        shot = false;
        for (int i = 0; i < bulletCount; i++) {
          Entity *bullet = &bullets[i];
          if (bullet->isActive == false) {
            bullet->isActive = true;
            bullet->position = position;
            bullet->velocity.y = 1.0f * bullet->speed;
            bullet->shotPower = shotPower;
            bullet->modelMatrix = glm::mat4(1.0f);
            bullet->modelMatrix = glm::translate(bullet->modelMatrix, bullet->position);
            break;
          }
        }
      }
      break;
    case BULLET:
      position.y += velocity.y * deltaTime;
      if (position.y > 16.0f) { isActive = false; }
      break;
    case ENEMY:
      AI(deltaTime, player, enemyBullets, enemyBulletCount, enemies, enemyCount);
      checkCollisions(bullets, bulletCount);
      break;
    case ENEMY_BULLET:
      position.y += velocity.y * deltaTime;
      position.x += velocity.x * deltaTime;
      if (std::fabs(position.y) > 16.0f) { isActive = false; }
      if (std::fabs(position.x) > 21.0f) { isActive = false; }
      break;
  }

  modelMatrix = glm::mat4(1.0f);
  modelMatrix = glm::translate(modelMatrix, position);
}

void Entity::Render(ShaderProgram *program) {
  if (isActive == false) { return; }

  program->SetModelMatrix(modelMatrix);
  
  float vertices[]  = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
  float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
  
  glBindTexture(GL_TEXTURE_2D, textureID);
  
  glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
  glEnableVertexAttribArray(program->positionAttribute);
  
  glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
  glEnableVertexAttribArray(program->texCoordAttribute);
  
  glDrawArrays(GL_TRIANGLES, 0, 6);
  
  glDisableVertexAttribArray(program->positionAttribute);
  glDisableVertexAttribArray(program->texCoordAttribute);
}


void Entity::AI(float deltaTime, Entity *player, Entity *enemyBullets, int enemyBulletCount, Entity *enemies, int enemyCount) {
  switch (enemyType) {
    case SNIPER:
      AISniper(deltaTime, player, enemyBullets, enemyBulletCount);
      break;
    case BOMBER:
      AIBomber(deltaTime, enemyBullets, enemyBulletCount);
      break;
    case BOSS:
      AIBoss(deltaTime, enemyBullets, enemyBulletCount, enemies, enemyCount);
      break;
  }
}

void Entity::AISniper(float deltaTime, Entity *player, Entity *enemyBullets, int enemyBulletCount) {
  switch (enemyState) {
    case IDLE:
      break;

    case ENTERING:
      //high health while entering
      health = 100.0f;
      velocity.y = -1.0f;
      timer += deltaTime;
      if (timer > 4.0f) {
        timer = 0.0f;
        velocity.x *= -1.0f;
      }
      if (position.y < 5.0f) {
        health = 1.0f;
        enemyState = DEFAULT;
      }
      break;

    case DEFAULT:
      timer += deltaTime;
      if (timer > 1.5f) {
        shot = true;
        timer = 0.0f;
        velocity.y *= -1.0f;
      }
      if (player->position.x > position.x) { velocity.x = 1.0f; } 
      else if (player->position.x < position.x) { velocity.x = -1.0f; } 
      else { velocity.x = 0.0f; }

      //shoot
      if (shot) {
        shot = false;
        for (int i = 0; i < enemyBulletCount; i++) {
          Entity *bullet = &enemyBullets[i];
          if (bullet->isActive == false) {
            bullet->isActive = true;
            bullet->position = position;
            bullet->velocity.y = -1.0f * bullet->speed;
            bullet->shotPower = shotPower;
            bullet->modelMatrix = glm::mat4(1.0f);
            bullet->modelMatrix = glm::translate(bullet->modelMatrix, bullet->position);
            break;
          }
        }
      }

      break;
  }
  position.x += velocity.x * speed * deltaTime;
  position.y += velocity.y * speed * deltaTime;
}

void Entity::AIBomber(float deltaTime, Entity *enemyBullets, int enemyBulletCount) {
  switch (enemyState) {
    case IDLE:
      break;
    case ENTERING:
      //high health while entering
      health = 100.0f;
      if (position.y >= 14.5) {
        velocity.y = -1.0f;
      } else {
        health = 1.0f;
        enemyState = DEFAULT;
      }
      break;
    case DEFAULT:
      timer += deltaTime;
      if (timer > 2.5f) {
        shot = true;
        timer = 0.0f;
      }
      if (position.x <= -19.5f && position.y >= 14.5f) {
        position.x = -19.5f;
        velocity.y = -1.0f;
        velocity.x = 0.0f;
      }
      else if (position.y >= 14.5f) {
        position.y = 14.5f;
        velocity.x = -1.0f;
        velocity.y = 0.0f;
      }
      else if (position.x >= 19.5f) {
        position.x = 19.5f;
        velocity.y = 1.0f;
        velocity.x = 0.0f;
      }
      else if (position.y <= -14.5f) {
        position.y = -14.5f;
        velocity.x = 1.0f;
        velocity.y = 0.0f;
      }
      break;
  }
  //shooting
  int count = 0;
  float xs[] = { 0.0f, 0.0f, 1.0f, -1.0f };
  float ys[] = { -1.0f, 1.0f, 0.0f, 0.0f };
  if (shot) {
    shot = false;
    for (int i = 0; i < enemyBulletCount; i++) {
      Entity *bullet = &enemyBullets[i];
      if (bullet->isActive == false) {
        bullet->isActive = true;
        bullet->position = position;
        bullet->velocity.y = ys[count] * bullet->speed;
        bullet->velocity.x = xs[count] * bullet->speed;
        bullet->shotPower = shotPower;
        bullet->modelMatrix = glm::mat4(1.0f);
        bullet->modelMatrix = glm::translate(bullet->modelMatrix, bullet->position);
        count += 1;
        if (count == 4) { break; }
      }
    }
  }
  position.x += velocity.x * speed * deltaTime;
  position.y += velocity.y * speed * deltaTime;
}

void Entity::AIBoss(float deltaTime, Entity *enemyBullets, int enemyBulletCount, Entity *enemies, int enemyCount) {
  int deadCount = 0;
  switch (enemyState) {
    case IDLE:
      break;
    case ENTERING:
      //high health while entering;
      health = 9000.0f;
      velocity.y = -1.0f;
      velocity.x = 0.0f;
      if (position.y <= 12.5f) {
        enemyState = DEFAULT;
        velocity.x = 1.0f;
        health = 5;
      }
      break;
    case DEFAULT:
      timer += deltaTime;
      if (timer > 1.5f) {
        timer = 0.0f;
        velocity.y *= -1.0f;
      }
      timer2 += deltaTime;
      if (timer2 > 1.0f) {
        timer2 = 0.0f;
        shot = true;
      }
      if (position.x >= 15.0f) {velocity.x = -1.0f;}
      else if (position.x <= -15.0f) {velocity.x = 1.0f;}
      break;
  }

  int count = 0;
  float xs[] = { -1.0f, 0.0f, 1.0f };
  float ys[] = { -1.0f, -1.0f, -1.0f};
  if (shot) {
    shot = false;
    for (int i = 0; i < enemyBulletCount; i++) {
      Entity *bullet = &enemyBullets[i];
      if (bullet->isActive == false) {
        bullet->isActive = true;
        bullet->position = position;
        bullet->velocity.y = ys[count] * bullet->speed;
        bullet->velocity.x = xs[count] * bullet->speed;
        bullet->shotPower = shotPower;
        bullet->modelMatrix = glm::mat4(1.0f);
        bullet->modelMatrix = glm::translate(bullet->modelMatrix, bullet->position);
        count += 1;
        if (count == 3) { break; }
      }
    }
  }
  position.x += velocity.x * speed * deltaTime;
  position.y += velocity.y * speed * deltaTime;
}
