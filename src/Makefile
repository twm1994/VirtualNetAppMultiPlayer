#
# OMNeT++/OMNEST Makefile for VirtualNetAppMultiPlayer
#
# This file was generated with the command:
#  opp_makemake -f --deep -IC:/Workspaces/OMNeT++/omnetpp-5.4/include/omnetpp -IC:/Workspaces/boost_1_66_0 -IC:/Workspaces/irrlicht-1.8.4/include -lgdi32 -lWS2_32
#

# Name of target to be created (-o option)
TARGET = VirtualNetAppMultiPlayer$(D)$(EXE_SUFFIX)
TARGET_DIR = .

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(TKENV_LIBS) $(QTENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(TKENV_LIBS)
#USERIF_LIBS = $(QTENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH = -IC:/Workspaces/OMNeT++/omnetpp-5.4/include/omnetpp -IC:/Workspaces/boost_1_66_0 -IC:/Workspaces/irrlicht-1.8.4/include

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS =  -lgdi32 -lWS2_32

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = src
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc, .msg and .sm files
OBJS = \
    $O/app/ClientApp.o \
    $O/app/NodeApp.o \
    $O/common/Constants.o \
    $O/common/HostBase.o \
    $O/common/PearsonHash.o \
    $O/common/sdbm.o \
    $O/common/Util.o \
    $O/crypto/sha1.o \
    $O/ctrl/ChordCtrl.o \
    $O/ctrl/ClientCtrl.o \
    $O/ctrl/NodeCtrl.o \
    $O/ctrl/Rendezvous.o \
    $O/global/ChurnGenerator.o \
    $O/global/Coordinator.o \
    $O/global/GlobalNodeList.o \
    $O/global/GlobalParameters.o \
    $O/global/GlobalStatistics.o \
    $O/global/ManualChurnGenerator.o \
    $O/global/UnderlayConfigurator.o \
    $O/global/UniformChurnGenerator.o \
    $O/global/WeibullChurnGenerator.o \
    $O/jsoncpp/jsoncpp.o \
    $O/objects/ChordInfo.o \
    $O/objects/ClientInfo.o \
    $O/objects/HostInfo.o \
    $O/objects/IPAddress.o \
    $O/objects/IPv4InterfaceData.o \
    $O/objects/IPvXAddress.o \
    $O/objects/PeerInfo.o \
    $O/objects/PeerStorage.o \
    $O/objects/PlayerState.o \
    $O/objects/SimpleInfo.o \
    $O/objects/SimpleNodeEntry.o \
    $O/objects/TransportAddress.o \
    $O/others/BasicReplicaNumPolicy.o \
    $O/others/Clock.o \
    $O/others/DynamicReplicaNumPolicy.o \
    $O/others/InterfaceEntry.o \
    $O/others/InterfaceTable.o \
    $O/others/ReplicaNumPolicy.o \
    $O/server/connection.o \
    $O/server/environment.o \
    $O/server/map.o \
    $O/server/mapblock.o \
    $O/server/mapsector.o \
    $O/server/npc.o \
    $O/server/player.o \
    $O/server/socket.o \
    $O/server/utility.o \
    $O/thread/jmutex.o \
    $O/thread/jthread.o \
    $O/transport/NetworkAddress.o \
    $O/transport/ReliableUDP.o \
    $O/transport/SimpleUDP.o \
    $O/messages/AddNeighbor_m.o \
    $O/messages/AppData_m.o \
    $O/messages/CCAck_m.o \
    $O/messages/CCEnd_m.o \
    $O/messages/CCNAck_m.o \
    $O/messages/CCQuery_m.o \
    $O/messages/CCQueryReply_m.o \
    $O/messages/ChordMessage_m.o \
    $O/messages/ClientCycleTimeout_m.o \
    $O/messages/ClientData_m.o \
    $O/messages/ConfigUpdate_m.o \
    $O/messages/Connect_m.o \
    $O/messages/ConnectReply_m.o \
    $O/messages/ConnectTimeout_m.o \
    $O/messages/ConsensusNAck_m.o \
    $O/messages/ConsensusQuery_m.o \
    $O/messages/CycleEvent_m.o \
    $O/messages/Decision_m.o \
    $O/messages/Event_m.o \
    $O/messages/EventApply_m.o \
    $O/messages/Failure_m.o \
    $O/messages/Final_m.o \
    $O/messages/Gossip_m.o \
    $O/messages/GREnd_m.o \
    $O/messages/GRNAck_m.o \
    $O/messages/GRQuery_m.o \
    $O/messages/GRQueryReply_m.o \
    $O/messages/GRSilent_m.o \
    $O/messages/HandShake_m.o \
    $O/messages/HandShakeReject_m.o \
    $O/messages/HandShakeTimeout_m.o \
    $O/messages/HBProbe_m.o \
    $O/messages/HBResponse_m.o \
    $O/messages/HBTimeout_m.o \
    $O/messages/InitApp_m.o \
    $O/messages/InitJoin_m.o \
    $O/messages/Join_m.o \
    $O/messages/Proposal_m.o \
    $O/messages/RemoveNeighbor_m.o \
    $O/messages/Restore_m.o \
    $O/messages/RoundQuery_m.o \
    $O/messages/RoundQueryReply_m.o \
    $O/messages/Termination_m.o \
    $O/messages/Timeout_m.o \
    $O/messages/UDPControlInfo_m.o \
    $O/messages/Update_m.o \
    $O/messages/UpdateApply_m.o

# Message files
MSGFILES = \
    messages/AddNeighbor.msg \
    messages/AppData.msg \
    messages/CCAck.msg \
    messages/CCEnd.msg \
    messages/CCNAck.msg \
    messages/CCQuery.msg \
    messages/CCQueryReply.msg \
    messages/ChordMessage.msg \
    messages/ClientCycleTimeout.msg \
    messages/ClientData.msg \
    messages/ConfigUpdate.msg \
    messages/Connect.msg \
    messages/ConnectReply.msg \
    messages/ConnectTimeout.msg \
    messages/ConsensusNAck.msg \
    messages/ConsensusQuery.msg \
    messages/CycleEvent.msg \
    messages/Decision.msg \
    messages/Event.msg \
    messages/EventApply.msg \
    messages/Failure.msg \
    messages/Final.msg \
    messages/Gossip.msg \
    messages/GREnd.msg \
    messages/GRNAck.msg \
    messages/GRQuery.msg \
    messages/GRQueryReply.msg \
    messages/GRSilent.msg \
    messages/HandShake.msg \
    messages/HandShakeReject.msg \
    messages/HandShakeTimeout.msg \
    messages/HBProbe.msg \
    messages/HBResponse.msg \
    messages/HBTimeout.msg \
    messages/InitApp.msg \
    messages/InitJoin.msg \
    messages/Join.msg \
    messages/Proposal.msg \
    messages/RemoveNeighbor.msg \
    messages/Restore.msg \
    messages/RoundQuery.msg \
    messages/RoundQueryReply.msg \
    messages/Termination.msg \
    messages/Timeout.msg \
    messages/UDPControlInfo.msg \
    messages/Update.msg \
    messages/UpdateApply.msg

# SM files
SMFILES =

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIBS = $(OPPMAIN_LIB) $(USERIF_LIBS) $(KERNEL_LIBS) $(SYS_LIBS)

COPTS = $(CFLAGS) $(IMPORT_DEFINES)  $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)
SMCOPTS =

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE and have object
# files depend on it (except when "make depend" was called)
COPTS_FILE = $O/.last-copts
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
$(shell $(MKPATH) "$O" && echo "$(COPTS)" >$(COPTS_FILE))
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# <<<
#------------------------------------------------------------------------------

# Main target
all: $(TARGET_DIR)/$(TARGET)

$(TARGET_DIR)/% :: $O/%
	@mkdir -p $(TARGET_DIR)
	$(Q)$(LN) $< $@
ifeq ($(TOOLCHAIN_NAME),clangc2)
	$(Q)-$(LN) $(<:%.dll=%.lib) $(@:%.dll=%.lib)
endif

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile $(CONFIGFILE)
	@$(MKPATH) $O
	@echo Creating executable: $@
	$(Q)echo >.tmp$$$$ $(OBJS) $(EXTRA_OBJS) && $(AR) .tmplib$$$$ @.tmp$$$$ && $(CXX) -o $O/$(TARGET) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) .tmplib$$$$ $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS) $(LDFLAGS) && rm .tmp$$$$ && rm .tmplib$$$$

.PHONY: all clean cleanall depend msgheaders smheaders

.SUFFIXES: .cc

$O/%.o: %.cc $(COPTS_FILE) | msgheaders smheaders
	@$(MKPATH) $(dir $@)
	$(qecho) "$<"
	$(Q)$(CXX) -c $(CXXFLAGS) $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(qecho) MSGC: $<
	$(Q)$(MSGC) -s _m.cc -MD -MP -MF $O/$(basename $@).d $(MSGCOPTS) $?

%_sm.cc %_sm.h: %.sm
	$(qecho) SMC: $<
	$(Q)$(SMC) -c++ -suffix cc $(SMCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

smheaders: $(SMFILES:.sm=_sm.h)

clean:
	$(qecho) Cleaning $(TARGET)
	$(Q)-rm -rf $O
	$(Q)-rm -f $(TARGET_DIR)/$(TARGET)
	$(Q)-rm -f $(TARGET_DIR)/$(TARGET:%.dll=%.lib)
	$(Q)-rm -f $(call opp_rwildcard, . , *_m.cc *_m.h *_sm.cc *_sm.h)

cleanall:
	$(Q)$(MAKE) -s clean MODE=release
	$(Q)$(MAKE) -s clean MODE=debug
	$(Q)-rm -rf $(PROJECT_OUTPUT_DIR)

# include all dependencies
-include $(OBJS:%.o=%.d)
