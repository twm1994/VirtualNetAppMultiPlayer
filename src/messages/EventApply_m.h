//
// Generated file, do not edit! Created by nedtool 5.5 from messages/EventApply.msg.
//

#ifndef __EVENTAPPLY_M_H
#define __EVENTAPPLY_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0505
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



/**
 * Class generated from <tt>messages/EventApply.msg:19</tt> by nedtool.
 * <pre>
 * //
 * // TODO generated message class
 * //
 * packet EventApply
 * {
 *     int index;
 *     string event;
 * 	// corresponding round in Qd
 *     int round;
 *     // corresponding clientId of the round in Qd
 *     unsigned long clientId;
 *     // event sequence number of the client and the round in Qd 
 *     int sequence;
 * }
 * </pre>
 */
class EventApply : public ::omnetpp::cPacket
{
  protected:
    int index;
    ::omnetpp::opp_string event;
    int round;
    unsigned long clientId;
    int sequence;

  private:
    void copy(const EventApply& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const EventApply&);

  public:
    EventApply(const char *name=nullptr, short kind=0);
    EventApply(const EventApply& other);
    virtual ~EventApply();
    EventApply& operator=(const EventApply& other);
    virtual EventApply *dup() const override {return new EventApply(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual int getIndex() const;
    virtual void setIndex(int index);
    virtual const char * getEvent() const;
    virtual void setEvent(const char * event);
    virtual int getRound() const;
    virtual void setRound(int round);
    virtual unsigned long getClientId() const;
    virtual void setClientId(unsigned long clientId);
    virtual int getSequence() const;
    virtual void setSequence(int sequence);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const EventApply& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, EventApply& obj) {obj.parsimUnpack(b);}


#endif // ifndef __EVENTAPPLY_M_H

