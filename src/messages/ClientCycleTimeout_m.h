//
// Generated file, do not edit! Created by nedtool 5.5 from messages/ClientCycleTimeout.msg.
//

#ifndef __CLIENTCYCLETIMEOUT_M_H
#define __CLIENTCYCLETIMEOUT_M_H

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
 * Class generated from <tt>messages/ClientCycleTimeout.msg:19</tt> by nedtool.
 * <pre>
 * //
 * // TODO generated message class
 * //
 * packet ClientCycleTimeout
 * {
 *     string LCName;
 * }
 * </pre>
 */
class ClientCycleTimeout : public ::omnetpp::cPacket
{
  protected:
    ::omnetpp::opp_string LCName;

  private:
    void copy(const ClientCycleTimeout& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const ClientCycleTimeout&);

  public:
    ClientCycleTimeout(const char *name=nullptr, short kind=0);
    ClientCycleTimeout(const ClientCycleTimeout& other);
    virtual ~ClientCycleTimeout();
    ClientCycleTimeout& operator=(const ClientCycleTimeout& other);
    virtual ClientCycleTimeout *dup() const override {return new ClientCycleTimeout(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual const char * getLCName() const;
    virtual void setLCName(const char * LCName);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const ClientCycleTimeout& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, ClientCycleTimeout& obj) {obj.parsimUnpack(b);}


#endif // ifndef __CLIENTCYCLETIMEOUT_M_H

