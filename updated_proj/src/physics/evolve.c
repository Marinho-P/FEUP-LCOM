#include "evolve.h"
#include "physicsMacros.h"
#include "utilities.h"
#include <math.h>

void evolveBallMotion(Table *table, Ball *ball, double time) {
  if (ball->transition != NULL) {
    ball->transition->time -= time;
  }

  switch (getBallState(ball)) {

    case STATIONARY:
    case POCKETED:
      return;
      break;
    case SLIDING: {
      double slideTime = getSlideTime(ball, table->slidingFriction, table->gravityAcceleration);

      evolveSlideState(ball, MIN(slideTime, time), table->spinningFriction, table->slidingFriction, table->gravityAcceleration);
      if (time >= slideTime) {
        ball->state = ROLLING;
        time -= slideTime;
      }
      else
        return;
      break;
    }

    case ROLLING: {

      double rollTime = getRollTime(ball, table->rollingFriction, table->gravityAcceleration);

      evolveRollState(ball, MIN(time, rollTime), table->rollingFriction, table->spinningFriction, table->gravityAcceleration);

      if (time >= rollTime) {
        ball->state = SPINNING;
        time -= rollTime;
      }
      else
        return;

      break;
    }

    case SPINNING: {

      double spinTime = getSpinTime(ball, table->spinningFriction, table->gravityAcceleration);

      evolvePrependicularSpin(ball, MIN(spinTime, time), table->spinningFriction, table->gravityAcceleration);

      if (time >= spinTime) {
        ball->state = STATIONARY;
        time -= spinTime;
      }
      else
        return;

      break;
      default:
        break;
    }
  }
}

void evolveRollState(Ball *ball, double t, double uRolling, double uSpinning, double g) {


  if (t == 0)
    return;

  vector_t v0 = normalizeVector(ball->velocity);

  vector_t displacement = { ball->velocity.x * t - 0.5 * uRolling * g * t * t * v0.x,  ball->velocity.y * t - 0.5 * uRolling * g * t * t * v0.y};
  ball->position.x += displacement.x;
  ball->position.y += displacement.y;

  evolveXYRotationAmount(ball, displacement);

  ball->velocity.x = ball->velocity.x - uRolling * g * t * v0.x;
  ball->velocity.y = ball->velocity.y - uRolling * g * t * v0.y;

  vector_t temp = {ball->velocity.x / ball->radius, ball->velocity.y / ball->radius};
  vector_t xyAngVelocity = rotate2d(temp, M_PI / 2);
  ball->ang_velocity.x = xyAngVelocity.x;
  ball->ang_velocity.y = xyAngVelocity.y;
  evolvePrependicularSpin(ball, t, uSpinning, g);
}

void evolveSlideState(Ball *ball, double t, double uSpinning, double uSliding, double g) {

  if (t == 0)
    return;

  // Calculate the angle for coordinate transformation
  double ang = angle(ball->velocity);

  // Change coordinates (aka. rotate) each component
  vector_t velocityB = rotate2d(ball->velocity, -ang);

  vector_t xyAngVelocityB = {ball->ang_velocity.x, ball->ang_velocity.y};
  xyAngVelocityB = rotate2d(xyAngVelocityB, -ang);

  vector_t relVelocityB = rotate2d(normalizeVector(relativeVelocity(ball)), -ang);

  // Use the formulas
  vector_t positionB;
  positionB.x = velocityB.x * t - 0.5 * uSliding * g * t * t * relVelocityB.x;
  positionB.y = -0.5 * uSliding * g * t * t * relVelocityB.y;

  velocityB.x = velocityB.x - uSliding * g * t * relVelocityB.x;
  velocityB.y = velocityB.y - uSliding * g * t * relVelocityB.y;

  double denominator = 1.0 / (2.0 * ball->radius);

  vector_t displacement = {xyAngVelocityB.x * t - 5.0 / 2.0 * denominator * uSliding * g * t * t * relVelocityB.y, xyAngVelocityB.y * t - 5.0 / 2.0 * denominator * uSliding * g * t * t * -relVelocityB.x};
  evolveXYRotationAmount(ball, displacement);

  xyAngVelocityB.x = xyAngVelocityB.x - 5 * denominator * uSliding * g * t * relVelocityB.y;
  xyAngVelocityB.y = xyAngVelocityB.y - 5 * denominator * uSliding * g * t * -relVelocityB.x;

  evolvePrependicularSpin(ball, uSpinning, g, t);

  // Return coords to table coordinates

  vector_t positionT = rotate2d(positionB, ang);
  ball->position.x = ball->position.x + positionT.x;
  ball->position.y = ball->position.y + positionT.y;

  ball->velocity = rotate2d(velocityB, ang);

  vector_t xyAngVelocityT = rotate2d(xyAngVelocityB, ang);
  ball->ang_velocity.x = xyAngVelocityT.x;
  ball->ang_velocity.y = xyAngVelocityT.y;
}

void evolvePrependicularSpin(Ball *ball, double t, double uSpinning, double g) {

  // If angVelocity is really small or time is 0
  if (abs(ball->ang_velocity.z) < EPS || t==0) {
    return;
  }

  double alpha = 5 * uSpinning * g / (2 * ball->radius);
  // Limit t so that decay stops at 0
  t = MIN(t, ball->ang_velocity.z / alpha);
  int sign = ball->ang_velocity.z > 0 ? 1 : -1;
  ball->rotation.z += ball->ang_velocity.z * t - (sign*alpha*t*t)/2;
  ball->rotation.z = fmod(ball->rotation.z, 2.0*M_PI);
  if (ball->rotation.z < 0 ) ball->rotation.z += 2.0*M_PI;

  ball->ang_velocity.z = ball->ang_velocity.z - sign * alpha * t;

}

void evolveXYRotationAmount(Ball *ball, vector_t displacement) {
 
 double fullRotation = 2.0*M_PI;

  rotate2d(displacement, ball->rotation.z);

  ball->rotation.x += displacement.x / (ball->radius * 2 * M_PI) * fullRotation + M_PI;
  ball->rotation.x = fmod(ball->rotation.x, fullRotation);
  if (ball->rotation.x < 0) ball->rotation.x += fullRotation;
  ball->rotation.x -= M_PI;

  ball->rotation.y += displacement.y / (ball->radius * 2 * M_PI) * fullRotation + M_PI;
  ball->rotation.y = fmod(ball->rotation.y, fullRotation);
  if (ball->rotation.y < 0) ball->rotation.y += fullRotation;
  ball->rotation.y -= M_PI;
}
