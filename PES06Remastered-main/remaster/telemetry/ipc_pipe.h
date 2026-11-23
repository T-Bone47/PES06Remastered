#ifndef REMASTER_IPC_PIPE_H
#define REMASTER_IPC_PIPE_H

#include <stddef.h>

namespace remaster {
namespace telemetry {

#define TELEMETRY_PIPE_NAME "\\\\.\\pipe\\PES6Remaster_Telemetry"
#define TELEMETRY_MAGIC 0x36534550 // 'PES6'
#define MAX_IPC_ENTITIES 32

#pragma pack(push, 1)
struct TelemetryPacketHeader {
    unsigned int magic;        // 'PES6' = 0x36534550
    unsigned int version;      // 1
    unsigned int sequence;     // Incremental packet sequence
    unsigned long long wallMs; // GetTickCount64
    int mode;                  // 0 = menu, 1/2 = active match
    int phase;                 // 0 = pre, 2/3/12 = match play
    int matchType;             // 4 = league, 5 = cup/ML
    float matchClock;          // Match elapsed clock in seconds
    int entityCount;           // Number of active entities populated
};

struct EntityTelemetry {
    short id;
    short side;                // 0 = Home, 1 = Away, -1 = Ball / Neutral
    float posX;                // Pitch 3D X coordinate
    float posY;                // Pitch 3D Y coordinate
    float posZ;                // Pitch 3D Z coordinate
    float velocityX;           // Velocity X
    float velocityY;           // Velocity Y
    float velocityZ;           // Velocity Z
    short stamina;             // Remaining stamina (0..1000)
    short flags;               // Bitflags (has ball, tackled, offside, etc.)
};

struct TelemetryPacket {
    TelemetryPacketHeader header;
    EntityTelemetry entities[MAX_IPC_ENTITIES];
    unsigned int checksum;     // Packet integrity checksum
};
#pragma pack(pop)

// Computes a fast 32-bit checksum over the packet data.
unsigned int computePacketChecksum(const TelemetryPacket& packet);

// Initializes the non-blocking named pipe server.
bool initPipeServer();

// Broadcasts a telemetry packet to any connected IPC client.
bool broadcastTelemetryPacket(TelemetryPacket& packet);

// Checks if an IPC companion client is currently connected.
bool isPipeClientConnected();

// Closes and cleans up the named pipe server.
void closePipeServer();

}  // namespace telemetry
}  // namespace remaster

#endif  // REMASTER_IPC_PIPE_H
