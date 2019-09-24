//
// Generated file, do not edit! Created by nedtool 5.5 from messages/RemoveNeighbor.msg.
//

#ifndef __REMOVENEIGHBOR_M_H
#define __REMOVENEIGHBOR_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0505
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



// cplusplus {{
#include "../common/Constants.h"
// }}

/**
 * Class generated from <tt>messages/RemoveNeighbor.msg:20</tt> by nedtool.
 * <pre>
 * packet RemoveNeighbor
 * {
 *     int seq;
 *     string neighbor;
 *     string hostAddrs;
 * }
 * </pre>
 */
class RemoveNeighbor : public ::omnetpp::cPacket
{
  protected:
    int seq;
    ::omnetpp::opp_string neighbor;
    ::omnetpp::opp_string hostAddrs;

  private:
    void copy(const RemoveNeighbor& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const RemoveNeighbor&);

  public:
    RemoveNeighbor(const char *name=nullptr, short kind=0);
    RemoveNeighbor(const RemoveNeighbor& other);
    virtual ~RemoveNeighbor();
    RemoveNeighbor& operator=(const RemoveNeighbor& other);
    virtual RemoveNeighbor *dup() const override {return new RemoveNeighbor(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual int getSeq() const;
    virtual void setSeq(int seq);
    virtual const char * getNeighbor() const;
    virtual void setNeighbor(const char * neighbor);
    virtual const char * getHostAddrs() const;
    virtual void setHostAddrs(const char * hostAddrs);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const RemoveNeighbor& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, RemoveNeighbor& obj) {obj.parsimUnpack(b);}


#endif // ifndef __REMOVENEIGHBOR_M_H

