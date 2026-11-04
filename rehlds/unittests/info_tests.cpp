#include "precompiled.h"
#include "rehlds_tests_shared.h"
#include "cppunitlite/TestHarness.h"

TEST(PrefixedKeysRemove, Info, 1000) {
	struct testdata_t {
		const char* inData;
		const char* outData;
	};

	testdata_t testdata[] = {
		{ "", "" },
		{ "\\key\\value", "\\key\\value" },
		{ "\\_key\\value", "" },
		{ "\\k\\v\\_u\\t\\y\\z", "\\k\\v\\y\\z" },
		{ "\\_k\\v\\u\\t\\y\\z", "\\u\\t\\y\\z" },
		{ "\\k\\v\\u\\t\\_y\\z", "\\k\\v\\u\\t" },
		{ "\\k\\v\\_u\\t\\*yyyyyyy\\zzzzzzz", "\\k\\v\\*yyyyyyy\\zzzzzzz" },
		{ "\\cl_updaterate\\60\\topcolor\\30\\_vgui_menus\\1\\_ah\\1\\_gm\\1ba5\\rate\\30000\\name\\aiaiaiaiaia\\*sid\\42893935\\model\\urban", "\\cl_updaterate\\60\\topcolor\\30\\rate\\30000\\name\\aiaiaiaiaia\\*sid\\42893935\\model\\urban" }
	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];
		char localInfo[256];
		strcpy(localInfo, d->inData);
		Info_RemovePrefixedKeys(localInfo, '_');
		ZSTR_EQUAL("Invalid info string", d->outData, localInfo);
	}

}

TEST(SetValueForStarKey, Info, 1000) {
	struct testdata_t {
		const char* initialInfo;
		const char* key;
		const char* value;
		const char* finalInfo;
	};

	testdata_t testdata[] = {
		// Behavior
		{ "\\a\\b", "c", "d", "\\a\\b\\c\\d" },
		{ "\\a\\b\\c\\d", "b", "f", "\\a\\b\\c\\d\\b\\f" },
		{ "\\a\\b\\c\\d", "c", "", "\\a\\b" },
		{ "\\a\\b\\c\\d\\e\\f", "c", "", "\\a\\b\\e\\f" },
		{ "\\a\\b\\c\\d\\e\\f", "z", "", "\\a\\b\\c\\d\\e\\f" },
		{ "\\a\\b\\c\\d", "a", "e", "\\c\\d\\a\\e" },
		{ "\\a\\b\\c\\d", "e", "f", "\\a\\b\\c\\d\\e\\f" },
		{ "\\a\\b\\c\\d", "b", "c", "\\a\\b\\c\\d\\b\\c" },
		{ "\\a\\b\\c\\d", "team", "aBcD", "\\a\\b\\c\\d\\team\\abcd" },

		// Invalid keys/values
		{ "\\a\\b", "c", nullptr, "\\a\\b" },
		{ "\\a\\b", nullptr, "c", "\\a\\b" },
		{ "\\a\\b", "c", "d..", "\\a\\b" },
		{ "\\a\\b", "..c", "d", "\\a\\b" },
		{ "\\a\\b", "c\"", "d", "\\a\\b" },
		{ "\\a\\b", "c", "d\"", "\\a\\b" },
		{ "\\a\\b", "c\\", "d", "\\a\\b" },
		{ "\\a\\b", "c", "d\\", "\\a\\b" },

		//limits
		{ //do nothing since 'team' is not important key
			"\\cl_updaterate\\100\\topcolor\\60\\name\\abcdefghijklmnop\\*sid\\12332432525345\\_vgui_menus\\1\\model\\urban\\somelargeuselesskey\\12312321321323123123123213123123123123123123123123123123123123123dasdsad\\_cl_autowepswitch\\1",
			"team",
			"1234567890123456789012345678901234567890",
			"\\cl_updaterate\\100\\topcolor\\60\\name\\abcdefghijklmnop\\*sid\\12332432525345\\_vgui_menus\\1\\model\\urban\\somelargeuselesskey\\12312321321323123123123213123123123123123123123123123123123123123dasdsad\\_cl_autowepswitch\\1",
		},

		{
			"\\cl_updaterate\\100\\topcolor\\60\\name\\abcdefghijklmnop\\*sid\\12332432525345\\_vgui_menus\\1\\model\\urban\\somelargeuselesskey\\12312321321323123123123213123123123123123123123123123123123123123dasdsad\\_cl_autowepswitch\\1",
			"*team",
			"12345678901234567890123456789012345678",
			"\\cl_updaterate\\100\\topcolor\\60\\name\\abcdefghijklmnop\\*sid\\12332432525345\\_vgui_menus\\1\\model\\urban\\_cl_autowepswitch\\1\\*team\\12345678901234567890123456789012345678",
		},

		{
			"\\cl_updaterate\\100\\topcolor\\60\\name\\abcdefghijklmnop\\*sid\\12332432525345\\_vgui_menus\\1\\model\\urban\\somelargeuselesskey\\12312321321323123123123213123123123123123123123123123123123123123dasdsad\\_cl_autowepswitch\\1",
			"*team",
			"1234567890123456789012345678901234567",
			"\\cl_updaterate\\100\\topcolor\\60\\name\\abcdefghijklmnop\\*sid\\12332432525345\\_vgui_menus\\1\\model\\urban\\somelargeuselesskey\\12312321321323123123123213123123123123123123123123123123123123123dasdsad\\_cl_autowepswitch\\1\\*team\\1234567890123456789012345678901234567",
		}
	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];
		char localInfo[257];
		strcpy(localInfo, d->initialInfo);
		localInfo[256] = 0;
		localInfo[255] = 0;
		Info_SetValueForStarKey(localInfo, d->key, d->value, 256);
		int infoLen = strlen(localInfo);
		CHECK("info string length", infoLen < 256);
		ZSTR_EQUAL("Invalid info string", d->finalInfo, localInfo);
	}
}

#ifdef REHLDS_FIXES
TEST(SetValueForStarKeyPriority, Info, 1000) {
	struct testdata_t {
		const char* initialInfo;
		const char* key;
		const char* value;
		const char* finalInfo;
	};

	testdata_t testdata[] = {
		{ // the largest unknown key goes, game preferences stay
			"\\name\\abcdefghijklmnopqrstuvwxyz012345678\\model\\urban\\_cl_autowepswitch\\0\\_ah\\0\\junk1\\012345678901234567890123456789012345678901234567890123456789\\junk2\\0123456789012345678901234567890123456789\\rate\\100000\\cl_updaterate\\101",
			"*sid",
			"0123456789012345678901234567890123456789",
			"\\name\\abcdefghijklmnopqrstuvwxyz012345678\\model\\urban\\_cl_autowepswitch\\0\\_ah\\0\\junk2\\0123456789012345678901234567890123456789\\rate\\100000\\cl_updaterate\\101\\*sid\\0123456789012345678901234567890123456789",
		},

		{ // unknown keys are not enough, the low priority one goes next
			"\\name\\abcdefghijklmnopqrstuvwxyz012345678\\model\\0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\\_cl_autowepswitch\\0\\_ah\\0\\j\\1\\cl_updaterate\\101\\rate\\100000",
			"*sid",
			"01234567890123456789012345678901234567890123456789",
			"\\name\\abcdefghijklmnopqrstuvwxyz012345678\\model\\0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\\_cl_autowepswitch\\0\\cl_updaterate\\101\\rate\\100000\\*sid\\01234567890123456789012345678901234567890123456789",
		},

		{ // a low priority key takes room only from unknown keys
			"\\name\\abcdefghijklmnopqrstuvwxyz012345678\\model\\0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\\_cl_autowepswitch\\0\\junk\\0123456789\\cl_updaterate\\101\\rate\\100000\\topcolor\\01234567890123456789012345",
			"_ah",
			"0",
			"\\name\\abcdefghijklmnopqrstuvwxyz012345678\\model\\0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\\_cl_autowepswitch\\0\\cl_updaterate\\101\\rate\\100000\\topcolor\\01234567890123456789012345\\_ah\\0",
		},

		{ // a key of the client goes after keys of plugins, even the largest one
			"\\name\\abcdefghijklmnopqrstuvwxyz012345678\\_pw\\0123456789012345678901234567890123456789012345678901234567890123456789\\junk1\\012345678901234567890123456789012345678901234\\model\\urban\\rate\\100000\\cl_updaterate\\101\\topcolor\\0123456789",
			"*sid",
			"01234567890123456789012345678901234567890123",
			"\\name\\abcdefghijklmnopqrstuvwxyz012345678\\_pw\\0123456789012345678901234567890123456789012345678901234567890123456789\\model\\urban\\rate\\100000\\cl_updaterate\\101\\topcolor\\0123456789\\*sid\\01234567890123456789012345678901234567890123",
		},
	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];
		char localInfo[257];
		strcpy(localInfo, d->initialInfo);
		Info_SetValueForStarKey(localInfo, d->key, d->value, 256);
		int infoLen = strlen(localInfo);
		CHECK("info string length", infoLen < 256);
		ZSTR_EQUAL("Invalid info string", d->finalInfo, localInfo);
	}
}

TEST(SetValueForStarKeyResult, Info, 1000) {
	struct testdata_t {
		const char* initialInfo;
		const char* key;
		const char* value;
		bool success;
	};

	testdata_t testdata[] = {
		// Behavior
		{ "\\a\\b", "c", "d", true },
		{ "\\a\\b\\c\\d", "b", "f", true },
		{ "\\a\\b\\c\\d", "b", "c", true },

		// Invalid keys/values
		{ "\\a\\b", "c", nullptr, false },
		{ "\\a\\b", nullptr, "c", false },
		{ "\\a\\b", "c", "d..", false },
		{ "\\a\\b", "..c", "d", false },
		{ "\\a\\b", "c\"", "d", false },
		{ "\\a\\b", "c", "d\"", false },
		{ "\\a\\b", "c\\", "d", false },
		{ "\\a\\b", "c", "d\\", false },
	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];
		char localInfo[256];
		strcpy(localInfo, d->initialInfo);
		localInfo[255] = 0;
		bool result = Info_SetValueForStarKey(localInfo, d->key, d->value, 256) ? true : false;
		CHECK("Invalid info string", d->success == result);
	}
}
#endif

TEST(RemoveKeyValue, Info, 1000) {
	struct testdata_t {
		const char* initialInfo;
		const char* key;
		const char* finalInfo;
	};

	testdata_t testdata[] = {
		{ "", "a", "" },
		{ "\\a\\b", "a", "" },
		{ "\\a\\b\\c\\d\\e\\f", "d", "\\a\\b\\c\\d\\e\\f" },
		{ "\\a\\b\\c\\d\\e\\f", "c", "\\a\\b\\e\\f" },
#ifdef REHLDS_FIXES
		{ "\\abc\\def\\x\\y\\ab\\cd", "ab", "\\abc\\def\\x\\y" },
#else
		{ "\\abc\\def\\x\\y\\ab\\cd", "ab", "\\x\\y" },
#endif
		{ "\\ab\\cd", "abc", "\\ab\\cd" }
	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];
		char localInfo[256];
		strcpy(localInfo, d->initialInfo);
		Info_RemoveKey(localInfo, d->key);
		ZSTR_EQUAL("Invalid info string", d->finalInfo, localInfo);
	}
}

TEST(GetKeyValue, Info, 1000) {
	struct testdata_t {
		const char* info;
		const char* key;
		const char* result;
	};

	testdata_t testdata[] = {
		{ "", "a", "" },
		{ "\\a\\b", "a", "b" },
		{ "\\a\\", "a", "" },
		{ "\\a\\\\", "a", "" },
		{ "\\a", "a", "" },
		{ "\\a\\b\\c\\d\\e\\f", "d", "" },
		{ "\\a\\b\\c\\d\\e\\f", "c", "d" },
		{ "\\a\\b\\c\\d\\e\\f", "e", "f" },

	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];

		const char* res = Info_ValueForKey(d->info, d->key);
		ZSTR_EQUAL("Invalid info value", d->result, res);
	}
}

TEST(FindLargestKey, Info, 1000) {
	struct testdata_t {
		const char* info;
		const char* result;
	};

	testdata_t testdata[] = {
		{ "", "" },
		{ "\\name\\a\\model\\b", "" },
		{ "\\name\\a\\model\\b\\c\\d", "c" },
		{ "\\name\\a\\1234567890abcdef\\b\\model\\c\\1234567890abcdefghi\\d\\rate\\1000", "1234567890abcdefghi" },
		{ "\\name\\a\\1234567890abcdefghi\\b\\model\\c\\1234567890abcdef\\d\\rate\\1000", "1234567890abcdefghi" }
	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];

		const char* res = Info_FindLargestKey(d->info, MAX_INFO_STRING);
		ZSTR_EQUAL("Invalid info value", d->result, res);
	}
}

TEST(InfoIsValid, Info, 1000) {
	struct testdata_t {
		const char* info;
		qboolean result;
	};

	testdata_t testdata[] = {
		{ "", false }, // by original design
#ifdef REHLDS_FIXES
		{ "a\\b", false },
#endif
		{ "\\a\\b", true },
		{ "\\a\\b\\c", false },
		{ "\\a\\b\\c\\", false },
		{ "\\a\\b\\c\\\\", false },
#ifdef REHLDS_FIXES
		{ "\\a\\b\\\\d", false },
		{ "\\a\\b\\\\d\\", false },
		{ "\\a\\b..c", false },
		{ "\\a\\b\"c", false },
		{ "\\a..b\\c", false },
		{ "\\a\"b\\c", false },
#endif
		{ "\\a\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", false },
		{ "\\bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\\c", false },
#ifdef REHLDS_FIXES
		{ "\\a\\b\\c\\d\\a\\e", false },
#endif
		{ "\\aaab\\b\\c\\d\\aaa\\e", true },
		{ "\\aaa\\b\\c\\d\\aaab\\e", true }
	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];

		char error[256];
		snprintf(error, sizeof error, "Invalid result for string '%s'", d->info);

		qboolean res = Info_IsValid(d->info);
		CHECK(error, d->result == res);
	}
}

#ifdef REHLDS_FIXES
TEST(InfoCollectFields, Info, 1000)
{
	struct testdata_t {
		const char* info;
		const char* cvar;
		const char* result;
	};

	testdata_t testdata[] = {
		{ "\\cl_updaterate\\100\\topcolor\\60\\name\\abcdefghijklmnop\\*sid\\12332432525345\\_vgui_menus\\1\\model\\urban\\anykey\\1", "", "\\cl_updaterate\\100\\topcolor\\60\\name\\abcdefghijklmnop\\*sid\\12332432525345\\model\\urban\\anykey\\1" },
		{ "\\cl_updaterate\\100\\topcolor\\60\\name\\abcdefghijklmnop\\*sid\\12332432525345\\_vgui_menus\\1\\model\\urban", "rate", "" },
		{ "\\cl_updaterate\\100\\topcolor\\60\\name\\abcdefghijklmnop\\*sid\\12332432525345\\_vgui_menus\\1\\model\\urban", "topcolor\\*sid\\_vgui_menus", "\\topcolor\\60\\*sid\\12332432525345" },
		{ "\\*hltv\\1dsgs", "*hltv", "\\*hltv\\1" },
		{ "\\name\\player", "bottomcolor\\name", "\\name\\player" },
	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];

		char destinfo[MAX_INFO_STRING];
		sv_rehlds_userinfo_transmitted_fields.string = (char *)d->cvar;
		Info_SetFieldsToTransmit();
		Info_CollectFields(destinfo, d->info, MAX_INFO_STRING);

		ZSTR_EQUAL("Invalid info string", d->result, destinfo);
	}
}

TEST(InfoShrinkToLength, Info, 1000)
{
	struct testdata_t {
		const char* info;
		size_t maxsize;
		const char* result;
	};

	testdata_t testdata[] = {
		{ // fits as it is
			"\\name\\player\\junk\\0123456789",
			64,
			"\\name\\player\\junk\\0123456789",
		},
		{ // the largest unknown key goes first, the low priority one stays while unknown keys are left
			"\\name\\player\\_ah\\1\\junk1\\0123456789\\junk2\\01234567890123456789",
			40,
			"\\name\\player\\_ah\\1\\junk1\\0123456789",
		},
		{ // unknown keys are not enough, the low priority one goes next
			"\\name\\player\\_ah\\1\\junk1\\0123456789",
			15,
			"\\name\\player",
		},
	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];

		char info[MAX_EXT_INFO_STRING];
		Q_strcpy(info, d->info);
		Info_ShrinkToLength(info, d->maxsize);

		CHECK("info string length", Q_strlen(info) < d->maxsize);
		ZSTR_EQUAL("Invalid info string", d->result, info);
	}
}

TEST(InfoCollectFieldsWholeKeys, Info, 1000)
{
	// Longer than MAX_INFO_STRING: the keys go whole, the important ones stay, *sid at the end among them
	char userinfo[MAX_EXT_INFO_STRING];
	Q_strcpy(userinfo, "\\name\\abcdefghijklmnopqrstuvwxyz01234\\model\\urban\\topcolor\\60\\bottomcolor\\60\\rate\\100000\\cl_updaterate\\101\\cl_lw\\1\\cl_lc\\1\\_vgui_menus\\1\\_cl_autowepswitch\\0");

	for (int i = 0; i < 6; i++)
		Q_strcat(userinfo, va("\\plugin%i\\0123456789012345678901234567890123456789", i));

	Q_strcat(userinfo, "\\*sid\\76561197960265728");

	sv_rehlds_userinfo_transmitted_fields.string = (char *)"";
	Info_SetFieldsToTransmit();

	char info[MAX_EXT_INFO_STRING];
	Info_CollectFields(info, userinfo, MAX_INFO_STRING);

	CHECK("info length", Q_strlen(info) < MAX_INFO_STRING);
	CHECK("info is valid", Info_IsValid(info) == TRUE);
	ZSTR_EQUAL("*sid", "76561197960265728", Info_ValueForKey(info, "*sid"));
	ZSTR_EQUAL("name", "abcdefghijklmnopqrstuvwxyz01234", Info_ValueForKey(info, "name"));
	ZSTR_EQUAL("private key", "", Info_ValueForKey(info, "_vgui_menus"));

	char extended[MAX_EXT_INFO_STRING];
	Info_CollectFields(extended, userinfo, MAX_EXT_INFO_STRING);

	ZSTR_EQUAL("plugin key", "0123456789012345678901234567890123456789", Info_ValueForKey(extended, "plugin5"));
	ZSTR_EQUAL("private key", "", Info_ValueForKey(extended, "_cl_autowepswitch"));
}
#endif

TEST(Info_IsKeyImportant, Info, 1000)
{
	struct testdata_t {
		const char* key;
		bool result;
	};

	testdata_t testdata[] = {
		{ "bottomcolor", true },
		{ "bot", false },
		{ "*any", true },
		{ "_any", false },
		{ "model2", false }
	};

	for (int i = 0; i < ARRAYSIZE(testdata); i++) {
		testdata_t* d = &testdata[i];

		bool result = Info_IsKeyImportant(d->key) ? true : false;

		CHECK("wrong result", d->result == result);
	}
}

size_t Info_GetBufferSize_api(const char *infobuffer);
bool Info_SetBufferValue_api(char *infobuffer, const char *key, const char *value);

TEST(InfoBufferSize, Info, 1000)
{
	char userinfo[MAX_CLIENT_INFO_STRING] = "";

	LONGS_EQUAL("localinfo", MAX_LOCALINFO, Info_GetBufferSize_api(localinfo));
	LONGS_EQUAL("serverinfo", MAX_INFO_STRING, Info_GetBufferSize_api(Info_Serverinfo()));
	LONGS_EQUAL("userinfo", MAX_CLIENT_INFO_STRING, Info_GetBufferSize_api(userinfo));
}

TEST(InfoBufferSetValue, Info, 1000)
{
	char userinfo[MAX_CLIENT_INFO_STRING] = "\\name\\player";

	CHECK("key set", Info_SetBufferValue_api(userinfo, "model", "urban"));
	CHECK("star key set", Info_SetBufferValue_api(userinfo, "*sid", "12345"));
	ZSTR_EQUAL("keys in the buffer", "\\name\\player\\model\\urban\\*sid\\12345", userinfo);

	CHECK("key with a slash refused", !Info_SetBufferValue_api(userinfo, "bad\\key", "1"));
	CHECK("key removed by an empty value", Info_SetBufferValue_api(userinfo, "model", ""));
	ZSTR_EQUAL("key removed", "\\name\\player\\*sid\\12345", userinfo);
}
