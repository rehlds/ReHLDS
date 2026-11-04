#include "precompiled.h"
#include "rehlds_tests_shared.h"
#include "cppunitlite/TestHarness.h"

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
	FormatBits(negotiation.GetAgreed(), hex);
}

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
