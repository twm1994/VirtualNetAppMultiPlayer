/*
 modified on player.h
 */

#ifndef NPC_HEADER
#define NPC_HEADER

#define NPC_ID_NEW 100

#include <irrlicht.h>
#include "common_irrlicht.h"
#include "map.h"

using namespace irr;
//typedef core::vector3df v3f;
//typedef core::vector3d<s16> v3s16;

class Map;

class NPC {
public:

    NPC();
    ~NPC();

    void move(f32 dtime, Map &map);

    void randomWalk(f32 dtime, Map &map);

    v3f getPosition() {
        return m_position;
    }

    void setPosition(v3f position) {
        m_position = position;
    }

    v3f getRotation() {
        return m_rotation;
    }

    void setRotation(v3f rotation) {
        m_rotation = rotation;
    }

    int getStepCounter() {
        return m_step_counter;
    }

    void increaseStepCounter() {
        m_step_counter++;
    }

    void resetStepCounter() {
        m_step_counter = 0;
    }

    int getStepMax() {
        return m_step_max;
    }

    v3f speed;
    bool touching_ground;
    u16 npc_id;

private:
    int m_step_counter = 0;
    int m_step_max = 400; // max game step to reset walking direction
    f32 m_walk_speed = 4.0 * BS;
    v3f m_speed_vector = v3f(0, 0, m_walk_speed);
    f32 m_yaw;
    v3f m_position;
    v3f m_rotation;
    bool jump_trigger; // set NPC jumping
};

#endif

