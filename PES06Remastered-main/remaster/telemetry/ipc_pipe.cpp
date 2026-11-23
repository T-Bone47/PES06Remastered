// remaster telemetry subsystem - low-latency named pipe IPC server.
//
// Streams 10 Hz real-time match telemetry (player coordinates, velocities,
// ball 3D position, match clock, and phase) to an external 64-bit companion
// application ("Sidecar") over Windows Named Pipes without loading external
// DLLs into the game's address space or introducing frame stutter.

#include "ipc_pipe.h"
#include "../core/remaster.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace remaster {
namespace telemetry {

static HANDLE g_pipeHandle = INVALID_HANDLE_VALUE;
static HANDLE g_connectEvent = NULL;
static OVERLAPPED g_connectOverlapped;
static bool g_clientConnected = false;
static bool g_pendingConnect = false;
static unsigned int g_sequenceNumber = 0;

unsigned int computePacketChecksum(const TelemetryPacket& packet) {
    // 32-bit Fletcher checksum across header and entity payload
    const unsigned short* data = (const unsigned short*)&packet;
    size_t words = (sizeof(TelemetryPacketHeader) + sizeof(EntityTelemetry) * MAX_IPC_ENTITIES) / 2;
    unsigned int sum1 = 0xffff, sum2 = 0xffff;

    while (words) {
        size_t tlen = words > 360 ? 360 : words;
        words -= tlen;
        do {
            sum1 += *data++;
            sum2 += sum1;
        } while (--tlen);
        sum1 = (sum1 & 0xffff) + (sum1 >> 16);
        sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    }
    sum1 = (sum1 & 0xffff) + (sum1 >> 16);
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    return (sum2 << 16) | sum1;
}

bool initPipeServer() {
    if (g_pipeHandle != INVALID_HANDLE_VALUE) {
        return true;
    }

    g_connectEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);
    if (!g_connectEvent) {
        log().write("WARN", "telemetry/ipc: failed to create connect event (err %lu)", GetLastError());
        return false;
    }

    memset(&g_connectOverlapped, 0, sizeof(g_connectOverlapped));
    g_connectOverlapped.hEvent = g_connectEvent;

    g_pipeHandle = CreateNamedPipeA(
        TELEMETRY_PIPE_NAME,
        PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,              // Max 1 client instance
        sizeof(TelemetryPacket) * 4,
        sizeof(TelemetryPacket) * 4,
        0,              // Default timeout
        nullptr
    );

    if (g_pipeHandle == INVALID_HANDLE_VALUE) {
        log().write("WARN", "telemetry/ipc: failed to create named pipe %s (err %lu)",
                    TELEMETRY_PIPE_NAME, GetLastError());
        CloseHandle(g_connectEvent);
        g_connectEvent = NULL;
        return false;
    }

    // Start waiting for client connection asynchronously
    BOOL connected = ConnectNamedPipe(g_pipeHandle, &g_connectOverlapped);
    if (connected) {
        g_clientConnected = true;
        g_pendingConnect = false;
        log().write("INFO", "telemetry/ipc: client connected immediately to %s", TELEMETRY_PIPE_NAME);
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            g_pendingConnect = true;
            g_clientConnected = false;
        } else if (err == ERROR_PIPE_CONNECTED) {
            g_clientConnected = true;
            g_pendingConnect = false;
            SetEvent(g_connectEvent);
        } else {
            log().write("WARN", "telemetry/ipc: ConnectNamedPipe failed (err %lu)", err);
        }
    }

    log().write("INFO", "telemetry/ipc: server ready on %s", TELEMETRY_PIPE_NAME);
    return true;
}

bool isPipeClientConnected() {
    if (g_pipeHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    if (g_pendingConnect) {
        DWORD bytesTransferred = 0;
        if (GetOverlappedResult(g_pipeHandle, &g_connectOverlapped, &bytesTransferred, FALSE)) {
            g_clientConnected = true;
            g_pendingConnect = false;
            log().write("INFO", "telemetry/ipc: companion Sidecar connected");
        }
    }

    return g_clientConnected;
}

bool broadcastTelemetryPacket(TelemetryPacket& packet) {
    if (!isPipeClientConnected()) {
        return false;
    }

    packet.header.magic = TELEMETRY_MAGIC;
    packet.header.version = 1;
    packet.header.sequence = ++g_sequenceNumber;
    packet.checksum = computePacketChecksum(packet);

    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(g_pipeHandle, &packet, (DWORD)sizeof(TelemetryPacket), &bytesWritten, nullptr);
    if (!ok || bytesWritten != sizeof(TelemetryPacket)) {
        DWORD err = GetLastError();
        if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA || err == ERROR_PIPE_NOT_CONNECTED) {
            log().write("INFO", "telemetry/ipc: Sidecar disconnected");
            DisconnectNamedPipe(g_pipeHandle);
            g_clientConnected = false;
            g_pendingConnect = true;
            ResetEvent(g_connectEvent);
            ConnectNamedPipe(g_pipeHandle, &g_connectOverlapped);
        }
        return false;
    }

    return true;
}

void closePipeServer() {
    if (g_pipeHandle != INVALID_HANDLE_VALUE) {
        if (g_clientConnected) {
            FlushFileBuffers(g_pipeHandle);
            DisconnectNamedPipe(g_pipeHandle);
        }
        CloseHandle(g_pipeHandle);
        g_pipeHandle = INVALID_HANDLE_VALUE;
    }
    if (g_connectEvent != NULL) {
        CloseHandle(g_connectEvent);
        g_connectEvent = NULL;
    }
    g_clientConnected = false;
    g_pendingConnect = false;
    g_sequenceNumber = 0;
}

}  // namespace telemetry
}  // namespace remaster
