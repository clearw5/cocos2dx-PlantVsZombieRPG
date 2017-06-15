#pragma once
#include "cocos2d.h"
using namespace std;
USING_NS_CC;

class Tools {
public:
	static long currentTimeInMillis()
	{
		struct  timeval tv;
		gettimeofday(&tv, NULL);
		return  tv.tv_sec * 1000 + tv.tv_usec / 1000;
	}

	static int getIntOrDefault(const ValueMap &m, const string &key, int defaultValue) {
		auto v = m.find(key);
		if (v != m.end()) {
			return v->second.asInt();
		}
		return defaultValue;
	}

	static void assignIfContainsKey( const ValueMap &m, const string &key, int &out) {
		auto v = m.find(key);
		if (v != m.end()) {
			out = v->second.asInt();
		}
	}

	static float getFloatOrDefault(const ValueMap &m, const string &key, float defaultValue) {
		auto v = m.find(key);
		if (v != m.end()) {
			return v->second.asFloat();
		}
		return defaultValue;
	}


	static void assignIfContainsKey(const ValueMap &m, const string &key, float &out) {
		auto v = m.find(key);
		if (v != m.end()) {
			out = v->second.asFloat();
		}
	}
};
