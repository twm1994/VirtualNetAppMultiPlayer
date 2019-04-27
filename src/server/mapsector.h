/*
 (c) 2010 Perttu Ahola <celeron55@gmail.com>
 */

#ifndef MAPSECTOR_HEADER
#define MAPSECTOR_HEADER

#include "common_irrlicht.h"
#include "mapblock.h"
#include "exceptions.h"
#include "../thread/jmutex.h"
#include "exceptions.h"
#include "../thread/jmutexautolock.h"

using namespace jthread;

/*
 This is an Y-wise stack of MapBlocks.

 TODO:
 - Add heightmap functionality
 - Implement node container functionality and modify Map to use it
 */

class MapSector;

class MapSector;

class BlockGenerator {
public:
    //=====Change from virtual function=====
    MapBlock * makeBlock(MapSector *sector, s16 block_y);
};

class MapSector: public NodeContainer {
private:
    // The pile of MapBlocks
    irr::core::map<s16, MapBlock*> m_blocks;
    //JMutex m_blocks_mutex; // For public access functions

    NodeContainer *m_parent;
    // Position on parent (in MapBlock widths)
    v2s16 m_pos;

    MapBlock *getBlockBuffered(s16 y);

    // Be sure to set this to NULL when the cached block is deleted
    MapBlock *m_block_cache;
    s16 m_block_cache_y;
    JMutex m_mutex;

    BlockGenerator *m_block_gen;
public:

    MapSector(NodeContainer *parent, v2s16 pos, BlockGenerator *gen) :
            m_parent(parent), m_pos(pos), m_block_cache(NULL), m_block_gen(gen) {
        m_mutex.Init();
        assert(m_mutex.IsInitialized());
    }

    ~MapSector() {
        //TODO: clear m_blocks
        irr::core::map<s16, MapBlock*>::Iterator i = m_blocks.getIterator();
        for (; i.atEnd() == false; i++) {
            delete i.getNode()->getValue();
        }
    }

    v2s16 getPos() {
        return m_pos;
    }

    MapBlock * getBlockNoCreate(s16 y);
    MapBlock * createBlankBlockNoInsert(s16 y);
    MapBlock * createBlankBlock(s16 y);
    MapBlock * getBlock(s16 y);
    void insertBlock(MapBlock *block);
    irr::core::list<MapBlock*> getBlocks();
};

#endif

