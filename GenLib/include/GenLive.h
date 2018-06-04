#pragma once
#include "GenType.h"
#include "GenState.h"

#define LGMAXNODES 16
#define LGALLNODES 0x0000FFFF
#define LGDEFAULTPORT 6253

#define LGNODETOMASK(a) (1 << (a)) // Single node ID to mask

typedef genu16 lgnodemask;

struct NetNode; class ILiveGen; struct LgAddress;

ILiveGen* CreateLiveGen();
void DestroyLiveGen(ILiveGen* live);

class ILiveGen {
	public:
		virtual void Update() = 0;

		virtual bool Connect(const LgAddress& addr) = 0;
		virtual void DisconnectNodes(lgnodemask nodeMask = LGALLNODES) = 0;

		virtual void SendState(const GenState& stateIn, lgnodemask nodeMask = LGALLNODES) = 0;
		virtual void SendState(const GenSubstate& stateDataIn, lgnodemask nodeMask = LGALLNODES) = 0;
		virtual void SendStateDirect(genid id, GenStateType type, const GenValueSet& elements, lgnodemask nodeMask = LGALLNODES) = 0;
		virtual bool GetState(GenState* stateOut, lgnodemask* nodeMaskOut, lgnodemask nodeMaskIn = LGALLNODES) = 0;
		
		virtual genu16 GetLocalPort() const = 0;
		virtual int GetNumConnectedNodes() const = 0;
		
		virtual int CountPacketsIn(lgnodemask nodeMask) const = 0;
		virtual int CountPacketsOut(lgnodemask nodeMask) const = 0;
		virtual int CountPacketsInPending(lgnodemask nodeMask) const = 0;
		virtual int CountPacketsOutPending(lgnodemask nodeMask) const = 0;
		virtual int CountBytesIn(lgnodemask nodeMask) const = 0;
		virtual int CountBytesOut(lgnodemask nodeMask) const = 0;
		virtual int CountBytesInPending(lgnodemask nodeMask) const = 0;
		virtual int CountBytesOutPending(lgnodemask nodeMask) const = 0;
		virtual int CountBytesPerSecondIn(lgnodemask nodeMask) const = 0;
		virtual int CountBytesPerSecondOut(lgnodemask nodeMask) const = 0;
};

struct LgAddress {
	inline LgAddress() = default;
	inline LgAddress(genu8 ip1, genu8 ip2, genu8 ip3, genu8 ip4, genu16 _port) : addr32(ip4 << 24 | ip3 << 16 | ip2 << 8 | ip1), port(_port) {};
	inline LgAddress(const char* address) : addr32(0xFFFFFFFF), port(0) {SetIp(address);};
	inline LgAddress(const char* address, genu16 port_) : addr32(0xFFFFFFFF), port(0) {if (SetIp(address)) port = port_;};

	// functions
	bool SetIp(const char* address); // May also set port with a colon :
	void GetIp(      char* address, bool includePort) const;

	void CloneFromNetNode(const NetNode& netNodeIn);
	void CloneToNetNode(NetNode* netNodeOut) const;

	inline bool operator==(const LgAddress& other) const {
		return (addr32 == other.addr32 && port == other.port);
	}

	// vars
	union {
		genu32 addr32; // IPv4 address in uint32 format
		genu8 octets[4]; // IPv4 address in byte format
	};
	genu16 port; // Port in local (normal) byte order
};