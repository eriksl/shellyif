#pragma once

#include <string>
#include <vector>
#include <exception>
#include <boost/format.hpp>

class ShellyIf
{
	public:

		struct Data
		{
			time_t time;
			std::string type;
			std::string host_name;
			std::string host_alias;
			double power;
			double voltage;
			double current;
			double temperature;
		};

		class Exception : public std::exception
		{
			public:

				Exception() = delete;
				Exception(const std::string &what);
				Exception(const char *what);
				Exception(const boost::format &what);

				const char *what() const noexcept;

			private:

				const std::string what_string;
		};

		ShellyIf();
		ShellyIf(const ShellyIf &) = delete;
		~ShellyIf();

		const Data &fetch(const std::vector<std::string> &);
		const Data &fetch(int argc, const char * const *argv);
		const Data &fetch(const std::string &);

	private:

		static constexpr const char *dbus_service_id = "name.slagter.erik.shellyif";

		class InternalException : public std::exception
		{
			public:

				InternalException() = delete;
				InternalException(const std::string &what);
				InternalException(const char *what);
				InternalException(const boost::format &what);

				const char *what() const noexcept;

			private:

				const std::string what_string;
		};

		class ProxyThread
		{
			public:

				ProxyThread(ShellyIf &, bool debug, const std::string &hostname);
				__attribute__((noreturn)) void operator ()();

			private:

				ShellyIf &shellyif;
				bool debug;
				const std::string &hostname;
		};

		void fetch_all(const std::vector<std::string> &);

		__attribute__((noreturn)) void run_proxy(const std::string &host, bool legacy, bool debug);

		void fetch_legacy(const std::string &host);
		void fetch_modern(const std::string &host);

		Data data;
		ProxyThread *proxy_thread_class;
};
