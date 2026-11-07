#pragma once

#include "engine.h"

struct sockaddr;

// Records the server socket's traffic into .gcap captures under <gamedir>/captures:
// net_record start [all | <ip> | <ip>/<bits> | #<userid>], net_record stop, net_record status.
// The calls below cost one check while nothing is recorded.
void NetRecord_Init();
void NetRecord_Shutdown();

void NetRecord_DatagramReceived(netsrc_t sock, const struct sockaddr &from, const void *data, int length);
void NetRecord_DatagramSent(netsrc_t sock, const struct sockaddr &to, const void *data, int length);
void NetRecord_Frame(int frame);

void NetRecord_ClientConnected(const client_t *client);
void NetRecord_ClientDropped(const client_t *client, const char *reason);

// The tables a recording needs to decode messages: after the map's precache, after a user message registration
void NetRecord_ServerActivated();
void NetRecord_UserMessagesChanged();

// Around the parsing of one client's messages: a long one is recorded with the time it took
void NetRecord_ClientMessagesStarted(const client_t *client);
void NetRecord_ClientMessagesFinished();

// A write did not fit buf and the engine is about to clear it
void NetRecord_BufferOverflowed(const sizebuf_t *buf, int length);
