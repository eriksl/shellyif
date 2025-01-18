#include "shellyif.h"

#include <iostream>
#include <string>
#include <boost/format.hpp>

int main(int argc, const char **argv)
{
	try
	{
		ShellyIf shelly_if;
		ShellyIf::Data data;
		char timestring[64];
		const struct tm *tm;
		time_t timestamp;

		data = shelly_if.fetch(argc, argv);

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
	catch(const ShellyIf::Exception &e)
	{
		std::cerr << e.what() << std::endl;
		return(-1);
	}
	catch(...)
	{
		std::cerr << "unknown exception" << std::endl;
		return(-1);
	}

	return(0);
}
