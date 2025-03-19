#include "shellyif.h"

#include <dbus-tiny.h>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/json.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <string>
#include <sstream>
#include <iostream>

ShellyIf::ShellyIf()
{
	data.time = 0;
	data.power = 0;
	data.voltage = 0;
	data.current = 0;
	data.temperature = 0;
}

ShellyIf::~ShellyIf()
{
}

void ShellyIf::fetch_legacy(const std::string &host)
{
	try
	{
		curlpp::Cleanup myCleanup;
		curlpp::Easy handle1, handle2;
		std::stringstream ss;
		std::string config_data;
		std::string status_data;
		boost::json::parser json;
		boost::json::object object;

		handle1.setOpt(curlpp::options::Url((boost::format("http://%s/settings") % host).str()));
		handle2.setOpt(curlpp::options::Url((boost::format("http://%s/status") % host).str()));

		handle1.setOpt(curlpp::options::Timeout(5));
		handle2.setOpt(curlpp::options::Timeout(5));

		ss.clear();
		ss.str("");
		ss << handle1;
		config_data = ss.str();

		ss.clear();
		ss.str("");
		ss << handle2;
		status_data = ss.str();

		json.write(config_data);
		object = json.release().as_object();

		data.host_name = object["name"].as_string();
		data.type = "Plug-S";
		data.host_alias = object["relays"].as_array()[0].as_object()["name"].as_string();

		json.write(status_data);
		object = json.release().as_object();

		data.time = object["unixtime"].as_int64();
		data.power = object["meters"].as_array()[0].as_object()["power"].as_double();
		data.temperature = object["temperature"].as_double();
		data.voltage = 230;
		data.current = data.power / data.voltage;
	}
	catch(const boost::system::system_error &e)
	{
		throw(InternalException(boost::format("json: %s") % e.what()));
	}
	catch(const curlpp::RuntimeError &e)
	{
		throw(InternalException(boost::format("curl runtime: %s") % e.what()));
	}
	catch(const curlpp::LogicError &e)
	{
		throw(InternalException(boost::format("curl logic: %s") % e.what()));
	}
	catch(const InternalException &e)
	{
		throw(Exception(boost::format("error: %s") % e.what()));
	}
	catch(const std::exception &e)
	{
		throw(Exception(boost::format("std::exception: %s") % e.what()));
	}
}

void ShellyIf::fetch_modern(const std::string &host)
{
	try
	{
		curlpp::Cleanup myCleanup;
		curlpp::Easy handle1, handle2;
		std::stringstream ss;
		std::string config_data;
		std::string status_data;
		std::string key;
		boost::json::parser json;
		boost::json::object object, device;

		handle1.setOpt(curlpp::options::Url((boost::format("http://%s/rpc/Shelly.GetConfig") % host).str()));
		handle2.setOpt(curlpp::options::Url((boost::format("http://%s/rpc/Shelly.GetStatus") % host).str()));

		handle1.setOpt(curlpp::options::Timeout(5));
		handle2.setOpt(curlpp::options::Timeout(5));

		ss.clear();
		ss.str("");
		ss << handle1;
		config_data = ss.str();

		ss.clear();
		ss.str("");
		ss << handle2;
		status_data = ss.str();

		json.write(config_data);
		object = json.release().as_object();

		if(object.contains("switch:0"))
		{
			key = "switch:0";
			data.type = "Plug-Plus-S";
		}
		else
		{
			key = "pm1:0";
			data.type = "Plug-PM-Mini";
		}

		data.host_name = object["sys"].as_object()["device"].as_object()["name"].as_string();
		data.host_alias = object[key].as_object()["name"].as_string();

		json.write(status_data);
		object = json.release().as_object();
		device = object[key].as_object();

		data.time = object["sys"].as_object()["unixtime"].as_int64();
		data.power = device["apower"].as_double();

		if(device.contains("temperature"))
			data.temperature = device["temperature"].as_object()["tC"].as_double();
		else
			data.temperature = 0;

		data.voltage = device["voltage"].as_double();
		data.current = device["current"].as_double();
	}
	catch(const boost::system::system_error &e)
	{
		throw(InternalException(boost::format("json: %s") % e.what()));
	}
	catch(const curlpp::RuntimeError &e)
	{
		throw(InternalException(boost::format("curl runtime: %s") % e.what()));
	}
	catch(const curlpp::LogicError &e)
	{
		throw(InternalException(boost::format("curl logic: %s") % e.what()));
	}
	catch(const InternalException &e)
	{
		throw(Exception(boost::format("error: %s") % e.what()));
	}
	catch(const std::exception &e)
	{
		throw(Exception(boost::format("std::exception: %s") % e.what()));
	}
}

const ShellyIf::Data& ShellyIf::fetch(const std::vector<std::string> &args)
{
	fetch_all(args);

	return(data);
}

const ShellyIf::Data& ShellyIf::fetch(int argc, const char * const *argv)
{
	int ix;
	std::vector<std::string> args;

	for(ix = 1; ix < argc; ix++)
		args.push_back(std::string(argv[ix]));

	fetch_all(args);

	return(data);
}

const ShellyIf::Data& ShellyIf::fetch(const std::string &args)
{
	std::vector<std::string> args_split;

	args_split = boost::program_options::split_unix(args);

	fetch_all(args_split);

	return(data);
}

void ShellyIf::fetch_all(const std::vector<std::string> &argv)
{
	boost::program_options::options_description main_options("usage");
	boost::program_options::positional_options_description positional_options;

	try
	{
		bool legacy;
		bool proxy;
		bool debug;
		std::string host;

		positional_options.add("host", -1);
		main_options.add_options()
			("host,h",		boost::program_options::value<std::string>(&host)->required(),		"plug to connect to")
			("legacy,l",	boost::program_options::bool_switch(&legacy)->implicit_value(true),	"use legacy interface (for non-\"plus\" type devices)")
			("proxy,p",		boost::program_options::bool_switch(&proxy)->implicit_value(true),	"run proxy")
			("debug,d",		boost::program_options::bool_switch(&debug)->implicit_value(true),	"show fetched information from proxy");

		boost::program_options::variables_map varmap;
		boost::program_options::store(boost::program_options::command_line_parser(argv).options(main_options).positional(positional_options).run(), varmap);
		boost::program_options::notify(varmap);

		if(proxy)
			run_proxy(host, legacy, debug);
		else
		{
			if(legacy)
				fetch_legacy(host);
			else
				fetch_modern(host);
		}
	}
	catch(const boost::program_options::required_option &e)
	{
		throw(Exception(boost::format("%s\n%s") % e.what() % main_options));
	}
	catch(const boost::program_options::error &e)
	{
		throw(Exception(boost::format("%s\n%s") % e.what() % main_options));
	}
	catch(const InternalException &e)
	{
		throw(Exception(boost::format("internal error: %s") % e.what()));
	}
	catch(const std::exception &e)
	{
		throw(Exception(boost::format("std::exception: %s") % e.what()));
	}
	catch(const std::string &e)
	{
		throw(Exception(boost::format("std::string exception: %s") % e));
	}
	catch(const char *e)
	{
		throw(Exception(boost::format("unknown charptr exception: %s") % e));
	}
	catch(...)
	{
		throw(Exception(boost::format("unknown exception")));
	}
}

ShellyIf::ProxyThread::ProxyThread(ShellyIf &shellyif_in, bool debug_in, const std::string &hostname_in) : shellyif(shellyif_in), debug(debug_in), hostname(hostname_in)
{
}

void ShellyIf::ProxyThread::operator()()
{
	std::string message_type;
	std::string message_interface;
	std::string message_method;
	std::string error;
	std::string reply;
	std::string time_string;
	std::string service = (boost::format("%s.%s") % dbus_service_id % hostname).str();

	try
	{
		DbusTinyServer dbus_tiny_server(service);

		for(;;)
		{
			try
			{
				dbus_tiny_server.get_message(message_type, message_interface, message_method);

				if(message_type == "method call")
				{
					std::cerr << boost::format("message received, interface: %s, method: %s\n") % message_interface % message_method;

					if(message_interface == "org.freedesktop.DBus.Introspectable")
					{
						if(message_method == "Introspect")
						{
							reply += std::string("") +
										"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n" +
										"<node>\n" +
										"	<interface name=\"org.freedesktop.DBus.Introspectable\">\n" +
										"		<method name=\"Introspect\">\n" +
										"			<arg name=\"xml\" type=\"s\" direction=\"out\"/>\n" +
										"		</method>\n" +
										"	</interface>\n" +
										"	<interface name=\"" + service + "\">\n" +
										"		<method name=\"dump\">\n" +
										"			<arg name=\"info\" type=\"s\" direction=\"out\"/>\n" +
										"		</method>\n" +
										"		<method name=\"get_data\">\n" +
										"			<arg name=\"time\" type=\"t\" direction=\"out\"/>\n" +
										"			<arg name=\"type\" type=\"s\" direction=\"out\"/>\n" +
										"			<arg name=\"host_name\" type=\"s\" direction=\"out\"/>\n" +
										"			<arg name=\"host_alias\" type=\"s\" direction=\"out\"/>\n" +
										"			<arg name=\"power\" type=\"d\" direction=\"out\"/>\n" +
										"			<arg name=\"voltage\" type=\"d\" direction=\"out\"/>\n" +
										"			<arg name=\"current\" type=\"d\" direction=\"out\"/>\n" +
										"			<arg name=\"temperature\" type=\"d\" direction=\"out\"/>\n" +
										"		</method>\n" +
										"	</interface>\n" +
										"</node>\n";

							dbus_tiny_server.send_string(reply);

							reply.clear();
						}
						else
							throw(InternalException(dbus_tiny_server.inform_error(std::string("unknown introspection method called"))));
					}
					else
					{
						if((message_interface == dbus_service_id) || (message_interface == ""))
						{
							if(message_method == "dump")
							{
								char timestring[64];
								const struct tm *tm;
								time_t timestamp;

								timestamp = shellyif.data.time;
								tm = localtime(&timestamp);
								strftime(timestring, sizeof(timestring), "%Y-%m-%d %H:%M:%S", tm);

								reply = (boost::format("%-16s %-13s %-19s %5s %8s %7s %11s %-4s\n") %
										"host" % "type" % "alias" % "power" % "voltage" % "current" % "temperature" % "time").str();

								reply += (boost::format("%-16s %-13s %-19s %5.1f %8.1f %7.1f %11.1f %s\n") %
										shellyif.data.host_name %
										shellyif.data.type %
										shellyif.data.host_alias %
										shellyif.data.power %
										shellyif.data.voltage %
										shellyif.data.current %
										shellyif.data.temperature %
										timestring).str();

								dbus_tiny_server.send_string(reply);
							}
							else
							{
								if(message_method == "get_data")
								{
									dbus_tiny_server.send_uint64_x3string_x4double(shellyif.data.time,
											shellyif.data.type, shellyif.data.host_name, shellyif.data.host_alias,
											shellyif.data.power, shellyif.data.voltage, shellyif.data.current, shellyif.data.temperature);
								}
								else
									throw(InternalException(dbus_tiny_server.inform_error(std::string("unknown method called"))));
							}
						}
						else
							throw(InternalException(dbus_tiny_server.inform_error((boost::format("message not for our interface: %s") % message_interface).str())));
					}
				}
				else if(message_type == "signal")
				{
					if(debug)
						std::cerr << boost::format("signal received, interface: %s, method: %s\n") % message_interface % message_method;

					if((message_interface == "org.freedesktop.DBus") && (message_method == "NameAcquired"))
					{
						if(debug)
							std::cerr << "name on dbus acquired\n";
					}
					else
						throw(InternalException(boost::format("message of unknown type: %u") % message_type));
				}
			}
			catch(const InternalException &e)
			{
				std::cerr << boost::format("warning: %s\n") % e.what();
			}

			dbus_tiny_server.reset();
		}
	}
	catch(const DbusTinyException &e)
	{
		std::cerr << "shellyif proxy: fatal: " << e.what() << std::endl;
		exit(1);
	}
	catch(...)
	{
		std::cerr << "shellyif proxy: unknown fatal exception" << std::endl;
		exit(1);
	}
}

void ShellyIf::run_proxy(const std::string &host, bool legacy, bool debug)
{
	proxy_thread_class = new ProxyThread(*this, debug, host);
	boost::thread proxy_thread(*proxy_thread_class);
	proxy_thread.detach();

	for(;;)
	{
		try
		{
			if(legacy)
				fetch_legacy(host);
			else
				fetch_modern(host);

			if(debug)
			{
				char timestring[64];
				const struct tm *tm;
				time_t timestamp;

				timestamp = data.time;
				tm = localtime(&timestamp);
				strftime(timestring, sizeof(timestring), "%Y-%m-%d %H:%M:%S", tm);

				std::cout << boost::format("%-16s %-13s %-19s %5s %8s %7s %11s %-4s\n") %
						"host" % "type" % "alias" % "power" % "voltage" % "current" % "temperature" % "time";

				std::cout << boost::format("%-16s %-13s %-19s %5.1f %8.1f %7.1f %11.1f %s\n") %
						data.host_name %
						data.type %
						data.host_alias %
						data.power %
						data.voltage %
						data.current %
						data.temperature %
						timestring;
			}

			boost::this_thread::sleep_for(boost::chrono::duration<unsigned int>(10));
		}
		catch(const InternalException &e)
		{
			std::cerr << "warning: " << e.what() << std::endl;
			continue;
		}
	}
}
