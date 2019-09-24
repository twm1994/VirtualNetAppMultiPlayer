//
// Generated file, do not edit! Created by nedtool 5.5 from messages/CCAck.msg.
//

#ifndef __CCACK_M_H
#define __CCACK_M_H

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
 * Class generated from <tt>messages/CCAck.msg:20</tt> by nedtool.
 * <pre>
 * packet CCAck
 * {
 *     string displayString = DISPLAY_STR_MSG_GROUP_CC;
 * 
 *     string senderName;
 * }
 * </pre>
 */
class CCAck : public ::omnetpp::cPacket
{
  protected:
    ::omnetpp::opp_string displayString;
    ::omnetpp::opp_string senderName;

  private:
    void copy(const CCAck& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const CCAck&);

  public:
    CCAck(const char *name=nullptr, short kind=0);
    CCAck(const CCAck& other);
    virtual ~CCAck();
    CCAck& operator=(const CCAck& other);
    virtual CCAck *dup() const override {return new CCAck(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual const char * getDisplayString() const;
    virtual void setDisplayString(const char * displayString);
    virtual const char * getSenderName() const;
    virtual void setSenderName(const char * senderName);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const CCAck& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, CCAck& obj) {obj.parsimUnpack(b);}


#endif // ifndef __CCACK_M_H

