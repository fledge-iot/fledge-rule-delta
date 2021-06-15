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
	m_deviation = strtol(config.getValue("deviation").c_str(), NULL, 10);
	m_direction = config.getValue("direction");
	string aveType = config.getValue("deltaType");
	if (aveType.compare("Simple Moving Delta") == 0)
	{
		m_aveType = SMA;
	}
	else
	{
		m_aveType = EMA;
	}
	m_factor = strtol(config.getValue("factor").c_str(), NULL, 10);
	for (auto it = m_deltas.begin(); it != m_deltas.end(); it++)
	{
		it->second->setDeltaType(m_aveType, m_factor);
	}
}

/**
 * Evaluate a long value to see if the alert should trigger.
 * This will also build the value of the delta reading using this
 * value.
 *
 * @param asset		The asset name we are processing
 * @param datapoint	The data point we are processing
 * @param value		The value to consider
 * @return true if the value exceeds the defined percentage deviation
 */
bool DeltaRule::evaluate(const string& asset, const string& datapoint, long value)
{
	return evaluate(asset, datapoint, (double)value);
}


/**
 * Evaluate a double value to see if the alert should trigger.
 * This will also build the value of the delta reading using this
 * value.
 *
 * @param asset		The asset name we are processing
 * @param datapoint	The data point we are processing
 * @param value		The value to consider
 * @return true if the value exceeds the defined percentage deviation
 */
bool DeltaRule::evaluate(const string& asset, const string& datapoint, double value)
{
	map<string, Deltas *>::iterator it;

	if ((it = m_deltas.find(datapoint)) == m_deltas.end())
	{
		Deltas *ave = new Deltas();
		ave->addValue(value);
		ave->setDeltaType(m_aveType, m_factor);
		m_deltas.insert(pair<string, Deltas *>(datapoint, ave));
		return false;
	}
	double delta = it->second->delta();
	it->second->addValue(value);
	double deviation = ((value - delta) * 100) /delta;
	bool rval = false;
	if (m_direction.compare("Both") == 0)
	{
		rval = fabs(deviation) > m_deviation;
	}
	else if (m_direction.compare("Above Delta") == 0)
	{
		rval = deviation > m_deviation;
	}
	else if (m_direction.compare("Below Delta") == 0)
	{
		rval = -deviation > m_deviation;
	}
	if (rval)
	{
		Logger::getLogger()->warn("Deviation of %.1f%% in %s.%s  triggered alert, value is %.2f, delta is %.2f",
					  deviation,
					  asset.c_str(),
					  datapoint.c_str(),
					  value,
					  delta);
	}
	return rval;
}

bool DeltaRule::evaluate(const string& asset, const string& datapoint, const rapidjson::Value& value)
{
	map<string, double>::iterator it;

	if ((it = m_lastvalue.find(datapoint)) == m_lastvalue.end())
	{
		m_lastvalue.insert(pair<string, double>(datapoint, value.GetDouble()));
		Logger::getLogger()->warn("first insert");
		return false;
	}
	double lastvalue = it->second;
	it->second = value.GetDouble();

	map<string, rapidjson::Document *>::iterator it2;

	rapidjson::Document *new_value;
	new_value = new rapidjson::Document();
	new_value->CopyFrom(value, new_value->GetAllocator());

	if ((it2 = m_lv.find(datapoint)) == m_lv.end())
	{
		m_lv.insert(pair<string, rapidjson::Document *>(datapoint, new_value));
		Logger::getLogger()->warn("Document first insert ");
		return false;
	}

	rapidjson::Document& lv = *(it2->second);

	it2->second = new_value;

	if (lv[datapoint] == value)
	{
		Logger::getLogger()->warn("equal Value");
	}
	else
	{
		Logger::getLogger()->warn("not equal Value");
	}


	if (lastvalue == value.GetDouble())
	{
		Logger::getLogger()->warn("equal");
	}
	else
	{
		Logger::getLogger()->warn("not equal");
	}

	/*
	if (value.IsInt64())
	{
		Logger::getLogger()->warn("IsInt64");
	}
	else if (value.IsDouble())
	{
		Logger::getLogger()->warn("IsDouble");
	}
	else
	{
		Logger::getLogger()->warn("not working");
	}
  */

	bool rval = true;

	if (rval)
	{
		Logger::getLogger()->warn("%s.%s triggered, lastvalue: %f, value: %f",
						asset.c_str(),
						datapoint.c_str(),
						lastvalue,
						value.GetDouble()
						);
	}
	return rval;
}
