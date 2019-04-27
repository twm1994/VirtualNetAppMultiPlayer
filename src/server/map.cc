/*
 (c) 2010 Perttu Ahola <celeron55@gmail.com>
 */

#include <fstream>
#include "map.h"
#include "../jsoncpp/json/json.h"

std::ofstream dfile_map("VirtualNet_Map.txt");
std::ostream dout_map(dfile_map.rdbuf());

/*
 void limitBox(irr::core::aabbox3d<s16> & box, irr::core::aabbox3d<s16> & limits)
 {
 if(box.MinEdge.X < limits.MinEdge.X)
 box.MinEdge.X = limits.MinEdge.X;
 if(box.MaxEdge.X > limits.MaxEdge.X)
 box.MaxEdge.X = limits.MaxEdge.X;
 if(box.MinEdge.Y < limits.MinEdge.Y)
 box.MinEdge.Y = limits.MinEdge.Y;
 if(box.MaxEdge.Y > limits.MaxEdge.Y)
 box.MaxEdge.Y = limits.MaxEdge.Y;
 if(box.MinEdge.Z < limits.MinEdge.Z)
 box.MinEdge.Z = limits.MinEdge.Z;
 if(box.MaxEdge.Z > limits.MaxEdge.Z)
 box.MaxEdge.Z = limits.MaxEdge.Z;
 }
 */

void * MapUpdateThread::Thread() {
    ;   // this function only works at client end
    return NULL;
}

s16 Map::MAP_LENGTH = 100;
s16 Map::MAP_WIDTH = 100;
s16 Map::MAP_HEIGHT = 10;
s16 Map::MAP_BOTTOM = -10;
const char* Map::SERVER_MAP_FILE = "";

Map::Map() :
        camera_position(0, 0, 0), camera_direction(0, 0, 1), updater(this), drawoffset(
                0, 0, 0) {
    m_getsector_mutex.Init();
    m_gensector_mutex.Init();
    camera_mutex.Init();
    assert(m_getsector_mutex.IsInitialized());
    assert(m_gensector_mutex.IsInitialized());
    assert(camera_mutex.IsInitialized());

    // Get this so that the player can stay on it at first
    //getSector(v2s16(0,0));
    map_load_mutex.Init();
//    std::cout << "map length: " << MAP_LENGTH << std::endl;
//    std::cout << "map width: " << MAP_WIDTH << std::endl;
//    std::cout << "map height: " << MAP_HEIGHT << std::endl;
//    std::cout << "map bottom: " << MAP_BOTTOM << std::endl;
}

Map::~Map() {
    /*
     Stop updater thread
     */
    updater.setRun(false);
    while (updater.IsRunning())
        sleep_s(1);

    /*
     Free all MapSectors.
     */
    irr::core::map<v2s16, MapSector*>::Iterator i = m_sectors.getIterator();
    for (; i.atEnd() == false; i++) {
        MapSector *sector = i.getNode()->getValue();
        delete sector;
    }

    m_nodes.clear();
}

/*
 TODO: These mutexes need rethinking. They don't work properly
 at the moment. Maybe. Or do they?
 */

/*bool Map::sectorExists(v2s16 p)
 {
 JMutexAutoLock lock(m_getsector_mutex);
 irr::core::map<v2s16, MapSector*>::Node *n = m_sectors.find(p);
 return (n != NULL);
 }*/

MapSector * Map::getSectorNoGenerate(v2s16 p) {
    //JMutexAutoLock lock(m_getsector_mutex);

    if (m_sector_cache != NULL && p == m_sector_cache_p) {
        MapSector * ref(m_sector_cache);
        return ref;
    }

    irr::core::map<v2s16, MapSector*>::Node *n = m_sectors.find(p);
    // If sector doesn't exist, throw an exception
    if (n == NULL) {
        /*
         TODO: Check if sector is stored on disk
         and load dynamically
         */

        //sector = NULL;
        throw InvalidPositionException();
    }

    MapSector *sector = n->getValue();

    // Cache the last result
    m_sector_cache_p = p;
    m_sector_cache = sector;

    //MapSector * ref(sector);

    return sector;
}

MapSector * Map::getSector(v2s16 pos) {
    // Stub virtual method
    assert(0);
    return getSectorNoGenerate(pos);
}

/*
 If sector doesn't exist, returns NULL
 Otherwise returns what the sector returns

 TODO: Change this to use exceptions?
 */
MapBlock * Map::getBlockNoCreate(v3s16 p3d) {
    /*std::cout<<"Map::getBlockNoCreate(("
     <<p3d.X<<","<<p3d.Y<<","<<p3d.Z
     <<")"<<std::endl;*/
    v2s16 p2d(p3d.X, p3d.Z);
    //v2s16 sectorpos = getNodeSectorPos(p2d);
    MapSector * sectorref = getSectorNoGenerate(p2d);
    MapSector *sector = sectorref;

    //JMutexAutoLock(sector->mutex);

    MapBlock * blockref = sector->getBlockNoCreate(p3d.Y);

    return blockref;
}

/*
 If sector doesn't exist, generates a sector.
 Returns what the sector returns.
 */
MapBlock * Map::getBlock(v3s16 p3d) {
    //=====Set boundary block and normal block differently=====
    v2s16 p2d(p3d.X, p3d.Z);
    MapSector * sref = getSector(p2d);
    MapSector *sector = sref;
    MapBlock * blockref = sector->getBlock(p3d.Y);
    if ((p3d.X > -1) && (p3d.X < MAP_LENGTH) && (p3d.Z > -1)
            && (p3d.Z < MAP_WIDTH)) {
        //=====Add stored node if there is any inside boundary=====
        for (s16 z = 0; z < MAP_BLOCKSIZE; z++) {
            for (s16 y = 0; y < MAP_BLOCKSIZE; y++) {
                for (s16 x = 0; x < MAP_BLOCKSIZE; x++) {
                    s16 minX = p3d.X * MAP_BLOCKSIZE;
                    s16 minY = p3d.Y * MAP_BLOCKSIZE;
                    s16 minZ = p3d.Z * MAP_BLOCKSIZE;
                    v3s16 nodepos = v3s16(minX + x, minY + y, minZ + z);
                    irr::core::map<v3s16, s16>::Node *n = m_nodes.find(nodepos);
                    if (n != NULL) {
                        MapNode node;
                        node.d = (n->getValue());
                        blockref->setNode(x, y, z, node);
                    }
                } // for(int x=0;x<MAP_BLOCKSIZE;x++
            } // for(int y=0;y<MAP_BLOCKSIZE;y++)
        } // for(int z=0;z<MAP_BLOCKSIZE;z++)
    }
    return blockref;
}

/*
 TODO: Check the order of changing lighting and recursing in
 these functions (choose the faster one)

 TODO: It has to done like this: cache the neighbours that were
 changed; then recurse to them.
 */

/*
 Goes recursively through the neighbours of the node.

 Alters only transparent nodes.

 If the lighting of the neighbour is lower than the lighting of
 the node was (before changing it to 0 at the step before), the
 lighting of the neighbour is set to 0 and then the same stuff
 repeats for the neighbour.

 Some things are made strangely to make it as fast as possible.

 Usage:
 irr::core::list<v3s16> light_sources;
 irr::core::map<v3s16, MapBlock*> modified_blocks;
 f32 oldlight = node_at_pos.light;
 node_at_pos.light = 0;
 unLightNeighbors(pos, oldlight, light_sources, modified_blocks);
 */
void Map::unLightNeighbors(v3s16 pos, f32 oldlight,
        irr::core::list<v3s16> & light_sources,
        irr::core::map<v3s16, MapBlock*> & modified_blocks) {
    v3s16 dirs[6] = { v3s16(0, 0, 1), // back
    v3s16(0, 1, 0), // top
    v3s16(1, 0, 0), // right
    v3s16(0, 0, -1), // front
    v3s16(0, -1, 0), // bottom
    v3s16(-1, 0, 0), // left
            };

    // Loop through 6 neighbors
    for (u16 i = 0; i < 6; i++) {
        // Get the position of the neighbor node
        v3s16 n2pos = pos + dirs[i];

        // Get the block where the node is located
        v3s16 blockpos = getNodeBlockPos(n2pos);
        MapBlock *block;
        try {
            block = getBlockNoCreate(blockpos);
        } catch (InvalidPositionException &e) {
            // If block is inexistent, move to next one.
            continue;
        }

        // Find if block is in cache container
        irr::core::map<v3s16, MapBlock *>::Node *cacheblocknode;
        cacheblocknode = modified_blocks.find(blockpos);

        // If the block is not found in the cache
        if (cacheblocknode == NULL) {
            /*
             Add block to cache container. It could be a 'set' but
             there is no such thing in Irrlicht.

             We can use the pointer as the value of a map just fine,
             it gets nicely cached that way, too.
             */
            modified_blocks.insert(blockpos, block);
        }

        // Calculate relative position in block
        v3s16 relpos = n2pos - blockpos * MAP_BLOCKSIZE;
        // Get node straight from the block (fast!)
        MapNode *n2 = block->getNodePtr(relpos);

        /*
         If the neighbor is dimmer than what was specified
         as oldlight (the light of the previous node)...
         */
        if (n2->light < oldlight - 0.001) {
            if (n2->transparent()) {
                light_t current_light = n2->light;
                n2->light = 0.0;
                unLightNeighbors(n2pos, current_light, light_sources,
                        modified_blocks);
            }
        } else {
            light_sources.push_back(n2pos);
        }
    }
}

/*
 Goes recursively through the neighbours of the node.

 Alters only transparent nodes.

 If the lighting of the neighbour is lower than the calculated
 lighting coming from the current node to it, the lighting of
 the neighbour is set and the same thing repeats.

 Some things are made strangely to make it as fast as possible.

 Usage:
 irr::core::map<v3s16, MapBlock*> modified_blocks;
 lightNeighbors(pos, node_at_pos.light, modified_blocks);
 */
/*void Map::lightNeighbors(v3s16 pos, f32 oldlight,
 irr::core::map<v3s16, MapBlock*> & modified_blocks)*/
void Map::lightNeighbors(v3s16 pos,
        irr::core::map<v3s16, MapBlock*> & modified_blocks) {
    v3s16 dirs[6] = { v3s16(0, 0, 1), // back
    v3s16(0, 1, 0), // top
    v3s16(1, 0, 0), // right
    v3s16(0, 0, -1), // front
    v3s16(0, -1, 0), // bottom
    v3s16(-1, 0, 0), // left
            };

    irr::core::list<v3s16> neighbour_cache;

    /*
     Initialize block cache
     (using center node as a starting position)
     */
    v3s16 blockpos_last = getNodeBlockPos(pos);
    MapBlock *block = NULL;
    try {
        block = getBlockNoCreate(blockpos_last);
    } catch (InvalidPositionException &e) {
        return;
    }

    // Calculate relative position in block
    v3s16 relpos = pos - blockpos_last * MAP_BLOCKSIZE;
    // Get node straight from the block (fast!)
    MapNode *n = block->getNodePtr(relpos);

    f32 oldlight = n->light;
    f32 newlight = oldlight * LIGHT_DIMINISH_FACTOR;

    // Loop through 6 neighbors
    for (u16 i = 0; i < 6; i++) {
        // Get the position of the neighbor node
        v3s16 n2pos = pos + dirs[i];

        // Get the block where the node is located
        v3s16 blockpos = getNodeBlockPos(n2pos);

        try {
            /*
             Only fetch a new block if the block position has changed
             */
            if (blockpos != blockpos_last) {
                block = getBlockNoCreate(blockpos);
            }
            blockpos_last = blockpos;

            // Find if block is in cache container
            irr::core::map<v3s16, MapBlock *>::Node *cacheblocknode;
            cacheblocknode = modified_blocks.find(blockpos);

            // If the block is not found in the cache
            if (cacheblocknode == NULL) {
                /*
                 Add block to cache container. It could be a 'set' but
                 there is no such thing in Irrlicht.

                 We can use the pointer as the value of a map just fine,
                 it gets nicely cached that way, too.
                 */
                modified_blocks.insert(blockpos, block);
            }

            // Calculate relative position in block
            v3s16 relpos = n2pos - blockpos * MAP_BLOCKSIZE;
            // Get node straight from the block (fast!)
            MapNode *n2 = block->getNodePtr(relpos);

            /*
             If the neighbor is dimmer than what was specified
             as oldlight (the light of the previous node)...
             */
            if (n2->light_source() > oldlight / LIGHT_DIMINISH_FACTOR + 0.01) {
                n2->light = n2->light_source();
                neighbour_cache.push_back(n2pos);
            }
            if (n2->light < newlight - 0.001) {
                if (n2->transparent()) {
                    n2->light = newlight;
                    // Cache and recurse at last step for maximum speed
                    neighbour_cache.push_back(n2pos);
                }
            }
        } catch (InvalidPositionException &e) {
            continue;
        }
    }

    irr::core::list<v3s16>::Iterator j = neighbour_cache.begin();
    for (; j != neighbour_cache.end(); j++) {
        lightNeighbors(*j, modified_blocks);
    }
}

v3s16 Map::getBrightestNeighbour(v3s16 p) {
    v3s16 dirs[6] = { v3s16(0, 0, 1), // back
    v3s16(0, 1, 0), // top
    v3s16(1, 0, 0), // right
    v3s16(0, 0, -1), // front
    v3s16(0, -1, 0), // bottom
    v3s16(-1, 0, 0), // left
            };

    f32 brightest_light = -1.0;
    v3s16 brightest_pos(0, 0, 0);

    // Loop through 6 neighbors
    for (u16 i = 0; i < 6; i++) {
        // Get the position of the neighbor node
        v3s16 n2pos = p + dirs[i];
        MapNode n2;
        try {
            n2 = getNode(n2pos);
        } catch (InvalidPositionException &e) {
            continue;
        }
        if (n2.light > brightest_light) {
            brightest_light = n2.light;
            brightest_pos = n2pos;
        }
    }
    if (brightest_light < -0.001)
        throw;
    return brightest_pos;
}

/*
 Propagates sunlight down from a node.
 Starting point gets sunlight.

 Returns the lowest y value of where the sunlight went.
 */
s16 Map::propagateSunlight(v3s16 start,
        irr::core::map<v3s16, MapBlock*> & modified_blocks) {
    s16 y = start.Y;
    for (;; y--) {
        v3s16 pos(start.X, y, start.Z);

        v3s16 blockpos = getNodeBlockPos(pos);
        MapBlock *block;
        try {
            block = getBlockNoCreate(blockpos);
        } catch (InvalidPositionException &e) {
            break;
        }

        v3s16 relpos = pos - blockpos * MAP_BLOCKSIZE;
        MapNode *n = block->getNodePtr(relpos);

        if (n->transparent()) {
            n->light = 1.0;

            modified_blocks.insert(blockpos, block);
        } else {
            break;
        }
    }
    return y + 1;
}

void Map::updateLighting(irr::core::list<MapBlock*> & a_blocks,
        irr::core::map<v3s16, MapBlock*> & modified_blocks) {
    std::cout << "Map::updateLighting(): " << a_blocks.getSize() << " blocks"
            << std::endl;

    /*status.setTodo(a_blocks.getSize());
     status.setDone(0);
     status.setText(L"Updating lighting");*/

    std::cout << "Flooding direct sunlight" << std::endl;

    // Copy list
    irr::core::list<MapBlock *> temp_blocks = a_blocks;

    /*
     Go from the highest blocks to the lowest, propagating sunlight
     through them.
     */

    while (temp_blocks.empty() == false) {
        // Get block with highest position in Y

        irr::core::list<MapBlock *>::Iterator highest_i = temp_blocks.end();
        v3s16 highest_pos(0, -32768, 0);

        irr::core::list<MapBlock *>::Iterator i;
        for (i = temp_blocks.begin(); i != temp_blocks.end(); i++) {
            MapBlock *block = *i;
            v3s16 pos = block->getPosRelative();
            if (highest_i == temp_blocks.end() || pos.Y > highest_pos.Y) {
                highest_i = i;
                highest_pos = pos;
            }
        }

        // Update sunlight in block
        MapBlock *block = *highest_i;
        block->propagateSunlight();

        /*std::cout<<"block ("<<block->getPosRelative().X<<","
         <<block->getPosRelative().Y<<","
         <<block->getPosRelative().Z<<") ";
         std::cout<<"touches_bottom="<<touches_bottom<<std::endl;*/

        // Add to modified blocks
        v3s16 blockpos = getNodeBlockPos(highest_pos);
        modified_blocks.insert(blockpos, *highest_i);

        // Remove block from list
        temp_blocks.erase(highest_i);
    }

    // Add other light sources here

    //

    std::cout << "Spreading light to remaining air" << std::endl;

    /*
     Spread light to air that hasn't got light

     TODO: This shouldn't have to be done to every block
     Though it doesn't hurt much i guess.
     */

    irr::core::list<MapBlock *>::Iterator i = a_blocks.begin();
    for (; i != a_blocks.end(); i++) {
        MapBlock *block = *i;
        v3s16 pos = block->getPosRelative();
        s16 xmin = pos.X;
        s16 zmin = pos.Z;
        s16 ymin = pos.Y;
        s16 xmax = pos.X + MAP_BLOCKSIZE - 1;
        s16 zmax = pos.Z + MAP_BLOCKSIZE - 1;
        s16 ymax = pos.Y + MAP_BLOCKSIZE - 1;
        for (s16 z = zmin; z <= zmax; z++) {
            for (s16 x = xmin; x <= xmax; x++) {
                for (s16 y = ymax; y >= ymin; y--) {
                    v3s16 pos(x, y, z);
                    MapNode n;
                    try {
                        n = getNode(pos);
                    } catch (InvalidPositionException &e) {
                        break;
                    }
                    if (n.light > NO_LIGHT_MAX) {
                        v3s16 pos(x, y, z);
                        lightNeighbors(pos, modified_blocks);
                    }
                }
            }
        }
        std::cout << "X";
        std::cout.flush();

        //status.setDone(status.getDone()+1);
    }
    std::cout << std::endl;
}

/*
 This is called after changing a node from transparent to opaque.
 The lighting value of the node should be left as-is after changing
 other values. This sets the lighting value to 0.
 */
void Map::nodeAddedUpdate(v3s16 p, f32 lightwas) {
    std::cout << "Map::nodeAddedUpdate()" << std::endl;

    irr::core::list<v3s16> light_sources;
    irr::core::map<v3s16, MapBlock*> modified_blocks;
    //MapNode n = getNode(p);

    /*
     From this node to nodes underneath:
     If lighting is sunlight (1.0), unlight neighbours and
     set lighting to 0.
     Else discontinue.
     */

    bool node_under_sunlight = true;

    v3s16 toppos = p + v3s16(0, 1, 0);

    /*
     If there is a node at top and it doesn't have sunlight,
     there has not been any sunlight going down.

     Otherwise there probably is.
     */
    try {
        MapNode topnode = getNode(toppos);

        if (topnode.light < 0.999)
            node_under_sunlight = false;
    } catch (InvalidPositionException &e) {
    }

    // Add the block of the added node to modified_blocks
    v3s16 blockpos = getNodeBlockPos(p);
    MapBlock * blockref = getBlockNoCreate(blockpos);
    MapBlock *block = blockref;
    assert(block != NULL);
    modified_blocks.insert(blockpos, block);

    // Unlight neighbours of node.
    // This means setting light of all consequent dimmer nodes
    // to 0.

    if (isValidPosition(p) == false)
        throw;

    unLightNeighbors(p, lightwas, light_sources, modified_blocks);

    MapNode n = getNode(p);
    n.light = 0;
    setNode(p, n);

    if (node_under_sunlight) {
        s16 y = p.Y - 1;
        for (;; y--) {
            std::cout << "y=" << y << std::endl;
            v3s16 n2pos(p.X, y, p.Z);

            MapNode n2;
            try {
                n2 = getNode(n2pos);
            } catch (InvalidPositionException &e) {
                break;
            }

            if (n2.light >= 0.999) {
                std::cout << "doing" << std::endl;
                unLightNeighbors(n2pos, n2.light, light_sources,
                        modified_blocks);
                n2.light = 0;
                setNode(n2pos, n2);
            } else
                break;
        }
    }

    irr::core::list<v3s16>::Iterator j = light_sources.begin();
    for (; j != light_sources.end(); j++) {
        lightNeighbors(*j, modified_blocks);
    }

    irr::core::map<v3s16, MapBlock *>::Iterator i = modified_blocks.getIterator();
    for (; i.atEnd() == false; i++) {
        i.getNode()->getValue()->updateFastFaces();
    }
}

/*
 */
void Map::removeNodeAndUpdate(v3s16 p) {
    std::cout << "Map::removeNodeAndUpdate()" << std::endl;

    irr::core::map<v3s16, MapBlock*> modified_blocks;

    bool node_under_sunlight = true;

    v3s16 toppos = p + v3s16(0, 1, 0);

    /*
     If there is a node at top and it doesn't have sunlight,
     there will be no sunlight going down.
     */
    try {
        MapNode topnode = getNode(toppos);

        if (topnode.light < 0.999)
            node_under_sunlight = false;
    } catch (InvalidPositionException &e) {
    }

    /*
     Unlight neighbors (in case the node is a light source)
     */
    irr::core::list<v3s16> light_sources;
    unLightNeighbors(p, getNode(p).light, light_sources, modified_blocks);

    /*
     Remove the node
     */
    MapNode n;
    n.d = MATERIAL_AIR;
    setNode(p, n);

    /*
     Recalculate lighting
     */
    irr::core::list<v3s16>::Iterator j = light_sources.begin();
    for (; j != light_sources.end(); j++) {
        lightNeighbors(*j, modified_blocks);
    }

    // Add the block of the removed node to modified_blocks
    v3s16 blockpos = getNodeBlockPos(p);
    MapBlock * blockref = getBlockNoCreate(blockpos);
    MapBlock *block = blockref;
    assert(block != NULL);
    modified_blocks.insert(blockpos, block);

    if (node_under_sunlight) {
        s16 ybottom = propagateSunlight(p, modified_blocks);
        /*std::cout<<"Node was under sunlight. "
         "Propagating sunlight";
         std::cout<<" -> ybottom="<<ybottom<<std::endl;*/
        s16 y = p.Y;
        for (; y >= ybottom; y--) {
            v3s16 p2(p.X, y, p.Z);
            /*std::cout<<"lighting neighbors of node ("
             <<p2.X<<","<<p2.Y<<","<<p2.Z<<")"
             <<std::endl;*/
            lightNeighbors(p2, modified_blocks);
        }
    } else {
        // Set the lighting of this node to 0
        try {
            MapNode n = getNode(p);
            n.light = 0;
            setNode(p, n);
        } catch (InvalidPositionException &e) {
            throw;
        }
    }

    // Get the brightest neighbour node and propagate light from it
    v3s16 n2p = getBrightestNeighbour(p);
    try {
        MapNode n2 = getNode(n2p);
        lightNeighbors(n2p, modified_blocks);
    } catch (InvalidPositionException &e) {
    }

    irr::core::map<v3s16, MapBlock *>::Iterator i = modified_blocks.getIterator();
    for (; i.atEnd() == false; i++) {
        i.getNode()->getValue()->updateFastFaces();
    }
}

irr::core::aabbox3d<s16> Map::getDisplayedBlockArea() {
    return irr::core::aabbox3d<s16>();   // this function only works at client end
}

void Map::renderMap(video::IVideoDriver* driver, video::SMaterial *materials) {
    ;   // this function only works at client end
}

/*
 Returns true if updated something
 */
bool Map::updateChangedVisibleArea() {
    return false;   // this function only works at client end
}

void Map::loadCreatedNodes() {
    JMutexAutoLock lock(map_load_mutex);
    std::ifstream ifs(SERVER_MAP_FILE);
    if (ifs) {
        dout_map << "Map file found" << std::endl;

        Json::CharReaderBuilder reader;
        Json::Value map;
        JSONCPP_STRING errs;
        Json::parseFromStream(reader, ifs, &map, &errs);
        for (Json::Value::const_iterator i = map.begin(); i != map.end(); i++) {
            Json::Value pos = (*i)["0"];
            v3s16 nodePos = v3s16(pos[0].asInt(), pos[1].asInt(),
                    pos[2].asInt());
            s16 d = (*i)["1"].asInt();
            m_nodes.insert(nodePos, d);
        }
    } else {
        dout_map << "Map file does not exist" << std::endl;
    }
}

MapSector * MasterMap::getSector(v2s16 p2d) {
    // Check that it doesn't exist already
    try {
        MapSector * sref = getSectorNoGenerate(p2d);
        return sref;
    } catch (InvalidPositionException &e) {
    }

    //=====Not using HeightmapBlockGenerator here=====
    BlockGenerator* gen;
    gen = new BlockGenerator();
    MapSector *sector = new MapSector(this, p2d, gen);

    m_sectors.insert(p2d, sector);
    return sector;
}
