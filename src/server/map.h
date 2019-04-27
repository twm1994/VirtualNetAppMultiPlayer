/*
 (c) 2010 Perttu Ahola <celeron55@gmail.com>
 */

#ifndef MAP_HEADER
#define MAP_HEADER

#include <iostream>
#include <malloc.h>
#include "../thread/jmutexautolock.h"
#include "../thread/jthread.h"

#ifdef _WIN32
#include <windows.h>
#define sleep_s(x) Sleep((x*1000))
#else
#include <unistd.h>
#define sleep_s(x) sleep(x)
#endif

#include "common_irrlicht.h"
#include "mapnode.h"
#include "mapblock.h"
#include "mapsector.h"

using namespace jthread;

/*

 TODO: Automatically unload blocks from memory and save on disk
 when they are far away
 */

//void limitBox(irr::core::aabbox3d<s16> & box, irr::core::aabbox3d<s16> & limits);
class Map;

class MapUpdateThread: public JThread {
    bool run;
    JMutex run_mutex;

    Map *map;

public:

    MapUpdateThread(Map *the_map) :
            JThread(), run(true), map(the_map) {
        ;   // this function only works at client end
    }

    // update map content per 500ms
    void * Thread();

    bool getRun() {
        return false;   // this function only works at client end
    }
    void setRun(bool a_run) {
        ;   // this function only works at client end
    }
};

class Map: public NodeContainer {
public:
    /*
     TODO: Dynamic block loading
     Add a filename to the constructor, in which the map will be
     automatically stored - or something.
     */

protected:

    irr::core::map<v2s16, MapSector*> m_sectors;
    JMutex m_getsector_mutex;
    JMutex m_gensector_mutex;

    v3f camera_position;
    v3f camera_direction;
    JMutex camera_mutex;

    MapUpdateThread updater;

    // Be sure to set this to NULL when the cached sector is deleted
    MapSector *m_sector_cache;
    v2s16 m_sector_cache_p;

    irr::core::map<v3s16, s16> m_nodes;
    // file read is exclusive
    JMutex map_load_mutex;

public:
    static s16 MAP_LENGTH;
    static s16 MAP_WIDTH;
    static s16 MAP_HEIGHT;
    static s16 MAP_BOTTOM;
    static const char* SERVER_MAP_FILE;

    v3s16 drawoffset;

    //LoadStatus status;

    Map();
    ~Map();

    void load() {
        loadCreatedNodes();
    }
    void loadCreatedNodes();

    virtual MapSector * getSector(v2s16 pos);
    virtual MapBlock * getBlock(v3s16 pos);

    void updateCamera(v3f pos, v3f dir) {
        ;   // this function only works at client end
    }

    void StartUpdater() {
        ;   // this function only works at client end
    }

    void StopUpdater() {
        ;   // this function only works at client end
    }

    bool UpdaterIsRunning() {
        return false;   // this function only works at client end
    }

    /*
     Returns integer position of the node in given
     floating point position.
     */
    static v3s16 floatToInt(v3f p) {
        v3s16 p2((p.X + (p.X > 0 ? BS / 2 : -BS / 2)) / BS,
                (p.Y + (p.Y > 0 ? BS / 2 : -BS / 2)) / BS,
                (p.Z + (p.Z > 0 ? BS / 2 : -BS / 2)) / BS);
        return p2;
    }

    static v3f intToFloat(v3s16 p) {
        v3f p2(p.X * BS, p.Y * BS, p.Z * BS);
        return p2;
    }

    static irr::core::aabbox3d<f32> getNodeBox(v3s16 p) {
        return irr::core::aabbox3d<f32>((float) p.X * BS - 0.5 * BS,
                (float) p.Y * BS - 0.5 * BS, (float) p.Z * BS - 0.5 * BS,
                (float) p.X * BS + 0.5 * BS, (float) p.Y * BS + 0.5 * BS,
                (float) p.Z * BS + 0.5 * BS);
    }

    //bool sectorExists(v2s16 p);
    MapSector * getSectorNoGenerate(v2s16 p2d);

    MapBlock * getBlockNoCreate(v3s16 p);

    bool isValidPosition(v3s16 p) {
        v3s16 blockpos = getNodeBlockPos(p);
        MapBlock * blockref = getBlockNoCreate(blockpos);
        if (blockref == NULL) {
            /*std::cout<<"Map::isValidPosition("<<p.X<<","<<p.Y<<","<<p.Z
             <<"): is_valid=0 (block inexistent)";*/
            return false;
        }
        v3s16 relpos = p - blockpos * MAP_BLOCKSIZE;
        bool is_valid = blockref->isValidPosition(relpos);
        /*std::cout<<"Map::isValidPosition("<<p.X<<","<<p.Y<<","<<p.Z
         <<"): is_valid="<<is_valid<<" (block reported)";*/
        return is_valid;
    }

    /*
     Returns the position of the block where the node is located
     */
    v3s16 getNodeBlockPos(v3s16 p) {
        return v3s16((p.X >= 0 ? p.X : p.X - MAP_BLOCKSIZE + 1) / MAP_BLOCKSIZE,
                (p.Y >= 0 ? p.Y : p.Y - MAP_BLOCKSIZE + 1) / MAP_BLOCKSIZE,
                (p.Z >= 0 ? p.Z : p.Z - MAP_BLOCKSIZE + 1) / MAP_BLOCKSIZE);
    }

    v2s16 getNodeSectorPos(v2s16 p) {
        return v2s16((p.X >= 0 ? p.X : p.X - MAP_BLOCKSIZE + 1) / MAP_BLOCKSIZE,
                (p.Y >= 0 ? p.Y : p.Y - MAP_BLOCKSIZE + 1) / MAP_BLOCKSIZE);
    }

    s16 getNodeBlockY(s16 y) {
        return (y >= 0 ? y : y - MAP_BLOCKSIZE + 1) / MAP_BLOCKSIZE;
    }

    MapBlock * getNodeBlock(v3s16 p) {
        v3s16 blockpos = getNodeBlockPos(p);
        return getBlock(blockpos);
    }

    MapNode getNode(v3s16 p) {
        v3s16 blockpos = getNodeBlockPos(p);
        MapBlock * blockref = getBlockNoCreate(blockpos);
        if (blockref == NULL)
            throw InvalidPositionException();
        v3s16 relpos = p - blockpos * MAP_BLOCKSIZE;

        return blockref->getNode(relpos);
    }

    MapNode getNode(s16 x, s16 y, s16 z) {
        return getNode(v3s16(x, y, z));
    }

    void setNode(v3s16 p, MapNode & n) {
        v3s16 blockpos = getNodeBlockPos(p);
        MapBlock * blockref = getBlockNoCreate(blockpos);
        if (blockref == NULL)
            throw InvalidPositionException();
        v3s16 relpos = p - blockpos * MAP_BLOCKSIZE;
        blockref->setNode(relpos, n);
        //=====For node storage=====
        if (n.d < MATERIAL_AIR)
            m_nodes.insert(p, s16(n.d));
        else {
            irr::core::map<v3s16, s16>::Node *n = m_nodes.find(p);
            if (n != NULL) {
                m_nodes.remove(n);
            }
        }
    }

    void setNode(s16 x, s16 y, s16 z, MapNode & n) {
        setNode(v3s16(x, y, z), n);
    }

    MapNode getNode(v3f p) {
        return getNode(floatToInt(p));
    }

    void unLightNeighbors(v3s16 pos, f32 oldlight,
            irr::core::list<v3s16> & light_sources,
            irr::core::map<v3s16, MapBlock*> & modified_blocks);

    /*void lightNeighbors(v3s16 pos, f32 oldlight,
     irr::core::map<v3s16, MapBlock*> & modified_blocks);*/
    void lightNeighbors(v3s16 pos,
            irr::core::map<v3s16, MapBlock*> & modified_blocks);

    v3s16 getBrightestNeighbour(v3s16 p);

    s16 propagateSunlight(v3s16 start,
            irr::core::map<v3s16, MapBlock*> & modified_blocks);

    void updateLighting(irr::core::list<MapBlock*> & a_blocks,
            irr::core::map<v3s16, MapBlock*> & modified_blocks);

    void nodeAddedUpdate(v3s16 p, f32 light);
    void removeNodeAndUpdate(v3s16 p);

    irr::core::aabbox3d<s16> getDisplayedBlockArea();

    /*void generateBlock(MapBlock *block);
     void generateMaster();*/

    bool updateChangedVisibleArea();

    void renderMap(video::IVideoDriver* driver, video::SMaterial *materials);

};

class MasterMap: public Map {
public:
    MapSector * getSector(v2s16 p);
};

#endif
