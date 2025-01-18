# shellyif
Successor to Shelly::IF for querying Shelly (Plus) Plugs.

The simple form just queries plugs and displays main data on stdout.

The more interesting form is the proxy, which frequently (every 10 seconds now) queries the plug and serves this data over the DBus. The proxy can be queried using any standard DBus interface tool or using DBusTinyClient from DBus::Tiny by me (shared library for C++ and XS bindings by SWIG for Perl).
