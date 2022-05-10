/**
 * Fledge Delta notification rule plugin
 *
 * Copyright (c) 2021 ACDP
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Sebastian Kropatschek, Thorsten Steuer
 */

/***********************************************************************
* DISCLAIMER:
*
* All sample code is provided by ACDP for illustrative purposes only.
* These examples have not been thoroughly tested under all conditions.
* ACDP provides no guarantee nor implies any reliability,
* serviceability, or function of these programs.
* ALL PROGRAMS CONTAINED HEREIN ARE PROVIDED TO YOU "AS IS"
* WITHOUT ANY WARRANTIES OF ANY KIND. ALL WARRANTIES INCLUDING
* THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
* AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY DISCLAIMED.
************************************************************************/

#include <plugin_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <logger.h>
#include <plugin_exception.h>
#include <iostream>
#include <config_category.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <builtin_rule.h>
#include "version.h"
#include "delta.h"
//#include <boost/algorithm/string/trim.hpp>
#include <string_utils.h>

using namespace std;

/**
 * Delta rule constructor
 *
 * Call parent class BuiltinRule constructor
 */
DeltaRule::DeltaRule() : BuiltinRule()
{
}

/**
 * Delta destructor
 */
DeltaRule::~DeltaRule()
{
	for(std::map<std::string, rapidjson::Document *>::iterator itr = m_lastvalue.begin(); itr != m_lastvalue.end(); itr++)
	{
		delete (itr->second);
		itr->second = NULL;
	}
	m_lastvalue.clear();
}

/**
 * Configure the rule plugin
 *
 * @param    config	The configuration object to process
 */
void DeltaRule::configure(const ConfigCategory& config)
{
	// Remove current triggers
	// Configuration change is protected by a lock
	lockConfig();
	if (hasTriggers())
	{
		removeTriggers();
	}
	// Release lock
	unlockConfig();

	std::string assetName = config.getValue("asset");

	addTrigger(assetName, NULL);

	datapointJsonString = config.getValue("datapoints");
	getDatapointNamesConfig();
	// for (string elem: datapointNames){
	// 	Logger::getLogger()->debug(elem.c_str());
	// }

}

bool DeltaRule::evaluate(const std::string& asset, const std::string& datapoint, const rapidjson::Value& value)
{
	std::map<std::string, rapidjson::Document *>::iterator it;

	rapidjson::Document* newvalue = new rapidjson::Document();
	newvalue->CopyFrom(value, newvalue->GetAllocator());
	std::string newValueString = seralizeJson(newvalue);

	if ((it = m_lastvalue.find(datapoint)) == m_lastvalue.end())
	{
		//Logger::getLogger()->debug("VALUE INITIALIZING");
		m_lastvalue.insert(std::pair<std::string, rapidjson::Document *>(datapoint, newvalue));
		if(this->actionJsonObject.empty())
		{
			generateJsonActionObject(asset, datapoint, newValueString);
		}else{
			std::string jsonActionItem= generateJsonActionItem(asset, datapoint, newValueString);
			appendJsonActionItem(jsonActionItem);
		}
		return true;
	}

	bool rval = false;

	rapidjson::Document* lastvalue = it->second;
	it->second = newvalue;
	std::string lastValueString = seralizeJson(lastvalue);


	if (lastValueString == newValueString)
	{
		// Logger::getLogger()->debug("Values are equal. LastValue: %s NewValue: %s", lastValueString.c_str(), newValueString.c_str());
		if(this->actionJsonObject.empty())
		{
			generateJsonActionObject(asset, datapoint, newValueString, lastValueString);
		}else{
			std::string jsonActionItem= generateJsonActionItem(asset, datapoint, newValueString, lastValueString);
			appendJsonActionItem(jsonActionItem);
		}
		rval = false;
	}
	else
	{
		// Logger::getLogger()->debug("Values are NOT equal. LastValue: %s NewValue: %s", lastValueString.c_str(), newValueString.c_str());
		if(this->actionJsonObject.empty())
		{
			generateJsonActionObject(asset, datapoint, newValueString, lastValueString);
		}else{
			std::string jsonActionItem= generateJsonActionItem(asset, datapoint, newValueString, lastValueString);
			appendJsonActionItem(jsonActionItem);
		}
		rval = true;
	}

	if (lastvalue != NULL)
	{
		delete lastvalue;
		lastvalue = NULL;
	}

	if (rval)
	{
		Logger::getLogger()->info("%s.%s triggered",
						asset.c_str(),
						datapoint.c_str()
						);
	}
	return rval;
}

const std::string DeltaRule::seralizeJson(const rapidjson::Document* doc)
{
	if(doc)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<StringBuffer> writer(buffer);
		doc->Accept(writer);

		std::string jsonString = buffer.GetString();
		return jsonString;

	}else
	{
		return "{}";
	}

}

bool DeltaRule::chosenDatapoint(const std::string& datapoint)
{
	if(datapointNames.empty())
	{
		Logger::getLogger()->warn("No datapoints have been submitted all datapoints in the asset will be considered");
		return true;
	}

	if(std::find(datapointNames.begin(), datapointNames.end(), datapoint) != datapointNames.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}


void DeltaRule::generateJsonActionObject(const std::string& asset, const std::string& datapoint, const std::string& newValue, const std::string& lastValue)
{
	const std::string new_datapoint = getAliasNameConfig(datapoint);
	if(lastValue.empty())
	{
		this->actionJsonObject = "{\"" + asset + "\": {\"" + new_datapoint + "\":  { \"lastValue\": " + "null" + ", \"value\": " + newValue + "}}}";
	}else{
		this->actionJsonObject = "{\"" + asset + "\": {\"" + new_datapoint + "\":  { \"lastValue\": " + lastValue + ", \"value\": " + newValue + "}}}";
	}
}

const std::string DeltaRule::generateJsonActionItem(const std::string& asset, const std::string& datapoint, const std::string& newValue, const std::string& lastValue)
{
	const std::string new_datapoint = getAliasNameConfig(datapoint);
	if(lastValue.empty())
	{
		std::string actionJsonItem = "\"" + new_datapoint + "\": { \"lastValue\": " + "null" + ", \"value\": " + newValue + "}}";
		return actionJsonItem;
	}else{
		std::string actionJsonItem = "\"" + new_datapoint + "\": { \"lastValue\": " + lastValue + ", \"value\": " + newValue + "}}";
		return actionJsonItem;
	}
}

void DeltaRule::appendJsonActionItem(const std::string& actionJsonItem)
{
	//Logger::getLogger()->debug("Append Item %s", actionJsonItem.c_str());
	//Delete last two characters to append item to current object
	this->actionJsonObject.pop_back();
	this->actionJsonObject.pop_back();
	this->actionJsonObject += ", ";
	this->actionJsonObject += actionJsonItem;
	this->actionJsonObject += "}";
}

const std::string DeltaRule::getJsonActionObject()
{
	// return escape_json(actionJsonObject);
	return actionJsonObject;
}

void DeltaRule::setJsonActionObject(const std::string& jsonString)
{
	this->actionJsonObject = jsonString;
}

std::string DeltaRule::escape_json(const std::string &s)
{
	std::ostringstream o;
	for (auto c = s.cbegin(); c != s.cend(); c++)
	{
		switch (*c)
		{
			case '"': o << "\\\""; break;
			case '\\': o << "\\\\"; break;
			case '\b': o << "\\b"; break;
			case '\f': o << "\\f"; break;
			case '\n': o << "\\n"; break;
			case '\r': o << "\\r"; break;
			case '\t': o << "\\t"; break;
			default:
				if ('\x00' <= *c && *c <= '\x1f') {
					o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
				}
				else
				{
					o << *c;
				}
		}
	}
	return o.str();
}

void DeltaRule::getDatapointNamesConfig(){
	Document configDoc;
	configDoc.Parse(datapointJsonString.c_str());

	for (Value::ConstMemberIterator itr = 	configDoc.MemberBegin();itr != 	configDoc.MemberEnd(); ++itr){
			datapointNames.push_back(itr->name.GetString());
	}
}

const std::string DeltaRule::getAliasNameConfig(const std::string& datapointName){
	std::string alias_name;
	Document configDoc;
	configDoc.Parse(datapointJsonString.c_str());

	for (Value::ConstMemberIterator itr = 	configDoc.MemberBegin();itr != 	configDoc.MemberEnd(); ++itr)
  {
		if (datapointName==itr->name.GetString()){
			if(itr->value.IsString()){
				alias_name = itr->value.GetString();
				if(alias_name.empty() == true){
					alias_name=itr->name.GetString();
				}
			}else{
				Logger::getLogger()->info("Please submit a String as alias name");
			}
		}
	}
	return alias_name;
}
