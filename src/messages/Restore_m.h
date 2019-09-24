//
// Generated file, do not edit! Created by nedtool 5.5 from messages/Restore.msg.
//

#ifndef __RESTORE_M_H
#define __RESTORE_M_H

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
 * Class generated from <tt>messages/Restore.msg:24</tt> by nedtool.
 * <pre>
 * packet Restore
 * {
 *     string displayString = DISPLAY_STR_MSG_GROUP_GR;
 * 
 *     string senderName;
 *     int epoch;
 *     bool reconfig;
 *     int groupId;
 *     string groupMembers;
 *     string Qd;
 *     string Qc;
 *     string init;
 * }
 * </pre>
 */
class Restore : public ::omnetpp::cPacket
{
  protected:
    ::omnetpp::opp_string displayString;
    ::omnetpp::opp_string senderName;
    int epoch;
    bool reconfig;
    int groupId;
    ::omnetpp::opp_string groupMembers;
    ::omnetpp::opp_string Qd;
    ::omnetpp::opp_string Qc;
    ::omnetpp::opp_string init;

  private:
    void copy(const Restore& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Restore&);

  public:
    Restore(const char *name=nullptr, short kind=0);
    Restore(const Restore& other);
    virtual ~Restore();
    Restore& operator=(const Restore& other);
    virtual Restore *dup() const override {return new Restore(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual const char * getDisplayString() const;
    virtual void setDisplayString(const char * displayString);
    virtual const char * getSenderName() const;
    virtual void setSenderName(const char * senderName);
    virtual int getEpoch() const;
    virtual void setEpoch(int epoch);
    virtual bool getReconfig() const;
    virtual void setReconfig(bool reconfig);
    virtual int getGroupId() const;
    virtual void setGroupId(int groupId);
    virtual const char * getGroupMembers() const;
    virtual void setGroupMembers(const char * groupMembers);
    virtual const char * getQd() const;
    virtual void setQd(const char * Qd);
    virtual const char * getQc() const;
    virtual void setQc(const char * Qc);
    virtual const char * getInit() const;
    virtual void setInit(const char * init);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const Restore& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, Restore& obj) {obj.parsimUnpack(b);}


#endif // ifndef __RESTORE_M_H

