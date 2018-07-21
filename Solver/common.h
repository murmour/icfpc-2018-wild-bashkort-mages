#ifndef __COMMON_H__
#define __COMMON_H__

#define INTERACTIVE_MODE
#define _CRT_SECURE_NO_WARNINGS

// uncomment to disable assertion checks
//#define NDEBUG
#include <assert.h>

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <queue>
#include <deque>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <cstdio>
#include <functional>

#undef min
#undef max

#include "json/json.h"

//#include "log.h"

#define Assert(x) if (!(x)) throw (__FILE__ + __LINE__)
#ifdef INTERACTIVE_MODE
#define dassert(x)
#else
#define dassert(x) assert(x)
#endif

#define E_PB_OK                0
#define E_PB_FAIL              1

#define SAFE_DELETE(p)  { if(p) { delete (p); (p)=NULL; } }
#define SAFE_DELETE_ARR(p)  { if(p) { delete [] (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

#define ensure_true(x) { bool __res = bool(x); assert(__res); }

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);   \
  void operator=(const TypeName&)

#ifndef _MSC_VER
#define sprintf_s sprintf
#endif

inline int imin(int a, int b) { return std::min(a, b); }

template<typename T>
inline T Sqr(T x)
{
	return x * x;
}

typedef float f32;
typedef double f64;

typedef signed char i8;
typedef unsigned char u8;
typedef int i32;
typedef unsigned int u32;
typedef long long i64;

typedef u32 PBBOOL;

#define PBTRUE				1
#define PBFALSE				0

#define ANSI_CODEPAGE		1251

#define F32INF 1e38f
#define I32INF 0x7fffffff

#define FLD_BEGIN \
	Json::Value _serialize() { Json::Value res; _enumerate(Serializer(res)); return res; } \
	bool _deserialize(const Json::Value &data) { Deserializer x = Deserializer(data); _enumerate(x); return x.result; } \
	template<typename F> void _enumerate(F f) {

// validator should be a method of this class with no arguments returning bool
#define FLD_BEGINV(validator) \
	Json::Value _serialize() { Json::Value res; _enumerate(Serializer(res)); return res; } \
	bool _deserialize(const Json::Value &data) { Deserializer x = Deserializer(data); _enumerate(x); \
	      if (!x.result) return false; return validator(); } \
	template<typename F> void _enumerate(F f) {

#define FLD_END }
#define FLD(fld, ...) f(#fld, fld, ##__VA_ARGS__);
#define FLD_BASE(base_class) base_class::_enumerate(f);


	template<typename T>
	Json::Value serializeJson(const T &f) {
		return const_cast<T&>(f)._serialize();
	}

#define SERIALIZABLE_ENUM(ty) \
	template<> \
	inline bool deserializeJson(ty &f, const Json::Value &data) \
	{ \
		if (!data.isIntegral()) return false; \
		f = static_cast<ty>(data.asInt()); \
		return true; \
	} \
	template<> \
	inline Json::Value serializeJson(const ty &f) { \
		return Json::Value(static_cast<i32>(f)); \
	}

#define TRIVIAL_SERIALIZE(ty) \
	template<> \
	inline Json::Value serializeJson(const ty &f) { \
		return Json::Value(f); \
	}

	TRIVIAL_SERIALIZE(u32)
	TRIVIAL_SERIALIZE(i32)
	TRIVIAL_SERIALIZE(f32)
	TRIVIAL_SERIALIZE(std::string)
	TRIVIAL_SERIALIZE(bool)
	TRIVIAL_SERIALIZE(Json::Value)

#undef TRIVIAL_SERIALIZE

	template<typename T>
	Json::Value serializeJson(const std::vector<T> &f)
	{
		Json::Value res;
		res.resize(f.size());
		for (u32 i = 0; i < f.size(); i++)
			res[i] = serializeJson(f[i]);
		return res;
	}

	template<typename T>
	bool deserializeJson(T &f, const Json::Value &data)
	{
		return f._deserialize(data);
	}

	template<typename T>
	bool deserializeJson(std::vector<T> &f, const Json::Value &data)
	{
		if (!data.isArray()) return false;
		f.resize(data.size());
		for (u32 i = 0; i < f.size(); i++)
			if (!deserializeJson(f[i], data[i]))
				return false;
		return true;
	}

	template<>
	inline bool deserializeJson(bool &f, const Json::Value &data)
	{
		if (!data.isBool()) return false;
		f = data.asBool();
		return true;
	}

	template<>
	inline bool deserializeJson(Json::Value &f, const Json::Value &data)
	{
		f = data;
		return true;
	}

	template<>
	inline bool deserializeJson(u32 &f, const Json::Value &data)
	{
		if (!data.isIntegral()) return false;
		f = data.asUInt();
		return true;
	}

	template<>
	inline bool deserializeJson(i32 &f, const Json::Value &data)
	{
		if (!data.isIntegral()) return false;
		f = data.asInt();
		return true;
	}

	template<>
	inline bool deserializeJson(f32 &f, const Json::Value &data)
	{
		if (!data.isNumeric()) return false;
		f = data.asFloat();
		return true;
	}

	template<>
	inline bool deserializeJson(std::string &f, const Json::Value &data)
	{
		if (!data.isString()) return false;
		f = data.asString();
		return true;
	}

	template<typename T, size_t sz>
	Json::Value serializeJson(const T (&f)[sz])
	{
		Json::Value res;
		res.resize(sz);
		for (u32 i = 0; i < sz; i++) res[i] = serializeJson(f[i]);
		return res;
	}

	template<size_t sz>
	Json::Value serializeJson(const char (&f)[sz])
	{
		return Json::Value(f);
	}

	template<typename T, size_t sz>
	bool deserializeJson(T (&f)[sz], const Json::Value &data)
	{
		if (!data.isArray() || data.size() != sz) return false;
		for (u32 i = 0; i < sz; i++)
			if (!deserializeJson(f[i], data[i])) return false;
		return true;
	}

	struct Serializer
	{
		Json::Value &json;
		Serializer(Json::Value &json) : json(json) {}
		template<typename T> void operator() (const std::string &name, const T &f, const Json::Value &initval = Json::Value())
		{
			json[name] = serializeJson(f);
		}
		bool isSerializing() const {
			return true;
		}
		bool hasKey(const char * key) const
		{
			return false;
		}
	};

	struct Deserializer
	{
		const Json::Value &json;
		bool result;
		Deserializer(const Json::Value &json) : json(json), result(true) {}
		template<typename T> void operator() (const std::string &name, T &f, const Json::Value &initval = Json::Value())
		{
			result = result && deserializeJson(f, json.get(name, initval));
		}
		bool isSerializing() const {
			return false;
		}
		bool hasKey(const char * key) const
		{
			return json.isMember(key);
		}
	};


#endif
