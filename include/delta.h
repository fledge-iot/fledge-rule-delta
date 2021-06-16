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

#define	MIN(x, y)	((x) < (y) ? (x) : (y))
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
		typedef enum { SMA, EMA } DeltaType;

	private:
		std::mutex	m_configMutex;
		long		m_deviation;
		std::string	m_direction;
		DeltaType	m_aveType;
		int		m_factor;
		class Deltas
		{
			public:
				Deltas() : m_delta(0), m_samples(0) {};
				~Deltas();
				void	addValue(double value)
					{
						m_samples++;
						int divisor = (m_type == DeltaRule::SMA ? m_samples
							: MIN(m_samples, m_factor));
						m_delta += ((value - m_delta) / divisor);
					};
				double	delta() { return m_delta; };
				void	setDeltaType(DeltaRule::DeltaType type, int factor)
					{
						m_type = type;
						m_factor = factor;
					}
			private:
				double	m_delta;
				int	m_samples;
				int	m_factor;
				DeltaRule::DeltaType
					m_type;
		};
		std::map<std::string, Deltas *>
				m_deltas;

		std::map<std::string, rapidjson::Document *>
				m_lastvalue;
};

#endif
