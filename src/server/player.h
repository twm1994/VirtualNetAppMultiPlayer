/*
 (c) 2010 Perttu Ahola <celeron55@gmail.com>
 */

#ifndef PLAYER_HEADER
#define PLAYER_HEADER

#include "map.h"
#include "connection.h"
#include "common_irrlicht.h"

using namespace irr;
typedef irr::core::vector3df v3f;
typedef irr::core::vector3d<s16> v3s16;

class Map;

/*
 TODO: Make this a scene::ISceneNode
 */

class Player {
public:
    Player(bool is_local);

    Player(v3f speed, bool touching_ground, u16 peer_id, float timeout_counter,
            bool is_local, v3f position, v3f rotation);

    ~Player();

    void move(f32 dtime, Map &map);

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

    bool isLocal() {
        return m_is_local;
    }

    /*
     * current velocity of player:
     * X-axis: left-right-wise
     * Y-axis: top-down-wise
     * Z-axis: head-rear-wise
     */
    v3f speed;
    bool touching_ground;
    u16 peer_id;
    float timeout_counter;

private:
    bool m_is_local;
    v3f m_position;
    v3f m_rotation;
};

#endif

