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

#include "main.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <tnt/tntnet.h>
#include <tnt/httpreply.h>
#include <tnt/worker.h>
#include <cxxtools/loginit.h>
#include <cxxtools/arg.h>
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

zim::File articleFile;
zim::File indexFile;

int main(int argc, char* argv[])
{
  signal(SIGPIPE, SIG_IGN);
  signal(SIGABRT, SIG_IGN);

  try
  {
    log_init();

    cxxtools::Arg<std::string> listenIp(argc, argv, 'l', "0.0.0.0");
    cxxtools::Arg<unsigned short> port(argc, argv, 'p', 8080);
    cxxtools::Arg<std::string> indexFileName(argc, argv, 'x');
    cxxtools::Arg<bool> compression(argc, argv, 'z');

    if (argc != 2)
    {
      std::cout << "usage: " << argv[0] << " [options] zim-file\n"
                   "\n"
                   "options:\n"
                   "\t-l <ip>        listen ip (default 0.0.0.0)\n"
                   "\t-p <port>      listen port (default 8080)\n"
                   "\t-x <indexfile> full text index file name\n";
                   "\t-z             enable http compression\n";
      return -1;
    }

    articleFile = zim::File(argv[1]);
    indexFile = indexFileName.isSet() ? zim::File(indexFileName)
                                      : articleFile;

    if (!articleFile.good())
      throw std::runtime_error("articlefile not found");

    if (!indexFile.good())
      throw std::runtime_error("indexfile not found");

    tnt::Tntnet app;
    tnt::Worker::setEnableCompression(compression);
    tnt::HttpReply::setDefaultContentType("text/html; charset=UTF-8");

    std::cout << "IP " << listenIp.getValue() << " port " << port.getValue() << std::endl;
    app.listen(listenIp, port);

    app.mapUrl("^/$",                          "redirect");
    app.mapUrl("^/$",                          "index");
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

    std::cout << "Wikipedia ist jetzt unter http://localhost:" << port << "/ verfügbar\n"
                 "Die Einstellungen können unter $HOME/.ZimReader geändert werden" << std::endl;
    app.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

