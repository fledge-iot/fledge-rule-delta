/**
 * Fledge Delta notification rule plugin
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */

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
#include <boost/algorithm/string/trim.hpp>

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
	while( ss.good() )
	{
			string substr;
			std:getline( ss, substr, ',' );
			boost::algorithm::trim(substr);
			m_datapointNames.push_back(substr);
	}

}

bool DeltaRule::chosenDatapoint(const std::string& datapoint)
{
	if(m_datapointNames.empty())
	{
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

	if ((it = m_lastvalue.find(datapoint)) == m_lastvalue.end())
	{
		m_lastvalue.insert(std::pair<std::string, rapidjson::Document *>(datapoint, newvalue));
		Logger::getLogger()->warn("datapoint: %s.%s, initialization", asset.c_str(), datapoint.c_str());
		return true;
	}

	bool rval = false;

	rapidjson::Document* lastvalue = it->second;
	it->second = newvalue;

	if ((*lastvalue) == (*newvalue))
	{
		Logger::getLogger()->warn("datapoint: %s.%s, value is equal", asset.c_str(), datapoint.c_str());
		rval = false;
	}
	else
	{
		Logger::getLogger()->warn("datapoint: %s.%s, value is not equal -> trigger!", asset.c_str(), datapoint.c_str());
		rval = true;
	}

	if (lastvalue != NULL)
	{
		delete lastvalue;
		lastvalue = NULL;
	}

	if (rval)
	{
		Logger::getLogger()->warn("%s.%s triggered",
						asset.c_str(),
						datapoint.c_str()
						);
	}
	return rval;
}
