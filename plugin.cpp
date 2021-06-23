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
#include <string>
#include <logger.h>
#include <plugin_exception.h>
#include <iostream>
#include <config_category.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <builtin_rule.h>
#include "version.h"
#include "delta.h"


#include <typeinfo>


#define RULE_NAME "Delta"
#define DEFAULT_TIME_INTERVAL	30



static const char *default_config = QUOTE({
	"description": {
		"description": "Trigger if the current value deviates from the last one",
		"type": "string",
		"default":  RULE_NAME,
		"readonly": "true"
	},
	"plugin": {
		"description": "Trigger if the current value deviates from the last one",
		"type": "string",
		"default":  RULE_NAME,
		"readonly": "true"
	},
	"asset": 	{
		"description" : "Asset to monitor",
		"type" : "string",
		"default" : "",
		"displayName" : "Asset",
		"order": "1"
	},
	"datapoints": 	{
		"description" : "Datapoint(s) to monitor. If this input field is empty, a change in any of the existing datapoints or a newly added one will triger",
		"type" : "string",
		"default" : "",
		"displayName" : "Datapoint(s)",
		"order": "2"
	}
});


using namespace std;

/**
 * The C plugin interface
 */
extern "C" {
/**
 * The C API rule information structure
 */
static PLUGIN_INFORMATION ruleInfo = {
	RULE_NAME,			// Name
	VERSION,			// Version
	0,				// Flags
	PLUGIN_TYPE_NOTIFICATION_RULE,	// Type
	"1.0.0",			// Interface version
	default_config			// Configuration
};

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	return &ruleInfo;
}

/**
 * Initialise rule objects based in configuration
 *
 * @param    config	The rule configuration category data.
 * @return		The rule handle.
 */
PLUGIN_HANDLE plugin_init(const ConfigCategory& config)
{

	DeltaRule *handle = new DeltaRule();
	handle->configure(config);

	return (PLUGIN_HANDLE)handle;
}

/**
 * Free rule resources
 */
void plugin_shutdown(PLUGIN_HANDLE handle)
{
	DeltaRule *rule = (DeltaRule *)handle;
	// Delete plugin handle
	delete rule;
}

/**
 * Return triggers JSON document
 *
 * @return	JSON string
 */
string plugin_triggers(PLUGIN_HANDLE handle)
{
	string ret;
	DeltaRule *rule = (DeltaRule *)handle;

	if (!rule)
	{
		ret = "{\"triggers\" : []}";
		return ret;
	}

	// Configuration fetch is protected by a lock
	rule->lockConfig();

	if (!rule->hasTriggers())
	{
		rule->unlockConfig();
		ret = "{\"triggers\" : []}";
		return ret;
	}

	ret = "{\"triggers\" : [ ";
	std::map<std::string, RuleTrigger *> triggers = rule->getTriggers();
	for (auto it = triggers.begin();
		  it != triggers.end();
		  ++it)
	{
		ret += "{ \"asset\"  : \"" + (*it).first + "\"";
		ret += " }";

		if (std::next(it, 1) != triggers.end())
		{
			ret += ", ";
		}
	}

	ret += " ] }";

	// Release lock
	rule->unlockConfig();

	return ret;
}

/**
 * Evaluate notification data received
 *
 * @param    assetValues	JSON string document
 *				with notification data.
 * @return			True if the rule was triggered,
 *				false otherwise.
 */
bool plugin_eval(PLUGIN_HANDLE handle,
		 const string& assetValues)
{
	rapidjson::Document doc;
	doc.Parse(assetValues.c_str());
	if (doc.HasParseError())
	{
		return false;
	}

	bool eval = false;
	DeltaRule *rule = (DeltaRule *)handle;
	map<std::string, RuleTrigger *>& triggers = rule->getTriggers();

	// Iterate throgh all configured assets
	// If we have multiple asset the evaluation result is
	// TRUE only if all assets checks returned true
	for (auto t = triggers.begin(); t != triggers.end(); ++t)
	{
		string assetName = t->first;
		string assetTimestamp = "timestamp_" + assetName;
		if (doc.HasMember(assetName.c_str()))
		{
			// Get all datapoints for assetName
			const rapidjson::Value& assetValue = doc[assetName.c_str()];

			for (rapidjson::Value::ConstMemberIterator itr = assetValue.MemberBegin();
					    itr != assetValue.MemberEnd(); ++itr)
			{			
				if(rule->chosenDatapoint(itr->name.GetString()))
				{
					eval |= rule->evaluate(assetName, itr->name.GetString(), itr->value);
				}
			}
			// Add evalution timestamp
			if (doc.HasMember(assetTimestamp.c_str()))
			{
				const rapidjson::Value& assetTime = doc[assetTimestamp.c_str()];
				double timestamp = assetTime.GetDouble();
				rule->setEvalTimestamp(timestamp);
			}
		}
	}

	// Set final state: true is any calls to evalaute() returned true
	rule->setState(eval);

	Logger::getLogger()->warn("assetValues %s, eval: %d", assetValues.c_str(), eval);

	return eval;
}

/**
 * Return rule trigger reason: trigger or clear the notification.
 *
 * @return	 A JSON string
 */
string plugin_reason(PLUGIN_HANDLE handle)
{
	DeltaRule* rule = (DeltaRule *)handle;
	BuiltinRule::TriggerInfo info;
	rule->getFullState(info);

	string ret = "{ \"reason\": \"";
	ret += info.getState() == BuiltinRule::StateTriggered ? "triggered" : "cleared";
	ret += "\"";
	ret += ", \"asset\": " + info.getAssets();
	if (rule->getEvalTimestamp())
	{
		ret += string(", \"timestamp\": \"") + info.getUTCTimestamp() + string("\"");
	}
	ret += " }";

	return ret;
}

/**
 * Call the reconfigure method in the plugin
 *
 * Not implemented yet
 *
 * @param    newConfig		The new configuration for the plugin
 */
void plugin_reconfigure(PLUGIN_HANDLE handle,
			const string& newConfig)
{

	DeltaRule* rule = (DeltaRule *)handle;
	ConfigCategory  config("new_outofbound", newConfig);
	rule->configure(config);
}

// End of extern "C"
};
