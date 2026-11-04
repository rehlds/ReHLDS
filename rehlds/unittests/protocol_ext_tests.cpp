#include "precompiled.h"
#include "rehlds_tests_shared.h"
#include "cppunitlite/TestHarness.h"

#include <chrono>

static void SetupNetMessage(byte *data, int size)
{
	net_message.buffername = "net_message";
	net_message.data = data;
	net_message.maxsize = size;
	net_message.cursize = 0;
	net_message.flags = SIZEBUF_CHECK_OVERFLOW;
}

TEST(VarInt32RoundTrip, ProtocolExt, 1000)
{
	byte data[64];
	SetupNetMessage(data, sizeof(data));

	const uint32 values[] = { 0, 1, 127, 128, 300, 16383, 16384, 0x0FFFFFFF, 0xFFFFFFFF };
	const int sizes[]     = { 1, 1, 1,   2,   2,   2,     3,     4,          5          };

	for (int i = 0; i < ARRAYSIZE(values); i++)
	{
		LONGS_EQUAL("byte size", sizes[i], MSG_ByteSizeVarInt32(values[i]));

		SZ_Clear(&net_message);
		MSG_WriteVarInt32(&net_message, values[i]);
		LONGS_EQUAL("encoded size", sizes[i], net_message.cursize);

		MSG_BeginReading();
		UINT32_EQUALS("decoded value", values[i], MSG_ReadVarInt32());
		CHECK("no badread", !msg_badread);
		LONGS_EQUAL("whole value read", net_message.cursize, msg_readcount);
	}
}

TEST(VarInt32Malformed, ProtocolExt, 1000)
{
	byte data[8];
	SetupNetMessage(data, sizeof(data));

	data[0] = 0x80;
	net_message.cursize = 1;
	MSG_BeginReading();
	MSG_ReadVarInt32();
	CHECK("value cut by the end of the message", msg_badread);

	const byte tooWide[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x1F };
	Q_memcpy(data, tooWide, sizeof(tooWide));
	net_message.cursize = sizeof(tooWide);
	MSG_BeginReading();
	MSG_ReadVarInt32();
	CHECK("value wider than 32 bits", msg_badread);

	const byte tooLong[] = { 0x80, 0x80, 0x80, 0x80, 0x80, 0x00 };
	Q_memcpy(data, tooLong, sizeof(tooLong));
	net_message.cursize = sizeof(tooLong);
	MSG_BeginReading();
	MSG_ReadVarInt32();
	CHECK("value longer than five bytes", msg_badread);
}

TEST(NetMessageHeader, ProtocolExt, 1000)
{
	byte data[512];
	sizebuf_t buf = { "NetMessageHeader", SIZEBUF_CHECK_OVERFLOW, data, sizeof(data), 0 };

	MSG_WriteByte(&buf, svc_nop);

	MSG_BeginNetMessage(&buf, NET_SETCONVAR);
	for (int i = 0; i < 200; i++)
		MSG_WriteByte(&buf, i);
	MSG_EndNetMessage(&buf);

	// opcode, subtype, two byte length, body
	LONGS_EQUAL("message size", 1 + 1 + 1 + 2 + 200, buf.cursize);
	LONGS_EQUAL("previous message kept", svc_nop, data[0]);
	LONGS_EQUAL("opcode", svc_netmessage, data[1]);
	LONGS_EQUAL("subtype", NET_SETCONVAR, data[2]);
	LONGS_EQUAL("length low group", (200 & 0x7F) | 0x80, data[3]);
	LONGS_EQUAL("length high group", 200 >> 7, data[4]);
	LONGS_EQUAL("body start", 0, data[5]);
	LONGS_EQUAL("body end", 199, data[5 + 199]);

	SZ_Clear(&buf);
	MSG_BeginNetMessage(&buf, NET_NOP);
	MSG_EndNetMessage(&buf);
	LONGS_EQUAL("empty message size", 3, buf.cursize);
	LONGS_EQUAL("empty message length", 0, data[2]);
}

TEST(UserMessageLayout, ProtocolExt, 1000)
{
	byte data[32];
	sizebuf_t buf = { "UserMessageLayout", SIZEBUF_ALLOW_OVERFLOW, data, sizeof(data), 0 };

	const byte body[] = { 1, 2, 3 };
	CHECK("message fits", SV_WriteUserMessage(&buf, "Hello", body, sizeof(body)));

	byte expected[] = { svc_usermessage, 'H', 'e', 'l', 'l', 'o', 0, 3, 1, 2, 3 };
	LONGS_EQUAL("message size", sizeof(expected), buf.cursize);
	MEM_EQUAL("message bytes", expected, data, sizeof(expected));

	byte large[32] = {};
	CHECK("message too large", !SV_WriteUserMessage(&buf, "Hello", large, sizeof(large)));
	LONGS_EQUAL("nothing written", sizeof(expected), buf.cursize);
	CHECK("buffer not overflowed", !(buf.flags & SIZEBUF_OVERFLOWED));
}

static int g_ReceivedSize;

static void ReceiveUserMessage(IGameClient *client, const uint8 *data, int size)
{
	g_ReceivedSize = size;
}

TEST(ClientUserMessageHandlers, ProtocolExt, 1000)
{
	CClientUserMessages handlers;
	const uint8 body[] = { 7, 7 };

	CHECK("register", handlers.Register("ClientHello", ReceiveUserMessage));
	CHECK("one handler per name", !handlers.Register("ClientHello", ReceiveUserMessage));
	CHECK("name too long", !handlers.Register("ThisNameIsTooLong", ReceiveUserMessage));

	g_ReceivedSize = -1;
	CHECK("dispatched", handlers.Dispatch(NULL, "ClientHello", body, sizeof(body)));
	LONGS_EQUAL("size passed", sizeof(body), g_ReceivedSize);
	CHECK("unknown name", !handlers.Dispatch(NULL, "Other", body, sizeof(body)));

	CHECK("unregister", handlers.Unregister("ClientHello", ReceiveUserMessage));
	CHECK("gone", !handlers.Dispatch(NULL, "ClientHello", body, sizeof(body)));
}

static void FormatBits(const CProtocolExtensionBits &bits, char *hex)
{
	CProtocolNegotiation::FormatBits(bits, hex, PROTOCOL_EXT_MAX_BITS / 4 + 1);
}

TEST(ExtensionBitsHex, ProtocolExt, 1000)
{
	CProtocolExtensionBits bits;
	char hex[PROTOCOL_EXT_MAX_BITS / 4 + 1];

	CHECK("parse 7", CProtocolNegotiation::ParseBits("7", &bits));
	CHECK("bit 0", bits.IsBitSet(0));
	CHECK("bit 1", bits.IsBitSet(1));
	CHECK("bit 2", bits.IsBitSet(2));
	CHECK("bit 3", !bits.IsBitSet(3));
	FormatBits(bits, hex);
	ZSTR_EQUAL("format 7", "7", hex);

	CHECK("parse 12", CProtocolNegotiation::ParseBits("12", &bits));
	CHECK("bit 0", bits.IsBitSet(0));
	CHECK("bit 5", bits.IsBitSet(5));
	FormatBits(bits, hex);
	ZSTR_EQUAL("format 12", "12", hex);

	CHECK("parse 0", CProtocolNegotiation::ParseBits("0", &bits));
	CHECK("empty set", bits.IsAllClear());
	FormatBits(bits, hex);
	ZSTR_EQUAL("empty set is 0", "0", hex);

	CHECK("numbers past the bit set are ignored", CProtocolNegotiation::ParseBits("0000000000000000f", &bits));
	CHECK("still empty", bits.IsAllClear());

	CHECK("not hex", !CProtocolNegotiation::ParseBits("7g", &bits));
}

class CExtensionSettings
{
public:
	CExtensionSettings(float usermessage, float replicatedcvars)
	{
		m_UserMessage = sv_rehlds_ext_usermessage.value;
		m_ReplicatedCvars = sv_rehlds_ext_replicatedcvars.value;
		sv_rehlds_ext_usermessage.value = usermessage;
		sv_rehlds_ext_replicatedcvars.value = replicatedcvars;
	}

	~CExtensionSettings()
	{
		sv_rehlds_ext_usermessage.value = m_UserMessage;
		sv_rehlds_ext_replicatedcvars.value = m_ReplicatedCvars;
	}

private:
	float m_UserMessage;
	float m_ReplicatedCvars;
};

static void Negotiate(CProtocolNegotiation *negotiation, const char *protinfo)
{
	negotiation->ParseRequest(protinfo);
	SV_ProtocolExtensions_internal(NULL, negotiation);
	negotiation->Agree();
}

static void AgreedHex(const CProtocolNegotiation &negotiation, char *hex)
{
	FormatBits(negotiation.GetAgreed().GetBits(), hex);
}

class CCvarValue
{
public:
	CCvarValue(cvar_t &var, float value) : m_Var(var), m_Saved(var.value)
	{
		m_Var.value = value;
	}

	~CCvarValue()
	{
		m_Var.value = m_Saved;
	}

private:
	cvar_t &m_Var;
	float m_Saved;
};

TEST(NegotiationAgreement, ProtocolExt, 1000)
{
	char hex[PROTOCOL_EXT_MAX_BITS / 4 + 1];

	{
		CExtensionSettings settings(1, 1);
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\prot\\3\\pext\\ff");
		CHECK("request seen", negotiation.HasRequest());
		AgreedHex(negotiation, hex);
		ZSTR_EQUAL("only implemented extensions", "7", hex);
		CHECK("nothing required", !negotiation.FindMissingRequired());
	}

	{
		CExtensionSettings settings(1, 0);
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\7");
		AgreedHex(negotiation, hex);
		ZSTR_EQUAL("replicatedcvars switched off", "3", hex);
	}

	{
		CExtensionSettings settings(1, 1);
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\4");
		AgreedHex(negotiation, hex);
		ZSTR_EQUAL("replicatedcvars without netmessage", "0", hex);
	}

	{
		CExtensionSettings settings(2, 1);
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\1");
		ZSTR_EQUAL("usermessage required", "usermessage", negotiation.FindMissingRequired());
	}

	{
		CExtensionSettings settings(2, 1);
		CProtocolNegotiation negotiation;
		negotiation.ParseRequest("\\prot\\3");
		CHECK("vanilla client", !negotiation.HasRequest());
		ZSTR_EQUAL("vanilla client is rejected too", "usermessage", negotiation.FindMissingRequired());
	}

	{
		CProtocolNegotiation negotiation;
		CHECK("malformed request", !negotiation.ParseRequest("\\pext\\xyz"));
	}
}

TEST(NegotiationReply, ProtocolExt, 1000)
{
	CExtensionSettings settings(1, 1);
	CProtocolNegotiation negotiation;
	Negotiate(&negotiation, "\\pext\\7");

	CHECK("module key", negotiation.AddReplyValue("gcl_ver", "1"));
	CHECK("key of another module", !negotiation.AddReplyValue("gcl_ver", "2"));
	CHECK("no prefix", !negotiation.AddReplyValue("pext", "0"));
	CHECK("empty prefix", !negotiation.AddReplyValue("_ver", "1"));
	CHECK("empty name", !negotiation.AddReplyValue("gcl_", "1"));
	CHECK("empty value", !negotiation.AddReplyValue("ncl_ver", ""));

	char reply[PROTOCOL_EXT_REPLY_SIZE];
	negotiation.BuildReply(reply, sizeof(reply));
	ZSTR_EQUAL("reply", "\\pext\\7\\gcl_ver\\1", reply);
}

TEST(NegotiationParams, ProtocolExt, 1000)
{
	char hex[PROTOCOL_EXT_MAX_BITS / 4 + 1];
	CExtensionSettings settings(1, 1);
	CCvarValue packetSize(sv_rehlds_ext_packetsize, 1);
	CCvarValue packetEntities(sv_rehlds_ext_packetentities, 1);
	CCvarValue maxPacketSize(sv_rehlds_maxpacketsize, 16384);
	CCvarValue maxPacketEntities(sv_rehlds_maxpacketentities, 400);

	{
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\75\\psize\\8000\\pents\\300");
		AgreedHex(negotiation, hex);
		ZSTR_EQUAL("both agreed", "75", hex);
		LONGS_EQUAL("client packet size", 8000, negotiation.GetAgreed().GetParam(PROTOCOL_EXT_PACKETSIZE));
		LONGS_EQUAL("client entity count", 300, negotiation.GetAgreed().GetParam(PROTOCOL_EXT_PACKETENTITIES));

		CHECK("module key", negotiation.AddReplyValue("gcl_ver", "1"));
		char reply[PROTOCOL_EXT_REPLY_SIZE];
		negotiation.BuildReply(reply, sizeof(reply));
		ZSTR_EQUAL("reply", "\\pext\\75\\psize\\8000\\pents\\300\\gcl_ver\\1", reply);
	}

	{
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\05\\psize\\99999\\pents\\4096");
		LONGS_EQUAL("server packet size", 16384, negotiation.GetAgreed().GetParam(PROTOCOL_EXT_PACKETSIZE));
		LONGS_EQUAL("server entity count", 400, negotiation.GetAgreed().GetParam(PROTOCOL_EXT_PACKETENTITIES));
	}

	{
		CCvarValue largePacketSize(sv_rehlds_maxpacketsize, 999999);
		CCvarValue largePacketEntities(sv_rehlds_maxpacketentities, 99999);
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\05\\psize\\999999\\pents\\99999");
		LONGS_EQUAL("engine packet size", NET_MAX_EXT_PACKET, negotiation.GetAgreed().GetParam(PROTOCOL_EXT_PACKETSIZE));
		LONGS_EQUAL("engine entity count", MAX_EXT_PACKET_ENTITIES, negotiation.GetAgreed().GetParam(PROTOCOL_EXT_PACKETENTITIES));
	}

	{
		CCvarValue maxRoutable(sv_rehlds_maxroutable, 1200);

		CProtocolNegotiation client;
		Negotiate(&client, "\\pext\\05\\psize\\8000\\proute\\1000");
		LONGS_EQUAL("client part size", 1000, client.GetAgreed().GetMaxRoutable());

		CProtocolNegotiation server;
		Negotiate(&server, "\\pext\\05\\psize\\8000\\proute\\1400");
		LONGS_EQUAL("server part size", 1200, server.GetAgreed().GetMaxRoutable());

		CProtocolNegotiation none;
		Negotiate(&none, "\\pext\\05\\psize\\8000");
		LONGS_EQUAL("part size without proute", 1200, none.GetAgreed().GetMaxRoutable());

		CProtocolNegotiation small;
		Negotiate(&small, "\\pext\\05\\psize\\8000\\proute\\100");
		LONGS_EQUAL("smallest part size", MIN_USER_MAXROUTABLE_SIZE, small.GetAgreed().GetMaxRoutable());

		CProtocolNegotiation noPacketSize;
		Negotiate(&noPacketSize, "\\pext\\01\\proute\\1000");
		LONGS_EQUAL("no packetsize, no part size", 0, noPacketSize.GetAgreed().GetMaxRoutable());
	}

	{
		CCvarValue largeMaxRoutable(sv_rehlds_maxroutable, 9999);
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\05\\psize\\8000\\proute\\5000");
		LONGS_EQUAL("largest part size", MAX_USER_MAXROUTABLE_SIZE, negotiation.GetAgreed().GetMaxRoutable());
	}

	{
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\05\\psize\\4000\\pents\\512");
		AgreedHex(negotiation, hex);
		ZSTR_EQUAL("packet size within the default limit", "0", hex);
	}

	{
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\05\\psize\\8000\\pents\\200");
		AgreedHex(negotiation, hex);
		ZSTR_EQUAL("entity count within the default limit", "01", hex);
		LONGS_EQUAL("no entity count", 0, negotiation.GetAgreed().GetParam(PROTOCOL_EXT_PACKETENTITIES));
	}

	{
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\01");
		AgreedHex(negotiation, hex);
		ZSTR_EQUAL("parameter missing", "0", hex);
	}

	{
		CCvarValue noPacketSize(sv_rehlds_ext_packetsize, 0);
		CProtocolNegotiation negotiation;
		Negotiate(&negotiation, "\\pext\\05\\psize\\8000\\pents\\512");
		AgreedHex(negotiation, hex);
		ZSTR_EQUAL("packetentities without packetsize", "0", hex);

		char reply[PROTOCOL_EXT_REPLY_SIZE];
		negotiation.BuildReply(reply, sizeof(reply));
		ZSTR_EQUAL("no parameters in the reply", "\\pext\\0", reply);
	}
}

// Keeps the datagrams the engine sends instead of sending them
class CSentDatagrams : public CSimplePlatform
{
public:
	CSentDatagrams() : m_Count(0) { }

	virtual int sendto(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen)
	{
		if (m_Count < MAX_SPLITPACKET_SPLITS)
		{
			Q_memcpy(m_Data[m_Count], buf, len);
			m_Size[m_Count] = len;
			m_Count++;
		}

		return len;
	}

	int Count() const { return m_Count; }
	int Size(int i) const { return m_Size[i]; }
	const byte *Data(int i) const { return m_Data[i]; }

	SPLITPACKET_EXT Header(int i) const
	{
		SPLITPACKET_EXT header;
		Q_memcpy(&header, m_Data[i], sizeof(header));
		return header;
	}

	int PartNumber(int i) const { return Header(i).packetID >> 8; }
	int PartCount(int i) const { return Header(i).packetID & 0xff; }

private:
	int m_Count;
	int m_Size[MAX_SPLITPACKET_SPLITS];
	byte m_Data[MAX_SPLITPACKET_SPLITS][MAX_USER_MAXROUTABLE_SIZE];
};

// A queue without the thread: it sends when the test says, by the clock of the test
class CTestQueuedPacketSender : public CQueuedPacketSender
{
public:
	CTestQueuedPacketSender() : CQueuedPacketSender(false), m_Time(0) { }

	void SendAt(uint32 time)
	{
		m_Time = time;
		SendDuePackets();
	}

protected:
	virtual uint32 GetTime() const { return m_Time; }

private:
	uint32 m_Time;
};

// A queue with its thread on a wall clock: the timer of the engine is not started in the tests
class CWallClockQueuedPacketSender : public CQueuedPacketSender
{
protected:
	virtual uint32 GetTime() const
	{
		return (uint32)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}
};

// Points the engine at the datagram catcher and puts the platform and the socket back after the test
class CSplitPacketTest
{
public:
	CSplitPacketTest() : m_Platform(CRehldsPlatformHolder::get()), m_Socket(ip_sockets[NS_SERVER])
	{
		m_Sent = new CSentDatagrams();
		CRehldsPlatformHolder::set(m_Sent);

		// Anything but INV_SOCK: the datagrams are caught before they reach a socket
		ip_sockets[NS_SERVER] = 1;

		Q_memset(&m_To, 0, sizeof(m_To));
		m_To.type = NA_IP;
		m_To.ip[0] = 127;
		m_To.ip[3] = 1;
	}

	~CSplitPacketTest()
	{
		CRehldsPlatformHolder::set(m_Platform);
		ip_sockets[NS_SERVER] = m_Socket;
		delete m_Sent;
	}

	CSentDatagrams &Sent() { return *m_Sent; }
	const netadr_t &To() const { return m_To; }

private:
	IReHLDSPlatform *m_Platform;
	SOCKET m_Socket;
	CSentDatagrams *m_Sent;
	netadr_t m_To;
};

TEST(SplitPacketExtParts, ProtocolExt, 1000)
{
	CSplitPacketTest test;
	CTestQueuedPacketSender queue;
	CCvarValue splitRate(sv_rehlds_splitrate, 1);

	byte packet[10000];
	for (int i = 0; i < (int)sizeof(packet); i++)
		packet[i] = (byte)(i * 7);

	// 1200 bytes of a part with its UDP header take 10 ms at this rate
	CSplitPacketSender sender(queue);
	sender.SetMaxRoutable(1200);
	sender.Send(NS_SERVER, test.To(), packet, sizeof(packet), 122800.0);

	// 1188 packet bytes in a part: 8 full parts and 496 bytes; the first part goes at once
	LONGS_EQUAL("split rate", 1, test.Sent().Count());

	queue.SendAt(9);
	LONGS_EQUAL("parts before their time", 1, test.Sent().Count());

	queue.SendAt(10);
	LONGS_EQUAL("the second part", 2, test.Sent().Count());

	queue.SendAt(45);
	LONGS_EQUAL("parts due by 45 ms", 5, test.Sent().Count());

	queue.SendAt(1000);
	LONGS_EQUAL("all parts", 9, test.Sent().Count());

	byte assembled[sizeof(packet)];
	int assembledSize = 0;
	for (int i = 0; i < test.Sent().Count(); i++)
	{
		SPLITPACKET_EXT header = test.Sent().Header(i);
		LONGS_EQUAL("netID", NET_HEADER_FLAG_SPLITPACKET_EXT, header.netID);
		LONGS_EQUAL("sequence", 1, header.sequenceNumber);
		LONGS_EQUAL("part number", i, test.Sent().PartNumber(i));
		LONGS_EQUAL("part count", 9, test.Sent().PartCount(i));
		LONGS_EQUAL("split size", 1188, header.nSplitSize);

		int size = test.Sent().Size(i) - (int)sizeof(SPLITPACKET_EXT);
		LONGS_EQUAL("bytes in the part", (i < 8) ? 1188 : 496, size);

		Q_memcpy(assembled + test.Sent().PartNumber(i) * header.nSplitSize, test.Sent().Data(i) + sizeof(SPLITPACKET_EXT), size);
		assembledSize += size;
	}

	LONGS_EQUAL("assembled size", sizeof(packet), assembledSize);
	CHECK("assembled packet", Q_memcmp(assembled, packet, sizeof(packet)) == 0);
}

TEST(SplitPacketExtQueue, ProtocolExt, 1000)
{
	CSplitPacketTest test;
	CTestQueuedPacketSender queue;
	CCvarValue splitRate(sv_rehlds_splitrate, 1);

	byte packet[5000];
	Q_memset(packet, 0, sizeof(packet));

	CSplitPacketSender sender(queue);
	sender.SetMaxRoutable(1200);
	CSplitPacketSender other(queue);
	other.SetMaxRoutable(1200);

	{
		CCvarValue threeParts(sv_rehlds_splitrate, 3);
		sender.Send(NS_SERVER, test.To(), packet, sizeof(packet), 122800.0);
		LONGS_EQUAL("three parts at once", 3, test.Sent().Count());
		queue.SendAt(1000);
		LONGS_EQUAL("the rest later", 5, test.Sent().Count());
	}

	// A reconnect drops the queued parts of the client, not of another one, and not its sequence
	sender.Send(NS_SERVER, test.To(), packet, sizeof(packet), 122800.0);
	other.Send(NS_SERVER, test.To(), packet, sizeof(packet), 122800.0);
	LONGS_EQUAL("first parts", 7, test.Sent().Count());

	sender.Clear();
	queue.SendAt(2000);
	LONGS_EQUAL("parts of the other client", 11, test.Sent().Count());

	// Without a rate the client takes everything at once
	sender.Send(NS_SERVER, test.To(), packet, sizeof(packet), 0.0);
	LONGS_EQUAL("no rate", 16, test.Sent().Count());
	LONGS_EQUAL("sequence after a reconnect", 3, test.Sent().Header(15).sequenceNumber);
}

TEST(SplitPacketExtFlush, ProtocolExt, 1000)
{
	CSplitPacketTest test;
	CTestQueuedPacketSender queue;
	CCvarValue splitRate(sv_rehlds_splitrate, 1);

	byte packet[5000];
	Q_memset(packet, 0, sizeof(packet));

	CSplitPacketSender sender(queue);
	sender.SetMaxRoutable(1200);

	// What the thread has not sent of a packet goes before the next one, parts of the two never mix
	sender.Send(NS_SERVER, test.To(), packet, sizeof(packet), 122800.0);
	sender.Send(NS_SERVER, test.To(), packet, sizeof(packet), 122800.0);
	LONGS_EQUAL("first packet and a part of the second", 6, test.Sent().Count());

	for (int i = 0; i < 5; i++)
		LONGS_EQUAL("parts of the first packet", 1, test.Sent().Header(i).sequenceNumber);

	LONGS_EQUAL("second packet", 2, test.Sent().Header(5).sequenceNumber);

	// The netchan rate counts every part with its header and a UDP header
	LONGS_EQUAL("sent size",5000 + 5 * (12 + UDP_HEADER_SIZE), sender.Send(NS_SERVER, test.To(), packet, sizeof(packet), 0.0));
}

TEST(SplitPacketExtSize, ProtocolExt, 1000)
{
	static byte packet[NET_MAX_EXT_PACKET];

	{
		CSplitPacketTest test;
		CSplitPacketSender sender;
		sender.SetMaxRoutable(MIN_USER_MAXROUTABLE_SIZE);
		sender.Send(NS_SERVER, test.To(), packet, NET_MAX_EXT_PACKET, 0.0);
		LONGS_EQUAL("smallest datagram", MIN_USER_MAXROUTABLE_SIZE, test.Sent().Size(0));
		LONGS_EQUAL("parts of the largest packet", (NET_MAX_EXT_PACKET + MIN_SPLIT_SIZE - 1) / MIN_SPLIT_SIZE, test.Sent().Count());
	}

	{
		CSplitPacketTest test;
		CSplitPacketSender sender;
		sender.SetMaxRoutable(MAX_USER_MAXROUTABLE_SIZE);
		sender.Send(NS_SERVER, test.To(), packet, 5000, 0.0);
		LONGS_EQUAL("largest datagram", MAX_USER_MAXROUTABLE_SIZE, test.Sent().Size(0));
	}
}

TEST(SplitPacketExtThread, ProtocolExt, 1000)
{
	CSplitPacketTest test;
	CCvarValue splitRate(sv_rehlds_splitrate, 1);

	byte packet[10000];
	Q_memset(packet, 0, sizeof(packet));

	// The thread sends the queued parts by itself: 8 of them, 10 ms apart
	CWallClockQueuedPacketSender queue;
	CSplitPacketSender sender(queue);
	sender.SetMaxRoutable(1200);
	sender.Send(NS_SERVER, test.To(), packet, sizeof(packet), 122800.0);

	for (int i = 0; i < 200 && test.Sent().Count() < 9; i++)
		Sys_Sleep(10);

	queue.Shutdown();
	LONGS_EQUAL("parts sent by the thread", 9, test.Sent().Count());
	LONGS_EQUAL("last part", 8, test.Sent().PartNumber(8));
}
