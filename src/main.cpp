/*
 * Copyright (C) 2007 Tommi Maekitalo
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include <iostream>
#include <fstream>
#include <algorithm>
#include <tnt/tntnet.h>
#include <tnt/httpreply.h>
#include <tnt/worker.h>
#include <tnt/tntconfig.h>
#include <cxxtools/loginit.h>
#include <cxxtools/arg.h>
#include <cxxtools/inifile.h>
#include <signal.h>

log_define("zim.webapp.main")

namespace
{
  bool isTrue(char ch)
  {
    const char trueValues[] = "tTyY1xX";
    return std::find(trueValues, trueValues + sizeof(trueValues), ch)
                != trueValues + sizeof(trueValues);
  }

  bool isTrue(const std::string& s)
  {
    return !s.empty() && isTrue(s.at(0));
  }
}

int main(int argc, char* argv[])
{
  signal(SIGPIPE, SIG_IGN);
  signal(SIGABRT, SIG_IGN);

  try
  {
    log_init();

    const char* HOME = getenv("HOME");
    std::string settingsfile;
    if (HOME)
    {
      settingsfile = HOME;
      settingsfile += '/';
    }
    settingsfile += ".ZimReader";

    cxxtools::IniFile settings;
    
    try
    {
      settings = cxxtools::IniFile(settingsfile.c_str());
    }
    catch (const std::exception& e)
    {
      std::ofstream out(settingsfile.c_str());
      out << "[ZimReader]\n"
             "port=8080\n"
             "localonly=1\n"
             "directory=.\n";
      out.close();
      settings = cxxtools::IniFile(settingsfile.c_str());
    }

    std::string listenIp = settings.getValue("ZimReader", "listen", "");

    if (listenIp.empty())
    {
      std::string localonly = settings.getValue("ZimReader", "localonly", "0");
      log_debug("localonly=<" << localonly << "> b:" << isTrue(localonly));
      listenIp = isTrue(localonly) ?  "127.0.0.1" : "0.0.0.0";
    }

    unsigned short port = settings.getValueT<unsigned short>("ZimReader", "port", 8080);

    std::string directory = settings.getValue("ZimReader", "directory", ".");

    tnt::Tntnet app;
    tnt::Worker::setEnableCompression(false);
    tnt::HttpReply::setDefaultContentType("text/html; charset=UTF-8");

    std::cout << "IP " << listenIp << " port " << port << std::endl;
    app.listen(listenIp, port);

    app.mapUrl("^/$",                          "redirect");
    app.mapUrl("^/!/([0-9]+)$",                "$1", "number");
    app.mapUrl("^/-/([^.]+)\\.(.*)$",          "$1_$2");
    app.mapUrl("^/~/([^.]+)$",                 "$1");
    app.mapUrl("^/~/([^.]+)\\.([^.]*)$",       "$1_$2");

    app.mapUrl("^/(.)/(.+.svg)$", "zimcomp")
       .setPathInfo("$2.png")
       .pushArg("$1");

    app.mapUrl("^/(.+)/(.)/(.+.svg)$", "zimcomp")
       .setPathInfo("$3.png")
       .pushArg("$1.zim")
       .pushArg("$2");

    app.mapUrl("^/(.)/(.+)$", "zimcomp")
       .setPathInfo("$2")
       .pushArg("$1");

    app.mapUrl("^/(.+)/(.)/(.+)$", "zimcomp")
       .setPathInfo("$3")
       .pushArg("$2")
       .pushArg("$1.zim");

    app.mapUrl(".*", "notfound");

    tnt::Tntconfig config;
    config.setConfigValue("ZimPath", directory);
    tnt::Comploader::configure(config);

    std::cout << "Wikipedia ist jetzt unter http://localhost:" << port << "/ verfügbar\n"
                 "Die Einstellungen können unter $HOME/.ZimReader geändert werden" << std::endl;
    app.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

