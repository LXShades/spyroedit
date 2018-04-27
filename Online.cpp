#include <winsock2.h>

#include "Window.h"
#include "Online.h"
#include "SpyroData.h"
#include "Main.h"

#include <cstdio> // Debug stuff

#define MAXPLAYERS 10

int8 netstate = NETSTATE_NONE;

bool isNetworkInitialised = false;

SOCKET netSocket;
int netPort;

Player players[MAXPLAYERS];
int numPlayers = 1;
int localPlayerId = 0;

const int netPortMin = 18541;
const int netPortMax = 18545;

const int memdumpChunkSize = (0x00200000 / 128);

void StartupNetwork() {
	// Initialise WinSock
	WSADATA dummy;
	int ret = WSAStartup(MAKEWORD(2, 2), &dummy);

	if (ret != 0) {
		MessageBox(NULL, "Error starting up WinSock.", "", MB_OK);
		return;
	}

	// Create the socket
	netSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (netSocket == INVALID_SOCKET) {
		MessageBox(NULL, "Error creating socket.", "", MB_OK);
		return;
	}

	// Try to bind a UDP port between 2012 and 2015
	sockaddr_in localAddress = {0};
	bool hasConnected = false;

	localAddress.sin_family = AF_INET;

	for (int port = netPortMin; port <= netPortMax; ++port) {
		localAddress.sin_port = htons(port);

		// Try to bind
		if (bind(netSocket, (const sockaddr*) &localAddress, sizeof (localAddress)) != SOCKET_ERROR) {
			netPort = port;
			hasConnected = true;
			break;
		}
	}

	if (!hasConnected) {
		MessageBox(NULL, "Error binding socket to any port 2012 - 2015.", "", MB_OK);
		return;
	}

	// We're online
	isNetworkInitialised = true;
}

void NetworkLoop() {
	if (!isNetworkInitialised || !netstate)
		return;

	// Send self player update to [all players]
	NmPlayerUpdate updateMsg;
	updateMsg.msgId = NM_PLAYERUPDATE;
	updateMsg.powers = powers;
	memcpy(&updateMsg.spyroData, spyro, sizeof (Spyro));

	// Get animation info (frame rate conversion)
	if (gameState == GAMESTATE_INLEVEL && mobyModels && mobyModels[0].address) {
		uint8 anims[4] = {spyro->main_anim.prevanim, spyro->main_anim.nextanim, spyro->head_anim.prevanim, spyro->head_anim.nextanim};
		uint8* numFrames[4] = {&updateMsg.prevAnimNumFrames, &updateMsg.nextAnimNumFrames, &updateMsg.prevHeadAnimNumFrames, &updateMsg.nextHeadAnimNumFrames};

		for (int i = 0; i < 4; i++) {
			if (mobyModels[0]->anims[anims[i]].address)
				*numFrames[i] = mobyModels[0]->anims[anims[i]]->numFrames;
		}
	} else {
		updateMsg.nextAnimNumFrames = updateMsg.prevAnimNumFrames = updateMsg.nextHeadAnimNumFrames = updateMsg.prevHeadAnimNumFrames = 0;
	}

	// Then send to all connected players
	for (int i = 0; i < numPlayers; ++i) {
		if (i != localPlayerId)
			SendTo(&updateMsg, sizeof (NmPlayerUpdate), &players[i].addr);
	}

	// Do object check (flags)
	static int lastattackflags[1000];
	for (int i = 0; i < 1000; ++i) {
		if (mobys[i].state == -1)
			break;

		if (mobys[i].attack_flags != lastattackflags[i]) {
			if (!mobys) {
				MessageBox(NULL, "No mobys!", "", MB_OK);
				break;
			}

			// Send an attack flag message
			NmAttackFlag msg;

			msg.msgId = NM_ATTACKFLAGS;
			msg.objId = i;
			msg.flags = mobys[i].attack_flags;

			for (int j = 0; j < numPlayers; ++j) {
				if (j != localPlayerId)
					SendTo(&msg, sizeof (NmAttackFlag), &players[j].addr);
			}

			lastattackflags[i] = mobys[i].attack_flags;
		}
	}

	ReceiveMessages();
}

void ReceiveMessages() {
	Address addr;
	char buffer[512];
	int size;

	while ((size = RecvFrom(buffer, 512, &addr)) > 0) {
		int player_id;
		int message_id = buffer[0];
		static int send_piece = 0;

		// Find who sent the message
		for (player_id = 0; player_id < numPlayers; ++player_id) {
			if (!memcmp(&addr, &players[player_id].addr, sizeof (Address)))
				break;
		}

		if (player_id == numPlayers && message_id != NM_JOIN) // You haven't been accepted yet!
			continue;

		uint32* uintmem = (uint32*) memory;
		static GPUSnapshot tempGpuSnapshot;
		static Savestate* playerSavestates[MAXPLAYERS];

		switch (message_id) {
			// Player is trying to join
			case NM_JOIN: {
				if (netstate != NETSTATE_SERVER)
					break; // lol I'm not the server dude

				if (numPlayers >= 10)
					break; // This town ain't big enough for 10 of us. Seriously dude.

				if (strncmp(buffer, "\x00YODUDE", 8))
					break; // You're outta our league, man.

				bool alreadyHere = false;
				for (int i = 0; i < numPlayers; i++) {
					if (!memcmp(&players[i].addr, &addr, sizeof (Address)))
						alreadyHere = true;
				}

				if (alreadyHere)
					continue; // you're already here

				// Send the accept message
				char send[8] = "\x00HEYBRO";

				SendTo(send, 8, &addr);

				players[numPlayers].addr = addr;
				players[numPlayers].spyro = spyro ? *spyro : Spyro{0};
				++numPlayers;

				if (playerSavestates[player_id] == NULL)
					playerSavestates[player_id] = (Savestate*) malloc(sizeof (Savestate));

				// Setup the savestate to presumably send to the player...? What did this even do
				GetSnapshot(&tempGpuSnapshot);

				memcpy(playerSavestates[player_id]->memory, memory, 0x00200000);
				playerSavestates[player_id]->snapshot = tempGpuSnapshot;

				break;
			}
			// Player is updating their position, animation, etc
			case NM_PLAYERUPDATE: {
				NmPlayerUpdate* updateMsg = (NmPlayerUpdate*) buffer;

				if (player_id == localPlayerId)
					break;

				players[player_id].spyro = updateMsg->spyroData;
				players[player_id].powers = updateMsg->powers;

				// Convert animation state between PAL/NTSC
				if (gameState == GAMESTATE_INLEVEL && mobyModels && mobyModels[0].address) {
					uint8 anims[4] = {updateMsg->spyroData.main_anim.prevanim, updateMsg->spyroData.main_anim.nextanim, 
									  updateMsg->spyroData.head_anim.prevanim, updateMsg->spyroData.head_anim.nextanim};
					uint8* frames[4] = {&players[player_id].spyro.main_anim.prevframe, &players[player_id].spyro.main_anim.nextframe, 
									  &players[player_id].spyro.head_anim.prevframe, &players[player_id].spyro.head_anim.nextframe};
					uint8 numFrames[4] = {updateMsg->prevAnimNumFrames, updateMsg->nextAnimNumFrames, 
										  updateMsg->prevHeadAnimNumFrames, updateMsg->nextHeadAnimNumFrames};

					for (int i = 0; i < 4; i++) {
						if (mobyModels[0]->anims[anims[i]].address) {
							uint8 localNumFrames = mobyModels[0]->anims[anims[i]]->numFrames;

							if (numFrames[i])
								*frames[i] = *frames[i] * localNumFrames / numFrames[i];
							else
								*frames[i] = 0;
						}
					}
				}
				break;
			}
			// Player is downloading a savestate and wants more
			case NM_SAVESTATESTATUS:  {
				NmSavestateStatus* stateStatus = (NmSavestateStatus*) buffer;
				bool starting = true; // starting what??

				int sendNext = -1;
				for (int numStatesSent = 0; numStatesSent < 5; ++numStatesSent) {
					static NmSavestateData stateData;
					int lastSendNext = sendNext; // wtf??

					for (int j = sendNext + 1; j < 13*32; j ++) {
						if (j * 8192 > sizeof (Savestate)) {
							j = 0; // Wrap around

							if (lastSendNext == -1)
								goto BreakAll;
						}

						if (j == lastSendNext)
							goto BreakAll;

						if (!(stateStatus->dataFlags[j / 32] & 1 << (j & 31))) {
							// Receiver doesn't have this part yet; send this part!
							sendNext = j;
							break;
						}
					}
					
					if (sendNext == -1)
						break;

					// Copy savestate chunk data and send it
					int cpySize = sizeof (Savestate) - sendNext * 8192;

					if (cpySize > 8192)
						cpySize = 8192;

					stateData.msgId = NM_SAVESTATEDATA;
					stateData.dataId = sendNext;

					memcpy(stateData.data, &((char*) playerSavestates[player_id])[sendNext * 8192], cpySize);
					SendTo(&stateData, sizeof (stateData), &players[player_id].addr);

					continue;
					BreakAll:
					break;
				}

				// Update transfer status message
				char statusMsg[256] = "Transferring TO: \n";

				for (int i = 0; i < 13; i ++) {
					char tempStr[12];
					int bitsConfirmed = 0;

					for (int j = 0; j < 32; j ++) {
						if (stateStatus->dataFlags[i] & (1 << j))
							bitsConfirmed ++;
					}

					sprintf(tempStr, "%i%% ", bitsConfirmed * 100 / 32);
					strcat(statusMsg, tempStr);

					if (!(i & 3) && i > 0)
						strcat(statusMsg, "\n");
				}

				SetControlString(staticTransferStatus, statusMsg);

				break;
			}
			case NM_ATTACKFLAGS: {
				NmAttackFlag* msg = (NmAttackFlag*) buffer;

				// Hacky for loop to prevent affecting mobys beyond the total count
				for (int i = 0; i < 500; i ++) {
					if (mobys[i].state == -1)
						break;

					if (i == msg->objId) {
						mobys[i].attack_flags = msg->flags;
						break;
					}
				}

				break;
			}
			case NM_MOBYSYNC: {
				NmMobySync* msg = (NmMobySync*) buffer;

				if (*level != 17)
					break; // Buzz's Dungeon only

				mobys[msg->mobyId].x = msg->moby.x;
				mobys[msg->mobyId].y = msg->moby.y;
				mobys[msg->mobyId].z = msg->moby.z;
				mobys[msg->mobyId].anim = msg->moby.anim;
				mobys[msg->mobyId].angle = msg->moby.angle;
				mobys[msg->mobyId].animspeed = msg->moby.animspeed;
				mobys[msg->mobyId].animRun = msg->moby.animRun;
				mobys[msg->mobyId].state = 50;msg->moby.state;

				break;
			}
		}

		players[player_id].lastmessagetime = GetTickCount(); // This guy's still alive.
	}
}

int SendTo(const void* buffer, int buffer_length, Address* addr) {
	struct sockaddr_in temp;
	u_long value = 0;

	temp.sin_family = AF_INET;
	temp.sin_port = htons(addr->port);
	temp.sin_addr.S_un.S_un_b.s_b1 = addr->ip[0];
	temp.sin_addr.S_un.S_un_b.s_b2 = addr->ip[1];
	temp.sin_addr.S_un.S_un_b.s_b3 = addr->ip[2];
	temp.sin_addr.S_un.S_un_b.s_b4 = addr->ip[3];

	ioctlsocket(netSocket, FIONBIO, &value);

	return sendto(netSocket, (const char*) buffer, buffer_length, 0, (sockaddr*) &temp, sizeof (temp));
}

int RecvFrom(void* buffer, int buffer_length, Address* addr) {
	struct sockaddr_in temp;
	int ret;
	int temp_size = sizeof (temp);
	u_long value = 1;

	ioctlsocket(netSocket, FIONBIO, &value);

	ret = recvfrom(netSocket, (char*) buffer, buffer_length, 0, (sockaddr*) &temp, &temp_size);

	if (addr != NULL && ret > 0) {
		addr->port = ntohs(temp.sin_port);
		addr->ip[0] = temp.sin_addr.S_un.S_un_b.s_b1;
		addr->ip[1] = temp.sin_addr.S_un.S_un_b.s_b2;
		addr->ip[2] = temp.sin_addr.S_un.S_un_b.s_b3;
		addr->ip[3] = temp.sin_addr.S_un.S_un_b.s_b4;
	}

	return ret;
}

void Host() {
	if (!isNetworkInitialised)
	{
		MessageBox(hwndEditor, "Cannot host (network system was not successfully initialised).", 
				   "We'd apologise but we simply don't have the time for any of that.", MB_OK);
		return;
	}

	netstate = NETSTATE_SERVER;
	numPlayers = 1;
	localPlayerId = 0;

	EnableWindow(edit_ip, 0);
	EnableWindow(button_host, 0);
	EnableWindow(button_join, 0);
}

extern long (CALLBACK* route_GPUdmaChain)(unsigned long *,unsigned long);
extern long last_argtwo;

void Join() {
	if (!isNetworkInitialised) {
		MessageBox(hwndEditor, "Cannot join (network system was not successfully initialised).", 
				   "We'd apologise but we simply don't have the time for any of that.", MB_OK);
		return;
	}

	// Make an address from the IP given...
	char ip[16];
	int last_octect = 0;
	int octects = 0;

	SendMessage(edit_ip, WM_GETTEXT, (WPARAM) 16, (LPARAM) ip);

	for (int i = 0; i < 16; i ++) {
		if (ip[i] == '.' || !ip[i]) {
			int byte = atoi(&ip[last_octect]);

			if (byte > 255 || byte < 0)
				break;

			players[0].addr.ip[octects ++] = byte;

			last_octect = i + 1;

			if (!ip[i])
				break;
		}
	}

	if (octects != 4) {
		MessageBox(hwndEditor, "Invalid IP.", "We're helping you with this message. No, really.", MB_OK);
		return;
	}

	players[0].addr.port = 2012;

	// Send the join request, dude!
	char send_msg[8] = "\x00YODUDE";

	SendTo(send_msg, 8, &players[0].addr);

	// Wait up to five seconds for a response.
	DWORD start_time = GetTickCount();
	DWORD last_update = GetTickCount();
	Address recv_addr;
	char recv_buffer[8];
	bool joined = 0;

	while (!joined) {
		Sleep(10); // Give the CPU a rest.

		while (RecvFrom(recv_buffer, 8, &recv_addr) > 0) {
			if (!strncmp(recv_buffer, "\x00HEYBRO", 8)) {
				memcpy(&players[0].addr, &recv_addr, sizeof (Address)); // Just in case of a port inconsistency.

				joined = 1;
				break;
			}
		}

		if ((GetTickCount() - start_time) > 5000)
			break;
	}

	if (!joined) {
		MessageBox(hwndEditor, "Sorry - connection was unsuccessful.", "Failer, thy name is Failure", MB_OK);
		return;
	}

	memcpy(&players[0].addr, &recv_addr, sizeof (Address));
	memcpy(&players[0].spyro, (void*) ((uint32) memory + 0x00073624), sizeof (Spyro)); // To avoid crashes.
	players[0].lastmessagetime = GetTickCount();

	localPlayerId = 1;
	numPlayers = 2;

	netstate = NETSTATE_CLIENT;

	EnableWindow(edit_ip, 0);
	EnableWindow(button_host, 0);
	EnableWindow(button_join, 0);

	return;
	// Receive a save state
	DWORD lastRecvTime = GetTickCount();
	bool gotAll = false;
	NmSavestateStatus stateStatus;
	NmSavestateData stateData;
	static Savestate state;

	memset(&stateStatus, 0, sizeof (stateStatus));
	stateStatus.msgId = NM_SAVESTATESTATUS;

	SendTo(&stateStatus, sizeof (stateStatus), &players[0].addr);

	while (!gotAll) {
		bool send_update = 0;

		while (RecvFrom(&stateData, sizeof (stateData), NULL) > 0) {
			if (stateData.msgId != NM_SAVESTATEDATA) continue;

			// RAM copy
			if (stateData.dataId < 256) {
				/*int cpySize = 8192;

				if ((stateData.dataId + 1) * 8192 > sizeof (Savestate))
					cpySize = sizeof (Savestate) - stateData.dataId * 8192;*/
				int cpySize = sizeof (Savestate) - stateData.dataId * 8192;

				if (cpySize > 8192)
					cpySize = 8192;

				memcpy(&((unsigned char*) &state)[stateData.dataId * 8192], stateData.data, 8192);
			}
	
			stateStatus.dataFlags[stateData.dataId / 32] |= 1 << (stateData.dataId & 31);
			lastRecvTime = GetTickCount();
			send_update = 1;
		}

		if (GetTickCount() >= last_update + 1000) {
			send_update = 1;
			last_update = GetTickCount();
		}

		if (GetTickCount() >= lastRecvTime + 5000)
			return; // Give up

		if (send_update)
			SendTo(&stateStatus, sizeof (stateStatus), &players[0].addr);

		char statusMsg[256] = "Transferring FROM: \n";

		for (int i = 0; i < 13; i ++) {
			char tempStr[12];
			int bitsConfirmed = 0;

			for (int j = 0; j < 32; j ++) {
				if (stateStatus.dataFlags[i] & (1 << j))
					bitsConfirmed ++;
			}

			sprintf(tempStr, "%i%% ", bitsConfirmed * 100 / 32);
			strcat(statusMsg, tempStr);

			if (!(i & 3) && i > 0)
				strcat(statusMsg, "\n");
		}

		SetControlString(staticTransferStatus, statusMsg);
		
		// Are we finished?
		int numBits = 0;
		for (int i = 0; i < 13*32; i ++) {
			if (stateStatus.dataFlags[i / 32] & 1 << (i & 31))
				numBits ++;
		}

		if (numBits == ((sizeof (Savestate) + 8191) / 8192))
			gotAll = true;
	}

	//SetSnapshot(&state.snapshot);
	memcpy(memory, state.memory, 0x00200000);

	MessageBox(NULL, "Savestate transfer completed.", "", MB_OK);
}
