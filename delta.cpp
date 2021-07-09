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

	string assetName = config.getValue("asset");

	addTrigger(assetName, NULL);

	string datapointNames = config.getValue("datapoints");

	stringstream ss(datapointNames);
	while(ss.good())
	{
			string substr;
			std:getline( ss, substr, ',' );
			//boost::algorithm::trim(substr);
			substr = StringTrim(substr);
			if(!substr.empty())
			{
				m_datapointNames.push_back(substr);
			}
	}


}

bool DeltaRule::chosenDatapoint(const std::string& datapoint)
{
	if(m_datapointNames.empty())
	{
		Logger::getLogger()->warn("No datapoints have been submitted all datapoints in the asset will be considered");
		return true;
	}

	if(std::find(m_datapointNames.begin(), m_datapointNames.end(), datapoint) != m_datapointNames.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool DeltaRule::evaluate(const std::string& asset, const std::string& datapoint, const rapidjson::Value& value)
{
	std::map<std::string, rapidjson::Document *>::iterator it;

	rapidjson::Document* newvalue = new rapidjson::Document();
	newvalue->CopyFrom(value, newvalue->GetAllocator());
	std::string newValueString = seralizeJson(newvalue);

	if ((it = m_lastvalue.find(datapoint)) == m_lastvalue.end())
	{
		// Logger::getLogger()->debug("VALUE INITIALIZING");
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

void DeltaRule::generateJsonActionObject(const std::string& asset, const std::string& datapoint, const std::string& newValue, const std::string& lastValue)
{
	if(lastValue.empty())
	{
		this->actionJsonObject = "{\"" + asset + "\": [{\"" + datapoint + "\": { \"action\"  : { \"lastValue\": " + "null" + ", \"Value\": " + newValue + "}}}]}";
	}else{
		this->actionJsonObject = "{\"" + asset + "\": [{\"" + datapoint + "\": { \"action\"  : { \"lastValue\": " + lastValue + ", \"Value\": " + newValue + "}}}]}";
	}
}

const std::string DeltaRule::generateJsonActionItem(const std::string& asset, const std::string& datapoint, const std::string& newValue, const std::string& lastValue)
{
	if(lastValue.empty())
	{
		std::string actionJsonItem = "{\"" + datapoint + "\": { \"action\"  : { \"lastValue\": " + "null" + ", \"Value\": " + newValue + "}}}";
		return actionJsonItem;
	}else{
		std::string actionJsonItem = "{\"" + datapoint + "\": { \"action\"  : { \"lastValue\": " + lastValue + ", \"Value\": " + newValue + "}}}";
		return actionJsonItem;
	}
}

void DeltaRule::appendJsonActionItem(const std::string& actionJsonItem)
{
	// Logger::getLogger()->debug("Append Item %s", actionJsonItem.c_str());
	this->actionJsonObject.pop_back();
	this->actionJsonObject.pop_back();
	this->actionJsonObject += ", ";
	this->actionJsonObject += actionJsonItem;
	this->actionJsonObject += "]}";
}

const std::string DeltaRule::getJsonActionObject()
{
	return actionJsonObject;
}

void DeltaRule::setJsonActionObject(const std::string& jsonString)
{
	this->actionJsonObject = jsonString;
}
