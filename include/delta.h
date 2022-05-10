#ifndef _DELTA_RULE_H
#define _DELTA_RULE_H
/*
 * Fledge Delta class
 *
 * Copyright (c) 2019 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */
#include <plugin.h>
#include <plugin_manager.h>
#include <config_category.h>
#include <rule_plugin.h>
#include <builtin_rule.h>
#include <map>
#include <string>

//#include <rapidjson/writer.h>
//#include <rapidjson/stringbuffer.h>

/**
 * Delta class, derived from Notification BuiltinRule
 */
class DeltaRule: public BuiltinRule
{
	public:
		DeltaRule();
		~DeltaRule();

		void	configure(const ConfigCategory& config);
		void	lockConfig() { m_configMutex.lock(); };
		void	unlockConfig() { m_configMutex.unlock(); };


		bool	evaluate(const std::string& asset, const std::string& datapoint, const rapidjson::Value& value);
		bool	chosenDatapoint(const std::string& datapoint);
		const std::string getJsonActionObject();
		void setJsonActionObject(const std::string& jsonString);


	private:
		std::mutex	m_configMutex;
		std::map<std::string, rapidjson::Document *> m_lastvalue;
		std::string actionJsonObject;
		std::vector<std::string> datapointNames;
		std::string datapointJsonString;
		const std::string seralizeJson(const rapidjson::Document* doc);
		void generateJsonActionObject(const std::string& asset, const std::string& datapoint, const std::string& newValue, const std::string& lastValue="");
		const std::string generateJsonActionItem(const std::string& asset, const std::string& datapoint, const std::string& newValue, const std::string& lastValue="");
		void appendJsonActionItem(const std::string& actionJson);
		std::string escape_json(const std::string& s);
		void getDatapointNamesConfig();
		const std::string getAliasNameConfig(const std::string& datapointName);
};

#endif
