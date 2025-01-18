#include "shellyif.h"

ShellyIf::Exception::Exception(const std::string &what) : what_string(what)
{
}

ShellyIf::Exception::Exception(const char *what) : what_string(what)
{
}

ShellyIf::Exception::Exception(const boost::format &what) : what_string(what.str())
{
}

const char *ShellyIf::Exception::what() const noexcept
{
	return(what_string.c_str());
}

ShellyIf::InternalException::InternalException(const std::string &what) : what_string(what)
{
}

ShellyIf::InternalException::InternalException(const char *what) : what_string(what)
{
}

ShellyIf::InternalException::InternalException(const boost::format &what) : what_string(what.str())
{
}

const char *ShellyIf::InternalException::what() const noexcept
{
	return(what_string.c_str());
}
