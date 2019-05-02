#ifndef ENVIRONMENT_HEADER
#define ENVIRONMENT_HEADER

/*
 This class is the game's environment.
 It contains:
 - The map
 - Players
 - Other objects
 - The current time in the game, etc.
 */

#include <list>
#include <ostream>
#include "common_irrlicht.h"
#include "player.h"
#include "map.h"
#include "npc.h"

class Environment {
public:
    // Environment will delete the map passed to the constructor
    Environment(Map *map, std::ostream &dout);
    ~Environment();

    void initNPC(float dtime);
    /*
     This can do anything to the environment:
     1) removing timed-out players,
     2) update player position through player move
     */
    void step(f32 dtime);

    Map & getMap();
    void setMap(Map* map);
    /*
     Environment deallocates players after use.
     */
    void addPlayer(Player *player);
    NPC* getNPC();
    void setNPC(NPC *npc);
    void removePlayer(Player *player);
    Player * getLocalPlayer();
    Player * getPlayer(u16 peer_id);
    irr::core::list<Player*> getPlayers();
private:
    Map *m_map;
    irr::core::list<Player*> m_players;
    NPC* m_npc;
    // Debug output goes here
    std::ostream &m_dout;
};

#endif

