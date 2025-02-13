
#include "stdafx.hpp"

#include "JSONTypes.hpp"
#include "Helpers.hpp"

namespace flex
{
	JSONObject JSONObject::s_EmptyObject;
	std::vector<JSONObject> JSONObject::s_EmptyObjectArray;
	std::vector<JSONField> JSONObject::s_EmptyFieldArray;

	ValueType JSONValue::TypeFromChar(char c, const std::string& stringAfter)
	{
		switch (c)
		{
		case '{':
			return ValueType::OBJECT;
		case '[':
			if (stringAfter[0] == '{')
			{
				return ValueType::OBJECT_ARRAY;
			}
			else
			{
				// Arrays of strings are not supported
				return ValueType::FIELD_ARRAY;
			}
		case '\"':
			return ValueType::STRING;
		case 't':
		case 'f':
			if (c == 't' && stringAfter.size() >= 3 && stringAfter.substr(0, 3).compare("rue") == 0)
				return ValueType::BOOL;
			if (c == 'f' && stringAfter.size() >= 4 && stringAfter.substr(0, 4).compare("alse") == 0)
				return ValueType::BOOL;
			// Fall through
		default:
		{
			// Check if number
			bool isDigit = (isdigit(c) != 0);
			bool isDecimal = (c == '.');
			bool isNegation = (c == '-');

			if (isDigit || isDecimal || isNegation)
			{
				i32 nextNonAlphaNumeric = NextNonAlphaNumeric(stringAfter, 0);
				if (isDecimal || (nextNonAlphaNumeric != -1 && !stringAfter.empty() && stringAfter[nextNonAlphaNumeric] == '.'))
				{
					return ValueType::FLOAT;
				}
				else
				{
					std::string numberStr = std::string(1, c) + stringAfter.substr(0, nextNonAlphaNumeric);

					if (isNegation)
					{
						i64 result = strtoll(numberStr.c_str(), nullptr, 10);

						return (result < -INT_MAX || result > INT_MAX) ? ValueType::LONG : ValueType::INT;
					}
					else
					{
						u64 result = strtoull(numberStr.c_str(), nullptr, 10);

						return (result > INT_MAX) ? ValueType::ULONG : ValueType::UINT;
					}
				}
			}
		} return ValueType::STRING;
		}
	}

	JSONValue::JSONValue() :
		type(ValueType::UNINITIALIZED)
	{
	}

	JSONValue::JSONValue(const std::string& inStrValue) :
		type(ValueType::STRING),
		strValue(inStrValue)
	{
	}

	JSONValue::JSONValue(const char* inStrValue) :
		type(ValueType::STRING),
		strValue(inStrValue)
	{
	}

	JSONValue::JSONValue(i32 inIntValue) :
		type(ValueType::INT),
		intValue(inIntValue)
	{
		ENSURE(!IsNanOrInf((real)inIntValue));
	}

	JSONValue::JSONValue(u32 inUIntValue) :
		type(ValueType::UINT),
		uintValue(inUIntValue)
	{
	}

	JSONValue::JSONValue(i64 inLongValue) :
		type(ValueType::LONG),
		longValue(inLongValue)
	{
	}

	JSONValue::JSONValue(u64 inULongValue) :
		type(ValueType::ULONG),
		ulongValue(inULongValue)
	{
	}

	JSONValue::JSONValue(real inFloatValue) :
		JSONValue(inFloatValue, DEFAULT_FLOAT_PRECISION)
	{
	}

	JSONValue::JSONValue(real inFloatValue, u32 inFloatPrecision) :
		type(ValueType::FLOAT),
		floatValue(inFloatValue),
		floatPrecision(inFloatPrecision)
	{
		ENSURE(!IsNanOrInf(inFloatValue));
	}

	JSONValue::JSONValue(bool inBoolValue) :
		type(ValueType::BOOL),
		uintValue(inBoolValue ? 1 : 0)
	{
	}

	JSONValue::JSONValue(const glm::vec2& inVec2Value, u32 inFloatPrecision /* = DEFAULT_FLOAT_PRECISION */) :
		type(ValueType::VEC2),
		vecValue(inVec2Value, 0.0f, 0.0f),
		floatPrecision(inFloatPrecision)
	{
	}

	JSONValue::JSONValue(const glm::vec3& inVec3Value, u32 inFloatPrecision /* = DEFAULT_FLOAT_PRECISION */) :
		type(ValueType::VEC3),
		vecValue(inVec3Value, 0.0f),
		floatPrecision(inFloatPrecision)
	{
	}

	JSONValue::JSONValue(const glm::vec4& inVec4Value, u32 inFloatPrecision /* = DEFAULT_FLOAT_PRECISION */) :
		type(ValueType::VEC4),
		vecValue(inVec4Value),
		floatPrecision(inFloatPrecision)
	{
	}

	JSONValue::JSONValue(const glm::quat& inQuatValue, u32 inFloatPrecision /* = DEFAULT_FLOAT_PRECISION */) :
		type(ValueType::QUAT),
		vecValue(inQuatValue.x, inQuatValue.y, inQuatValue.z, inQuatValue.w),
		floatPrecision(inFloatPrecision)
	{
	}

	JSONValue::JSONValue(const glm::mat4& inMatValue, u32 inFloatPrecision /* = DEFAULT_FLOAT_PRECISION */) :
		type(ValueType::MAT4),
		matValue(inMatValue),
		floatPrecision(inFloatPrecision)
	{
	}

	JSONValue::JSONValue(const GUID& inGUIDValue) :
		type(ValueType::GUID),
		guidValue(inGUIDValue)
	{
	}

	JSONValue::JSONValue(const JSONObject& inObjectValue) :
		type(ValueType::OBJECT),
		objectValue(inObjectValue)
	{
	}

	JSONValue::JSONValue(const std::vector<JSONObject>& inObjectArrayValue) :
		type(ValueType::OBJECT_ARRAY),
		objectArrayValue(inObjectArrayValue)
	{
	}

	JSONValue::JSONValue(const std::vector<JSONField>& inFieldArrayValue) :
		type(ValueType::FIELD_ARRAY),
		fieldArrayValue(inFieldArrayValue)
	{
	}

	JSONValue JSONValue::FromRawPtr(void* valuePtr, ValueType type, u32 precision /* = DEFAULT_FLOAT_PRECISION */)
	{
		switch (type)
		{
		case ValueType::STRING:
			return JSONValue(*(std::string*)valuePtr);
		case ValueType::INT:
			return JSONValue(*(i32*)valuePtr);
		case ValueType::UINT:
			return JSONValue(*(u32*)valuePtr);
		case ValueType::LONG:
			return JSONValue(*(i64*)valuePtr);
		case ValueType::ULONG:
			return JSONValue(*(u64*)valuePtr);
		case ValueType::FLOAT:
			return JSONValue(*(real*)valuePtr, precision);
		case ValueType::BOOL:
			return JSONValue(*(bool*)valuePtr);
		case ValueType::VEC2:
			return JSONValue(*(glm::vec2*)valuePtr, precision);
		case ValueType::VEC3:
			return JSONValue(*(glm::vec3*)valuePtr, precision);
		case ValueType::VEC4:
			return JSONValue(*(glm::vec4*)valuePtr, precision);
		case ValueType::QUAT:
			return JSONValue(*(glm::quat*)valuePtr, precision);
		case ValueType::MAT4:
			return JSONValue(*(glm::mat4*)valuePtr, precision);
		case ValueType::GUID:
			return JSONValue(*(GUID*)valuePtr);
		case ValueType::OBJECT:
			return JSONValue(*(JSONObject*)valuePtr);
		case ValueType::OBJECT_ARRAY:
			return JSONValue(*(std::vector<JSONObject>*)valuePtr);
		case ValueType::FIELD_ARRAY:
			return JSONValue(*(std::vector<JSONField>*)valuePtr);
		default:
			PrintError("FromRawPtr was called with invalid type\n");
			return JSONValue();
		}
	}

	bool JSONValue::operator!=(const JSONValue& other)
	{
		return !(*this == other);
	}

	bool JSONValue::operator==(const JSONValue& other)
	{
		switch (type)
		{
		case ValueType::STRING:
			return AsString() == other.AsString();
		case ValueType::INT:
			return AsInt() == other.AsInt();
		case ValueType::UINT:
			return AsUInt() == other.AsUInt();
		case ValueType::LONG:
			return AsLong() == other.AsLong();
		case ValueType::ULONG:
			return AsULong() == other.AsULong();
		case ValueType::FLOAT:
			return AsFloat() == other.AsFloat();
		case ValueType::BOOL:
			return AsBool() == other.AsBool();
		case ValueType::VEC2:
			return AsVec2() == other.AsVec2();
		case ValueType::VEC3:
			return AsVec3() == other.AsVec3();
		case ValueType::VEC4:
			return AsVec4() == other.AsVec4();
		case ValueType::QUAT:
			return AsQuat() == other.AsQuat();
		case ValueType::MAT4:
			return AsMat4() == other.AsMat4();
		case ValueType::GUID:
			return AsGUID() == other.AsGUID();
		case ValueType::OBJECT:
			return objectValue == other.objectValue;
		case ValueType::OBJECT_ARRAY:
		{
			if (objectArrayValue.size() != other.objectArrayValue.size())
			{
				return false;
			}

			for (u32 i = 0; i < (u32)objectArrayValue.size(); ++i)
			{
				if (objectArrayValue[i] != other.objectArrayValue[i])
				{
					return false;
				}
			}

			return true;
		}
		case ValueType::FIELD_ARRAY:
		{
			if (fieldArrayValue.size() != other.fieldArrayValue.size())
			{
				return false;
			}

			for (u32 i = 0; i < (u32)fieldArrayValue.size(); ++i)
			{
				if (fieldArrayValue[i] != other.fieldArrayValue[i])
				{
					return false;
				}
			}

			return true;
		}
		default:
			PrintError("JSONValue::operator== was called with invalid type\n");
			return false;
		}
	}

	i32 JSONValue::AsInt() const
	{
		switch (type)
		{
		case ValueType::INT:
			return intValue;
		case ValueType::UINT:
			return (i32)uintValue;
		case ValueType::LONG:
			return (i32)longValue;
		case ValueType::ULONG:
			return (i32)ulongValue;
		case ValueType::FLOAT:
			return (i32)floatValue;
		case ValueType::BOOL:
			return (i32)uintValue;
		default:
			PrintError("AsInt was called on non-integer value\n");
			return i32_max;
		}
	}

	u32 JSONValue::AsUInt() const
	{
		switch (type)
		{
		case ValueType::INT:
			return (u32)intValue;
		case ValueType::UINT:
			return uintValue;
		case ValueType::LONG:
			return (u32)longValue;
		case ValueType::ULONG:
			return (u32)ulongValue;
		case ValueType::FLOAT:
			return (u32)floatValue;
		case ValueType::BOOL:
			return uintValue;
		default:
			PrintError("AsUInt was called on non-integer value\n");
			return u32_max;
		}
	}

	i64 JSONValue::AsLong() const
	{
		switch (type)
		{
		case ValueType::INT:
			return (i64)intValue;
		case ValueType::UINT:
			return (i64)uintValue;
		case ValueType::LONG:
			return longValue;
		case ValueType::ULONG:
			return (i64)ulongValue;
		case ValueType::FLOAT:
			return (i64)floatValue;
		case ValueType::BOOL:
			return (i64)uintValue;
		default:
			PrintError("AsLong was called on non-integer value\n");
			return -1;
		}
	}

	u64 JSONValue::AsULong() const
	{
		switch (type)
		{
		case ValueType::INT:
			return (u64)intValue;
		case ValueType::UINT:
			return (u64)uintValue;
		case ValueType::LONG:
			return (u64)longValue;
		case ValueType::ULONG:
			return ulongValue;
		case ValueType::FLOAT:
			return (u64)floatValue;
		case ValueType::BOOL:
			return (u64)uintValue;
		default:
			PrintError("AsULong was called on non-integer value\n");
			return u64_max;
		}
	}

	real JSONValue::AsFloat() const
	{
		switch (type)
		{
		case ValueType::INT:
			return (real)intValue;
		case ValueType::UINT:
			return (real)uintValue;
		case ValueType::LONG:
			return (real)longValue;
		case ValueType::ULONG:
			return (real)ulongValue;
		case ValueType::FLOAT:
			return floatValue;
		case ValueType::BOOL:
			return (uintValue != 0) ? 1.0f : 0.0f;
		default:
			PrintError("AsFloat was called on non-floating point value\n");
			return -1.0f;
		}
	}

	bool JSONValue::AsBool() const
	{
		switch (type)
		{
		case ValueType::INT:
			return (intValue != 0);
		case ValueType::UINT:
			return (uintValue != 0);
		case ValueType::LONG:
			return (longValue != 0);
		case ValueType::ULONG:
			return (ulongValue != 0);
		case ValueType::FLOAT:
			return (floatValue != 0.0f);
		case ValueType::BOOL:
			return (uintValue != 0);
		default:
			PrintError("AsBool was called on non-bool value\n");
			return false;
		}
	}

	std::string JSONValue::AsString() const
	{
		switch (type)
		{
		case ValueType::STRING:
			return strValue;
		default:
			PrintError("AsString was called on non-string value\n");
			return EMPTY_STRING;
		}
	}

	glm::vec2 JSONValue::AsVec2() const
	{
		switch (type)
		{
		case ValueType::VEC2:
		case ValueType::VEC3:
		case ValueType::VEC4:
			return (glm::vec2)vecValue;
		default:
			PrintError("AsVec2 was called on non-vec value\n");
			return VEC2_ZERO;
		}
	}

	glm::vec3 JSONValue::AsVec3() const
	{
		switch (type)
		{
		case ValueType::VEC2:
		case ValueType::VEC3:
		case ValueType::VEC4:
			return (glm::vec3)vecValue;
		default:
			PrintError("AsVec3 was called on non-vec value\n");
			return VEC3_ZERO;
		}
	}

	glm::vec4 JSONValue::AsVec4() const
	{
		switch (type)
		{
		case ValueType::VEC2:
		case ValueType::VEC3:
		case ValueType::VEC4:
			return (glm::vec4)vecValue;
		default:
			PrintError("AsVec4 was called on non-vec value\n");
			return VEC4_ZERO;
		}
	}

	glm::mat4 JSONValue::AsMat4() const
	{
		switch (type)
		{
		case ValueType::MAT4:
			return matValue;
		default:
			PrintError("AsMat4 was called on non-matrix value\n");
			return MAT4_IDENTITY;
		}
	}

	glm::quat JSONValue::AsQuat() const
	{
		switch (type)
		{
		case ValueType::QUAT:
			return (glm::quat)vecValue;
		default:
			PrintError("AsQuat was called on non-quat value\n");
			return QUAT_IDENTITY;
		}
	}

	GUID JSONValue::AsGUID() const
	{
		switch (type)
		{
		case ValueType::GUID:
			return guidValue;
		default:
			PrintError("AsGUID was called on non-GUID value\n");
			return InvalidGUID;
		}
	}

	bool JSONObject::HasField(const std::string& label) const
	{
		PROFILE_AUTO("JSONObject HasField");

		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return true;
			}
		}
		return false;
	}

	std::string JSONObject::GetString(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return field.value.strValue;
			}
		}
		return "";
	}

	bool JSONObject::TryGetString(const std::string& label, std::string& value) const
	{
		// TODO: PERFORMANCE: Return index of found item to avoid duplicate looping
		if (HasField(label))
		{
			value = GetString(label);
			return true;
		}
		return false;
	}

	StringID JSONObject::GetStringID(const std::string& label) const
	{
		if (HasField(label))
		{
			u64 result = GetULong(label);
			return (StringID)result;
		}
		return InvalidStringID;
	}

	bool JSONObject::TryGetStringID(const std::string& label, StringID& value) const
	{
		if (HasField(label))
		{
			value = (StringID)GetULong(label);
			return true;
		}
		return false;
	}

	glm::vec2 JSONObject::GetVec2(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return (glm::vec2)field.value.vecValue;
			}
		}
		return VEC2_ZERO;
	}

	bool JSONObject::TryGetVec2(const std::string& label, glm::vec2& value) const
	{
		if (HasField(label))
		{
			value = GetVec2(label);
			return true;
		}
		return false;
	}

	glm::vec3 JSONObject::GetVec3(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return (glm::vec3)field.value.vecValue;
			}
		}
		return VEC3_ZERO;
	}

	bool JSONObject::TryGetVec3(const std::string& label, glm::vec3& value) const
	{
		if (HasField(label))
		{
			value = GetVec3(label);
			return true;
		}
		return false;
	}

	glm::vec4 JSONObject::GetVec4(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return field.value.vecValue;
			}
		}
		return VEC4_ZERO;
	}

	bool JSONObject::TryGetVec4(const std::string& label, glm::vec4& value) const
	{
		if (HasField(label))
		{
			value = GetVec4(label);
			return true;
		}
		return false;
	}

	glm::quat JSONObject::GetQuat(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return (glm::quat)field.value.vecValue;
			}
		}
		return QUAT_IDENTITY;
	}

	bool JSONObject::TryGetQuat(const std::string& label, glm::quat& value) const
	{
		if (HasField(label))
		{
			value = GetQuat(label);
			return true;
		}
		return false;
	}

	glm::mat4 JSONObject::GetMat4(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return field.value.matValue;
			}
		}
		return MAT4_IDENTITY;
	}

	bool JSONObject::TryGetMat4(const std::string& label, glm::mat4& value) const
	{
		if (HasField(label))
		{
			value = GetMat4(label);
			return true;
		}
		return false;
	}

	i32 JSONObject::GetInt(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return field.value.intValue;
			}
		}
		return 0;
	}

	bool JSONObject::TryGetInt(const std::string& label, i32& value) const
	{
		if (HasField(label))
		{
			value = GetInt(label);
			return true;
		}
		return false;
	}

	u32 JSONObject::GetUInt(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return field.value.uintValue;
			}
		}
		return 0;
	}

	bool JSONObject::TryGetUInt(const std::string& label, u32& value) const
	{
		if (HasField(label))
		{
			value = GetUInt(label);
			return true;
		}
		return false;
	}

	i64 JSONObject::GetLong(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return field.value.longValue;
			}
		}
		return 0;
	}

	bool JSONObject::TryGetLong(const std::string& label, i64& value) const
	{
		if (HasField(label))
		{
			value = GetLong(label);
			return true;
		}
		return false;
	}

	u64 JSONObject::GetULong(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				// TODO: Call AsULong here
				return field.value.ulongValue;
			}
		}
		return 0;
	}

	bool JSONObject::TryGetULong(const std::string& label, u64& value) const
	{
		if (HasField(label))
		{
			value = GetULong(label);
			return true;
		}
		return false;
	}

	real JSONObject::GetFloat(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				// A float might be written without a decimal, making the system think it's an int
				if (field.value.floatValue != 0.0f)
				{
					ENSURE(!IsNanOrInf(field.value.floatValue));
					return field.value.floatValue;
				}
				ENSURE(!IsNanOrInf((real)field.value.intValue));
				return (real)field.value.intValue;
			}
		}
		return 0.0f;
	}

	bool JSONObject::TryGetFloat(const std::string& label, real& value) const
	{
		if (HasField(label))
		{
			value = GetFloat(label);
			ENSURE(!IsNanOrInf(value));
			return true;
		}
		return false;
	}

	bool JSONObject::GetBool(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return field.value.uintValue;
			}
		}
		return false;
	}

	bool JSONObject::TryGetBool(const std::string& label, bool& value) const
	{
		if (HasField(label))
		{
			value = GetBool(label);
			return true;
		}
		return false;
	}

	GUID JSONObject::GetGUID(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return GUID::FromString(field.value.strValue);
			}
		}
		return InvalidGUID;
	}

	bool JSONObject::TryGetGUID(const std::string& label, GUID& value) const
	{
		if (HasField(label))
		{
			value = GetGUID(label);
			return true;
		}
		return false;
	}

	GameObjectID JSONObject::GetGameObjectID(const std::string& label) const
	{
		GUID guid = GetGUID(label);
		return *(GameObjectID*)&guid;
	}

	bool JSONObject::TryGetGameObjectID(const std::string& label, GameObjectID& value) const
	{
		return TryGetGUID(label, *(GUID*)&value);
	}

	PrefabID JSONObject::GetPrefabID(const std::string& label) const
	{
		GUID guid = GetGUID(label);
		return *(PrefabID*)&guid;
	}

	bool JSONObject::TryGetPrefabID(const std::string& label, PrefabID& value) const
	{
		return TryGetGUID(label, *(GUID*)&value);
	}

	const std::vector<JSONField>& JSONObject::GetFieldArray(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return field.value.fieldArrayValue;
			}
		}
		return s_EmptyFieldArray;
	}

	bool JSONObject::TryGetFieldArray(const std::string& label, std::vector<JSONField>& value) const
	{
		if (HasField(label))
		{
			value = GetFieldArray(label);
			return true;
		}
		return false;
	}

	const std::vector<JSONObject>& JSONObject::GetObjectArray(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return field.value.objectArrayValue;
			}
		}
		return s_EmptyObjectArray;
	}

	bool JSONObject::TryGetObjectArray(const std::string& label, std::vector<JSONObject>& value) const
	{
		if (HasField(label))
		{
			value = GetObjectArray(label);
			return true;
		}
		return false;
	}

	const JSONObject& JSONObject::GetObject(const std::string& label) const
	{
		for (const JSONField& field : fields)
		{
			if (field.label == label)
			{
				return field.value.objectValue;
			}
		}
		return s_EmptyObject;
	}

	bool JSONObject::TryGetObject(const std::string& label, JSONObject& value) const
	{
		if (HasField(label))
		{
			value = GetObject(label);
			return true;
		}
		return false;
	}

	bool JSONObject::TryGetValueOfType(const char* label, void* valuePtr, ValueType type) const
	{
		switch (type)
		{
		case ValueType::STRING:
			return TryGetString(label, *(std::string*)valuePtr);
		case ValueType::INT:
			return TryGetInt(label, *(i32*)valuePtr);
		case ValueType::UINT:
			return TryGetUInt(label, *(u32*)valuePtr);
		case ValueType::LONG:
			return TryGetLong(label, *(i64*)valuePtr);
		case ValueType::ULONG:
			return TryGetULong(label, *(u64*)valuePtr);
		case ValueType::FLOAT:
			return TryGetFloat(label, *(real*)valuePtr);
		case ValueType::BOOL:
			return TryGetBool(label, *(bool*)valuePtr);
		case ValueType::VEC2:
			return TryGetVec2(label, *(glm::vec2*)valuePtr);
		case ValueType::VEC3:
			return TryGetVec3(label, *(glm::vec3*)valuePtr);
		case ValueType::VEC4:
			return TryGetVec4(label, *(glm::vec4*)valuePtr);
		case ValueType::QUAT:
			return TryGetQuat(label, *(glm::quat*)valuePtr);
		case ValueType::MAT4:
			return TryGetMat4(label, *(glm::mat4*)valuePtr);
		case ValueType::GUID:
			return TryGetGUID(label, *(GUID*)valuePtr);
		default:
			PrintError("Unhandled type passed to JSONObject::TryGetValueOfType\n");
			return false;
		}
	}


	JSONField::JSONField()
	{
	}

	JSONField::JSONField(const std::string& label, const JSONValue& value) :
		label(label),
		value(value)
	{
	}

	bool JSONField::operator!=(const JSONField& other)
	{
		return !(*this == other);
	}

	bool JSONField::operator==(const JSONField& other)
	{
		return label == other.label && value == other.value;
	}

	std::string JSONField::ToString(i32 tabCount) const
	{
		const std::string tabs(tabCount, '\t');

		// TODO: Use StringBuilder
		std::string result(tabs);

		if (!label.empty())
		{
			result += '\"' + label + "\" : ";
		}

		switch (value.type)
		{
		case ValueType::STRING:
			result += '\"' + value.strValue + '\"';
			break;
		case ValueType::INT:
			result += IntToString(value.intValue);
			break;
		case ValueType::UINT:
			result += UIntToString(value.uintValue);
			break;
		case ValueType::LONG:
			result += LongToString(value.longValue);
			break;
		case ValueType::ULONG:
			result += ULongToString(value.ulongValue);
			break;
		case ValueType::FLOAT:
			result += FloatToString(value.floatValue, value.floatPrecision);
			break;
		case ValueType::BOOL:
			result += (value.uintValue ? "true" : "false");
			break;
		case ValueType::VEC2:
			result += "\"";
			result += FloatToString(value.vecValue[0], value.floatPrecision);
			result += ",";
			result += FloatToString(value.vecValue[1], value.floatPrecision);
			result += "\"";
			break;
		case ValueType::VEC3:
			result += "\"";
			result += FloatToString(value.vecValue[0], value.floatPrecision);
			result += ",";
			result += FloatToString(value.vecValue[1], value.floatPrecision);
			result += ",";
			result += FloatToString(value.vecValue[2], value.floatPrecision);
			result += "\"";
			break;
		case ValueType::VEC4:
			result += "\"";
			result += FloatToString(value.vecValue[0], value.floatPrecision);
			result += ",";
			result += FloatToString(value.vecValue[1], value.floatPrecision);
			result += ",";
			result += FloatToString(value.vecValue[2], value.floatPrecision);
			result += ",";
			result += FloatToString(value.vecValue[3], value.floatPrecision);
			result += "\"";
			break;
		case ValueType::QUAT:
			result += "\"";
			result += FloatToString(value.vecValue[0], value.floatPrecision);
			result += ",";
			result += FloatToString(value.vecValue[1], value.floatPrecision);
			result += ",";
			result += FloatToString(value.vecValue[2], value.floatPrecision);
			result += ",";
			result += FloatToString(value.vecValue[3], value.floatPrecision);
			result += "\"";
			break;
		case ValueType::MAT4:
			result += "\"(";
			result += FloatToString(value.matValue[0][0], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[0][1], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[0][2], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[0][3], value.floatPrecision);
			result += "), (";

			result += FloatToString(value.matValue[1][0], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[1][1], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[1][2], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[1][3], value.floatPrecision);
			result += "), (";

			result += FloatToString(value.matValue[2][0], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[2][1], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[2][2], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[2][3], value.floatPrecision);
			result += "), (";

			result += FloatToString(value.matValue[3][0], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[3][1], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[3][2], value.floatPrecision);
			result += ",";
			result += FloatToString(value.matValue[3][3], value.floatPrecision);
			result += ")\"";
			break;
		case ValueType::GUID:
			result += "\"";
			result += value.guidValue.ToString();
			result += "\"";
			break;
		case ValueType::OBJECT:
			result += '\n' + tabs + "{\n";
			for (u32 i = 0; i < value.objectValue.fields.size(); ++i)
			{
				result += value.objectValue.fields[i].ToString(tabCount + 1);
				if (i != value.objectValue.fields.size() - 1)
				{
					result += ",\n";
				}
				else
				{
					result += '\n';
				}
			}
			result += tabs + "}";
			break;
		case ValueType::OBJECT_ARRAY:
			result += '\n' + tabs + "[\n";
			for (u32 i = 0; i < value.objectArrayValue.size(); ++i)
			{
				result += value.objectArrayValue[i].ToString(tabCount + 1);
				if (i != value.objectArrayValue.size() - 1)
				{
					result += ",\n";
				}
				else
				{
					result += '\n';
				}
			}
			result += tabs + "]";
			break;
		case ValueType::FIELD_ARRAY:
			result += '\n' + tabs + "[\n";
			for (u32 i = 0; i < value.fieldArrayValue.size(); ++i)
			{
				result += tabs + "\t" + value.fieldArrayValue[i].ToString(0);

				if (i != value.fieldArrayValue.size() - 1)
				{
					result += ",\n";
				}
				else
				{
					result += '\n';
				}
			}
			result += tabs + "]";
			break;
		case ValueType::UNINITIALIZED:
			result += "UNINITIALIZED TYPE\n";
			DEBUG_BREAK();
			PrintError("Uninitialized type in JSONField::ToString()\n");
			break;
		default:
			result += "UNHANDLED TYPE\n";
			DEBUG_BREAK();
			PrintError("Unhandled type in JSONField::ToString()\n");
			break;
		}

		return result;
	}

	std::string JSONObject::ToString(i32 tabCount /* = 0 */) const
	{
		// TODO: Use StringBuilder in PrintFunctions
		const std::string tabs(tabCount, '\t');
		std::string result(tabs + "{\n");

		for (u32 i = 0; i < fields.size(); ++i)
		{
			result += fields[i].ToString(tabCount + 1);
			if (i != fields.size() - 1)
			{
				result += ",\n";
			}
			else
			{
				result += '\n';
			}
		}

		result += tabs + "}";

		return result;
	}

	bool JSONObject::operator!=(const JSONObject& other)
	{
		return !(*this == other);
	}

	bool JSONObject::operator==(const JSONObject& other)
	{
		if (fields.size() != other.fields.size())
		{
			return false;
		}

		for (u32 i = 0; i < (u32)fields.size(); ++i)
		{
			if (fields[i] != other.fields[i])
			{
				return false;
			}
		}

		return true;
	}

	bool DrawImGuiForValueType(void* valuePtr, const char* label, ValueType type, bool valueMinSet, bool valueMaxSet, void* valueMin, void* valueMax)
	{
		bool bValueChanged = false;

		switch (type)
		{
		case ValueType::FLOAT:
		{
			if (valueMinSet != 0 && valueMaxSet != 0)
			{
				bValueChanged = ImGui::SliderFloat(label, (real*)valuePtr, *(real*)&valueMin, *(real*)&valueMax) || bValueChanged;
			}
			else
			{
				bValueChanged = ImGui::DragFloat(label, (real*)valuePtr) || bValueChanged;
			}
		} break;
		case ValueType::INT:
		{
			if (valueMinSet != 0 && valueMaxSet != 0)
			{
				bValueChanged = ImGui::SliderInt(label, (i32*)valuePtr, *(i32*)&valueMin, *(i32*)&valueMax) || bValueChanged;
			}
			else
			{
				bValueChanged = ImGui::DragInt(label, (i32*)valuePtr) || bValueChanged;
			}
		} break;
		case ValueType::UINT:
		{
			if (valueMinSet != 0 && valueMaxSet != 0)
			{
				bValueChanged = ImGuiExt::SliderUInt(label, (u32*)valuePtr, *(u32*)&valueMin, *(u32*)&valueMax) || bValueChanged;
			}
			else
			{
				bValueChanged = ImGuiExt::DragUInt(label, (u32*)valuePtr) || bValueChanged;
			}
		} break;
		case ValueType::BOOL:
		{
			bValueChanged = ImGui::Checkbox(label, (bool*)valuePtr) || bValueChanged;
		} break;
		case ValueType::VEC2:
		{
			if (valueMinSet != 0 && valueMaxSet != 0)
			{
				bValueChanged = ImGui::SliderFloat2(label, &((glm::vec2*)valuePtr)->x, *(real*)&valueMin, *(real*)&valueMax) || bValueChanged;
			}
			else
			{
				bValueChanged = ImGui::DragFloat2(label, &((glm::vec2*)valuePtr)->x) || bValueChanged;
			}
		} break;
		case ValueType::VEC3:
		{
			if (valueMinSet != 0 && valueMaxSet != 0)
			{
				bValueChanged = ImGui::SliderFloat3(label, &((glm::vec3*)valuePtr)->x, *(real*)&valueMin, *(real*)&valueMax) || bValueChanged;
			}
			else
			{
				bValueChanged = ImGui::DragFloat3(label, &((glm::vec3*)valuePtr)->x) || bValueChanged;
			}
		} break;
		case ValueType::VEC4:
		{
			if (valueMinSet != 0 && valueMaxSet != 0)
			{
				bValueChanged = ImGui::SliderFloat4(label, &((glm::vec4*)valuePtr)->x, *(real*)&valueMin, *(real*)&valueMax) || bValueChanged;
			}
			else
			{
				bValueChanged = ImGui::DragFloat4(label, &((glm::vec4*)valuePtr)->x) || bValueChanged;
			}
		} break;
		case ValueType::QUAT:
		{
			glm::vec3 rotEuler = glm::eulerAngles(*(glm::quat*)valuePtr);

			real valueMinReal = -TWO_PI;
			real valueMaxReal = TWO_PI;
			if (valueMinSet != 0 && valueMaxSet != 0)
			{
				valueMinReal = *(real*)&valueMin;
				valueMaxReal = *(real*)&valueMax;
			}

			if (ImGui::SliderFloat3(label, &rotEuler.x, valueMinReal, valueMaxReal))
			{
				*(glm::quat*)valuePtr = glm::quat(rotEuler);
				bValueChanged = true;
			}
		} break;
		case ValueType::STRING:
		{
			ImGui::Text("%s: %s", label, ((std::string*)valuePtr)->c_str());
		} break;
		case ValueType::GUID:
		{
			std::string guidStr = ((GUID*)valuePtr)->ToString();
			ImGui::Text("%s: %s", label, guidStr.c_str());
		} break;
		default:
		{
			PrintError("Unhandled value type in DrawImGuiForValueType: %u\n", (u32)type);
		} break;
		}

		return bValueChanged;
	}
} // namespace flex
