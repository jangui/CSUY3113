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
    lastCollision = other->entityType;
    return true;
  }
  return false;
}

void Entity::checkCollisionsY(Entity *objects, int objCount) {
  for (int i = 0; i < objCount; i++) {
    Entity *object = &objects[i];

    if (checkCollision(object)) {
      float ydist = std::fabs(position.y - object->position.y);
      float penetrationY = std::fabs(ydist - (height / 2.0f) - (object->height / 2.0f));
      if (velocity.y > 0) {
        position.y -= penetrationY;
        collidedTop = true;
      } else if (velocity.y < 0) {
        position.y += penetrationY;
        collidedBottom = true;
      }
      velocity.y = 0;
    }
  }
}

void Entity::checkCollisionsX(Entity *objects, int objCount) {
  for (int i = 0; i < objCount; i++) {
    Entity *object = &objects[i];

    if (checkCollision(object)) {
      float xdist = std::fabs(position.x - object->position.x);
      float penetrationX = std::fabs(xdist - (width / 2.0f) - (object->width / 2.0f));
      if (velocity.x > 0) {
        position.x -= penetrationX;
        collidedRight = true;
      } else if (velocity.x < 0) {
        position.x += penetrationX;
        collidedLeft = true;
      }
      velocity.x = 0;
    }
  }
}

void Entity::Update(float deltaTime, Entity *platforms, int platformCount) {
  if (isActive == false) { return; }

  collidedTop = false;
  collidedBottom = false;
  collidedRight = false;
  collidedLeft = false;

  if (jump) {
    jump = false;
    velocity.y += jumpPower;
  }
  
  velocity.x = movement.x * speed;
  velocity += acceleration * deltaTime;

  position.y += velocity.y * deltaTime;       //move on Y
  checkCollisionsY(platforms, platformCount);  //fix if needed

  position.x += velocity.x * deltaTime;       //move on X
  checkCollisionsX(platforms, platformCount);  //fix if needed


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

