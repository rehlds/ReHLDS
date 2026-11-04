#include "precompiled.h"
#include "rehlds_tests_shared.h"
#include "cppunitlite/TestHarness.h"

void PF_MessageBegin_Intercept(int msg_dest, int msg_id, const float *pOrigin, edict_t *ed);
void PF_WriteByte_Intercept(int iValue);
void PF_WriteChar_Intercept(int iValue);
void PF_WriteShort_Intercept(int iValue);
void PF_WriteLong_Intercept(int iValue);
void PF_WriteAngle_Intercept(float flValue);
void PF_WriteCoord_Intercept(float flValue);
void PF_WriteString_Intercept(const char *sz);
void PF_WriteEntity_Intercept(int iValue);
void PF_MessageEnd_Intercept(void);

// No user message has these ids. A hook that doesn't pass the message on keeps it from being sent.
static const int TEST_MSG_ID = 250;
static const int TEST_INNER_MSG_ID = 251;

// What a hook saw, read back by the test once the message has ended
struct HookedMessage_t
{
	int calls;
	int paramCount;
	IMessage::ParamType types[16];
	int ints[16];
	float floats[16];
	char strings[16][64];
	int lastInt;
};

static HookedMessage_t g_Hooked;

static void ResetHooked()
{
	Q_memset(&g_Hooked, 0, sizeof(g_Hooked));
	g_Hooked.paramCount = -1;
}

static void RecordParams(IMessage *msg)
{
	g_Hooked.calls++;
	g_Hooked.paramCount = msg->getParamCount();

	for (int i = 0; i < msg->getParamCount() && i < ARRAYSIZE(g_Hooked.types); i++)
	{
		g_Hooked.types[i] = msg->getParamType(i);
		g_Hooked.ints[i] = msg->getParamInt(i);
		g_Hooked.floats[i] = msg->getParamFloat(i);

		if (g_Hooked.types[i] == IMessage::ParamType::String)
			Q_strlcpy(g_Hooked.strings[i], msg->getParamString(i));
	}

	g_Hooked.lastInt = msg->getParamInt(msg->getParamCount() - 1);
}

static void HookRecord(IVoidHookChain<IMessage *> *chain, IMessage *msg)
{
	RecordParams(msg);
}

static void SendBytes(int msgId, int count)
{
	PF_MessageBegin_Intercept(MSG_BROADCAST, msgId, NULL, NULL);
	for (int i = 0; i < count; i++)
		PF_WriteByte_Intercept(i & 0x7F);
	PF_MessageEnd_Intercept();
}

TEST(HookSeesEveryParam, MessageManager, 1000)
{
	ResetHooked();
	MessageManager().registerHook(TEST_MSG_ID, HookRecord);
	SendBytes(TEST_MSG_ID, 40);
	MessageManager().unregisterHook(TEST_MSG_ID, HookRecord);

	LONGS_EQUAL("every param seen", 40, g_Hooked.paramCount);
	LONGS_EQUAL("last param", 39, g_Hooked.lastInt);
}

static void HookGrowString(IVoidHookChain<IMessage *> *chain, IMessage *msg)
{
	msg->setParamString(0, "a string longer than the one written");
	RecordParams(msg);
}

// A message of the engine the game DLL sends is not bound by the size limit of user messages
TEST(HookSeesMessageOfFullBuffer, MessageManager, 1000)
{
	ResetHooked();
	MessageManager().registerHook(TEST_MSG_ID, HookGrowString);

	PF_MessageBegin_Intercept(MSG_BROADCAST, TEST_MSG_ID, NULL, NULL);
	PF_WriteString_Intercept("short");
	for (int i = 0; i < 400; i++)
		PF_WriteByte_Intercept(i & 0x7F);
	PF_MessageEnd_Intercept();

	MessageManager().unregisterHook(TEST_MSG_ID, HookGrowString);

	LONGS_EQUAL("every param seen", 401, g_Hooked.paramCount);
	ZSTR_EQUAL("string changed", "a string longer than the one written", g_Hooked.strings[0]);
	LONGS_EQUAL("bytes after the string moved with it", 399 & 0x7F, g_Hooked.lastInt);
}

TEST(HookSeesExtendedMessage, MessageManager, 1000)
{
	const int msgId = SV_RegUserMsgEx("MsgMngrTest", -1, USERMSG_EXTENDED);
	CHECK("extended message registered", msgId != 0);

	ResetHooked();
	MessageManager().registerHook(msgId, HookGrowString);

	PF_MessageBegin_Intercept(MSG_BROADCAST, msgId, NULL, NULL);
	PF_WriteString_Intercept("short");
	for (int i = 0; i < 3000; i++)
		PF_WriteByte_Intercept(i & 0x7F);
	PF_MessageEnd_Intercept();

	MessageManager().unregisterHook(msgId, HookGrowString);

	LONGS_EQUAL("every param seen", 3001, g_Hooked.paramCount);
	ZSTR_EQUAL("string changed", "a string longer than the one written", g_Hooked.strings[0]);
	LONGS_EQUAL("bytes after the string moved with it", 2999 & 0x7F, g_Hooked.lastInt);
}

static void SendEveryType(int msgId)
{
	PF_MessageBegin_Intercept(MSG_BROADCAST, msgId, NULL, NULL);
	PF_WriteByte_Intercept(200);
	PF_WriteChar_Intercept(-5);
	PF_WriteShort_Intercept(-1234);
	PF_WriteLong_Intercept(0x12345678);
	PF_WriteAngle_Intercept(90.0f);
	PF_WriteCoord_Intercept(123.5f);
	PF_WriteString_Intercept("text");
	PF_WriteEntity_Intercept(7);
	PF_MessageEnd_Intercept();
}

TEST(HookReadsEveryParamType, MessageManager, 1000)
{
	ResetHooked();
	MessageManager().registerHook(TEST_MSG_ID, HookRecord);
	SendEveryType(TEST_MSG_ID);
	MessageManager().unregisterHook(TEST_MSG_ID, HookRecord);

	LONGS_EQUAL("param count", 8, g_Hooked.paramCount);

	CHECK("byte type", g_Hooked.types[0] == IMessage::ParamType::Byte);
	CHECK("char type", g_Hooked.types[1] == IMessage::ParamType::Char);
	CHECK("short type", g_Hooked.types[2] == IMessage::ParamType::Short);
	CHECK("long type", g_Hooked.types[3] == IMessage::ParamType::Long);
	CHECK("angle type", g_Hooked.types[4] == IMessage::ParamType::Angle);
	CHECK("coord type", g_Hooked.types[5] == IMessage::ParamType::Coord);
	CHECK("string type", g_Hooked.types[6] == IMessage::ParamType::String);
	CHECK("entity type", g_Hooked.types[7] == IMessage::ParamType::Entity);

	LONGS_EQUAL("byte", 200, g_Hooked.ints[0]);
	LONGS_EQUAL("char keeps its sign", -5, g_Hooked.ints[1]);
	LONGS_EQUAL("short keeps its sign", -1234, g_Hooked.ints[2]);
	LONGS_EQUAL("long", 0x12345678, g_Hooked.ints[3]);
	DOUBLES_EQUAL("angle", 90.0, g_Hooked.floats[4], 0.01);
	DOUBLES_EQUAL("coord", 123.5, g_Hooked.floats[5], 0.01);
	ZSTR_EQUAL("string", "text", g_Hooked.strings[6]);
	LONGS_EQUAL("entity", 7, g_Hooked.ints[7]);
}

static bool g_ModifiedBeforeChange;
static bool g_ModifiedParam[8];
static int g_OriginalShort;
static char g_OriginalString[64];

static void HookChangeParams(IVoidHookChain<IMessage *> *chain, IMessage *msg)
{
	g_ModifiedBeforeChange = msg->isDataModified();

	msg->setParamInt(2, 4321);
	msg->setParamFloat(5, -64.25f);
	msg->setParamString(6, "a longer text");

	for (int i = 0; i < ARRAYSIZE(g_ModifiedParam); i++)
		g_ModifiedParam[i] = msg->isDataModified(IMessage::DataType::Param, i);

	g_OriginalShort = msg->getOriginalParamInt(2);
	Q_strlcpy(g_OriginalString, msg->getOriginalParamString(6));

	RecordParams(msg);
}

TEST(ChangedParamsKeepOriginals, MessageManager, 1000)
{
	ResetHooked();
	MessageManager().registerHook(TEST_MSG_ID, HookChangeParams);
	SendEveryType(TEST_MSG_ID);
	MessageManager().unregisterHook(TEST_MSG_ID, HookChangeParams);

	CHECK("unchanged message", !g_ModifiedBeforeChange);

	LONGS_EQUAL("short changed", 4321, g_Hooked.ints[2]);
	DOUBLES_EQUAL("coord changed", -64.25, g_Hooked.floats[5], 0.01);
	ZSTR_EQUAL("string changed", "a longer text", g_Hooked.strings[6]);
	LONGS_EQUAL("entity after the string", 7, g_Hooked.ints[7]);

	CHECK("short marked", g_ModifiedParam[2]);
	CHECK("coord marked", g_ModifiedParam[5]);
	CHECK("string marked", g_ModifiedParam[6]);
	CHECK("byte not marked", !g_ModifiedParam[0]);
	CHECK("entity not marked", !g_ModifiedParam[7]);

	LONGS_EQUAL("original short", -1234, g_OriginalShort);
	ZSTR_EQUAL("original string", "text", g_OriginalString);
}

static bool g_ModifiedAfterReset;

static void HookChangeAndReset(IVoidHookChain<IMessage *> *chain, IMessage *msg)
{
	msg->setParamInt(0, 1);
	msg->setParamString(6, "a much longer text than the original one");
	msg->setDest(IMessage::Dest::ALL);

	msg->resetModifiedData();
	g_ModifiedAfterReset = msg->isDataModified();

	RecordParams(msg);
}

TEST(ResetRestoresTheMessage, MessageManager, 1000)
{
	ResetHooked();
	MessageManager().registerHook(TEST_MSG_ID, HookChangeAndReset);
	SendEveryType(TEST_MSG_ID);
	MessageManager().unregisterHook(TEST_MSG_ID, HookChangeAndReset);

	CHECK("nothing modified", !g_ModifiedAfterReset);
	LONGS_EQUAL("byte back", 200, g_Hooked.ints[0]);
	ZSTR_EQUAL("string back", "text", g_Hooked.strings[6]);
	LONGS_EQUAL("entity after the string back in place", 7, g_Hooked.ints[7]);
}

static void HookSetVec(IVoidHookChain<IMessage *> *chain, IMessage *msg)
{
	const float pos[3] = { 1.0f, -2.5f, 300.0f };
	msg->setParamVec(1, pos);

	RecordParams(msg);
}

TEST(SetVecWritesItsOwnCoords, MessageManager, 1000)
{
	ResetHooked();
	MessageManager().registerHook(TEST_MSG_ID, HookSetVec);

	PF_MessageBegin_Intercept(MSG_BROADCAST, TEST_MSG_ID, NULL, NULL);
	PF_WriteByte_Intercept(11);
	PF_WriteCoord_Intercept(0.0f);
	PF_WriteCoord_Intercept(0.0f);
	PF_WriteCoord_Intercept(0.0f);
	PF_WriteByte_Intercept(22);
	PF_MessageEnd_Intercept();

	MessageManager().unregisterHook(TEST_MSG_ID, HookSetVec);

	LONGS_EQUAL("byte before", 11, g_Hooked.ints[0]);
	DOUBLES_EQUAL("x", 1.0, g_Hooked.floats[1], 0.01);
	DOUBLES_EQUAL("y", -2.5, g_Hooked.floats[2], 0.01);
	DOUBLES_EQUAL("z", 300.0, g_Hooked.floats[3], 0.01);
	LONGS_EQUAL("byte after", 22, g_Hooked.ints[4]);
}

static void HookShrinkString(IVoidHookChain<IMessage *> *chain, IMessage *msg)
{
	msg->setParamString(0, "s");
	RecordParams(msg);
}

TEST(ShorterStringMovesFollowingParams, MessageManager, 1000)
{
	ResetHooked();
	MessageManager().registerHook(TEST_MSG_ID, HookShrinkString);

	PF_MessageBegin_Intercept(MSG_BROADCAST, TEST_MSG_ID, NULL, NULL);
	PF_WriteString_Intercept("a long string to cut");
	PF_WriteShort_Intercept(-777);
	PF_WriteString_Intercept("tail");
	PF_MessageEnd_Intercept();

	MessageManager().unregisterHook(TEST_MSG_ID, HookShrinkString);

	ZSTR_EQUAL("string cut", "s", g_Hooked.strings[0]);
	LONGS_EQUAL("short after it", -777, g_Hooked.ints[1]);
	ZSTR_EQUAL("last string", "tail", g_Hooked.strings[2]);
}

static IMessage::Dest g_Dest, g_OriginalDest;
static int g_Id, g_OriginalId;
static float g_Origin[3], g_OriginalOrigin[3];
static edict_t *g_Edict, *g_OriginalEdict;
static bool g_ModifiedDest, g_ModifiedId, g_ModifiedOrigin, g_ModifiedEdict, g_ModifiedParams;

static void HookChangeMessageData(IVoidHookChain<IMessage *> *chain, IMessage *msg)
{
	static edict_t s_Receiver;
	const float origin[3] = { 10.0f, 20.0f, 30.0f };

	msg->setDest(IMessage::Dest::ONE);
	msg->setId(TEST_INNER_MSG_ID);
	msg->setOrigin(origin);
	msg->setEdict(&s_Receiver);

	g_Dest = msg->getDest();
	g_OriginalDest = msg->getOriginalDest();
	g_Id = msg->getId();
	g_OriginalId = msg->getOriginalId();
	VectorCopy(msg->getOrigin(), g_Origin);
	VectorCopy(msg->getOriginalOrigin(), g_OriginalOrigin);
	g_Edict = msg->getEdict();
	g_OriginalEdict = msg->getOriginalEdict();

	g_ModifiedDest = msg->isDataModified(IMessage::DataType::Dest);
	g_ModifiedId = msg->isDataModified(IMessage::DataType::Index);
	g_ModifiedOrigin = msg->isDataModified(IMessage::DataType::Origin);
	g_ModifiedEdict = msg->isDataModified(IMessage::DataType::Edict);
	g_ModifiedParams = msg->isDataModified(IMessage::DataType::Param);
}

TEST(ChangedMessageDataKeepsOriginals, MessageManager, 1000)
{
	const float origin[3] = { 1.0f, 2.0f, 3.0f };

	MessageManager().registerHook(TEST_MSG_ID, HookChangeMessageData);
	PF_MessageBegin_Intercept(MSG_PVS, TEST_MSG_ID, origin, NULL);
	PF_WriteByte_Intercept(1);
	PF_MessageEnd_Intercept();
	MessageManager().unregisterHook(TEST_MSG_ID, HookChangeMessageData);

	CHECK("dest changed", g_Dest == IMessage::Dest::ONE);
	CHECK("original dest", g_OriginalDest == IMessage::Dest::PVS);
	LONGS_EQUAL("id changed", TEST_INNER_MSG_ID, g_Id);
	LONGS_EQUAL("original id", TEST_MSG_ID, g_OriginalId);
	DOUBLES_EQUAL("origin changed", 20.0, g_Origin[1], 0.01);
	DOUBLES_EQUAL("original origin", 2.0, g_OriginalOrigin[1], 0.01);
	CHECK("edict changed", g_Edict != NULL);
	CHECK("original edict", g_OriginalEdict == NULL);

	CHECK("dest marked", g_ModifiedDest);
	CHECK("id marked", g_ModifiedId);
	CHECK("origin marked", g_ModifiedOrigin);
	CHECK("edict marked", g_ModifiedEdict);
	CHECK("params not marked", !g_ModifiedParams);
}

static void HookChangeAndPassOn(IVoidHookChain<IMessage *> *chain, IMessage *msg)
{
	msg->setParamInt(0, 99);
	msg->setParamString(1, "changed by the first hook");
	chain->callNext(msg);
}

TEST(NextHookSeesTheChanges, MessageManager, 1000)
{
	ResetHooked();
	MessageManager().registerHook(TEST_MSG_ID, HookChangeAndPassOn, HC_PRIORITY_HIGH);
	MessageManager().registerHook(TEST_MSG_ID, HookRecord, HC_PRIORITY_LOW);

	PF_MessageBegin_Intercept(MSG_BROADCAST, TEST_MSG_ID, NULL, NULL);
	PF_WriteByte_Intercept(1);
	PF_WriteString_Intercept("written");
	PF_WriteByte_Intercept(55);
	PF_MessageEnd_Intercept();

	MessageManager().unregisterHook(TEST_MSG_ID, HookRecord);
	MessageManager().unregisterHook(TEST_MSG_ID, HookChangeAndPassOn);

	LONGS_EQUAL("one call", 1, g_Hooked.calls);
	LONGS_EQUAL("byte from the first hook", 99, g_Hooked.ints[0]);
	ZSTR_EQUAL("string from the first hook", "changed by the first hook", g_Hooked.strings[1]);
	LONGS_EQUAL("byte after the string", 55, g_Hooked.ints[2]);
}

TEST(BlockedMessageSkipsHooks, MessageManager, 1000)
{
	ResetHooked();
	MessageManager().registerHook(TEST_MSG_ID, HookRecord);

	MessageManager().setMessageBlock(TEST_MSG_ID, IMessage::BlockType::Once);
	SendBytes(TEST_MSG_ID, 3);
	LONGS_EQUAL("blocked once", 0, g_Hooked.calls);
	CHECK("block once is spent", MessageManager().getMessageBlock(TEST_MSG_ID) == IMessage::BlockType::Not);

	SendBytes(TEST_MSG_ID, 3);
	LONGS_EQUAL("passes after the block", 1, g_Hooked.calls);

	MessageManager().setMessageBlock(TEST_MSG_ID, IMessage::BlockType::Set);
	SendBytes(TEST_MSG_ID, 3);
	SendBytes(TEST_MSG_ID, 3);
	LONGS_EQUAL("blocked while set", 1, g_Hooked.calls);

	MessageManager().setMessageBlock(TEST_MSG_ID, IMessage::BlockType::Not);
	MessageManager().unregisterHook(TEST_MSG_ID, HookRecord);
}

static int g_OuterParamCount, g_OuterLastByte;

static void HookSendInner(IVoidHookChain<IMessage *> *chain, IMessage *msg)
{
	// A hook may send a message of its own while the outer one is still on the stack
	PF_MessageBegin_Intercept(MSG_BROADCAST, TEST_INNER_MSG_ID, NULL, NULL);
	PF_WriteString_Intercept("inner");
	PF_WriteByte_Intercept(42);
	PF_MessageEnd_Intercept();

	g_OuterParamCount = msg->getParamCount();
	g_OuterLastByte = msg->getParamInt(msg->getParamCount() - 1);
}

TEST(HookSendsMessageOfItsOwn, MessageManager, 1000)
{
	ResetHooked();
	MessageManager().registerHook(TEST_MSG_ID, HookSendInner);
	MessageManager().registerHook(TEST_INNER_MSG_ID, HookRecord);

	SendBytes(TEST_MSG_ID, 5);

	MessageManager().unregisterHook(TEST_INNER_MSG_ID, HookRecord);
	MessageManager().unregisterHook(TEST_MSG_ID, HookSendInner);

	LONGS_EQUAL("inner hook called", 1, g_Hooked.calls);
	ZSTR_EQUAL("inner string", "inner", g_Hooked.strings[0]);
	LONGS_EQUAL("inner byte", 42, g_Hooked.ints[1]);
	LONGS_EQUAL("outer params kept", 5, g_OuterParamCount);
	LONGS_EQUAL("outer last byte kept", 4, g_OuterLastByte);
}
