#include "Types.h"
#include "SpyroData.h"
#include "Vram.h"

#define NETSTATE_NONE   0
#define NETSTATE_SERVER 1
#define NETSTATE_CLIENT 2

enum netmsg_t
{
	NM_JOIN = 0,
	NM_PLAYERUPDATE = 1,
	NM_SAVESTATESTATUS = 2,
	NM_SAVESTATEDATA = 3,
	NM_ATTACKFLAGS = 4, 
	NM_MOBYSYNC = 5
};

struct Address
{
	uint8 ip[4];
	uint16 port;
};

#pragma pack(push,1)
struct Player {
	Address addr;

	Spyro spyro;

	uint32 powers;

	uint32 lastMessageTime;
};

struct Savestate {
	unsigned char memory[1024 * 1024 * 2];
	GPUSnapshot snapshot;
};

struct NmSavestateData {
	uint8 msgId;
	
	int16 dataId;
	uint8 data[8192];
};

struct NmSavestateStatus {
	uint8 msgId;

	uint32 dataFlags[13];
};

struct NmPlayerUpdate {
	uint8 msgId;

	Spyro spyroData;

	uint32 powers;
	uint8 prevAnimNumFrames, nextAnimNumFrames, prevHeadAnimNumFrames, nextHeadAnimNumFrames;
};

struct NmAttackFlag {
	uint8 msgId;
	int16 objId;
	uint32 flags;
};

struct NmMobySync {
	uint8 msgId;
	uint16 mobyId;

	Moby moby;
};
#pragma pack(pop)

extern int8 netstate;

extern int netPort;

extern Player players[10];
extern int numPlayers;
extern int localPlayerId;

extern bool isNetworkInitialised;

const int netPortMin = 18541;
const int netPortMax = 18545;

void Host();
void Join();

void NetworkLoop();
void ReceiveMessages();

int SendTo(const void* buffer, int buffer_length, Address* addr);
int RecvFrom(void* buffer, int buffer_length, Address* addr);
