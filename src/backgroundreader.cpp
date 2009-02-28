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

#include <backgroundreader.h>
#include <set>
#include <cxxtools/log.h>
#include <unistd.h>

log_define("zim.backgroundreader")

namespace zim
{
  Backgroundreader::Backgroundreader(zim::Files files_)
    : files(files_),
      stopRunning(false)
  {
  }

  Backgroundreader::~Backgroundreader()
  {
    stopRunning = true;
  }

  zim::File Backgroundreader::getFile(char ns)
  {
    for (Files::iterator it = files.begin(); it != files.end(); ++it)
      if (it->second.hasNamespace(ns))
        return it->second;
    return zim::File();
  }

  void Backgroundreader::readUrls(UrlsType& urls, zim::File file)
  {
    cxxtools::MutexLock lock(mutex);

    // loop through all links
    while (!stopRunning
        && lastArticle.getUrl() == currentArticle.getUrl()
        && !urls.empty())
    {
      lock.unlock();

      char ns = urls.begin()->first;
      QUnicodeString url = urls.begin()->second;
      Article article = file.getArticle(ns, url);
      article.getData();
      urls.erase(std::make_pair(ns, url));

      lock.lock();

      cachedArticles[std::make_pair(ns, url)] = article;
    }
  }

  void Backgroundreader::readImages()
  {
    UrlsType urls;

    cxxtools::MutexLock lock(mutex);

    // look for links and check, if the article has been cached before

    log_debug("new article \"" << currentArticle.getUrl() << "\" requested - read images");
    urls.clear();

    std::string::size_type pos = 0;
    std::string data = currentArticle.getData();
    while (!stopRunning && (pos = data.find("img src=\"../wikipedia.images/I/", pos) ) != std::string::npos)
    {
      std::string::size_type pos2 = data.find_first_of("\"?", pos + 31);
      if (pos2 != std::string::npos)
      {
        QUnicodeString url(std::string(data, pos + 29, pos2 - pos - 29));
        log_debug("url \"" << url << "\" found");
        CachedArticlesType::const_iterator it = prevCachedArticles.find(std::make_pair('I', url));
        if (it == prevCachedArticles.end())
        {
          log_debug("will read image \"" << url << '"');
          urls.insert(std::make_pair('I', QUnicodeString(url)));
        }
        else
        {
          log_debug("image \"" << url << "\" already cached");
          cachedArticles.insert(*it);
        }
        pos = pos2;
      }
    }

    lock.unlock();
    readUrls(urls, getFile('I'));
  }

  void Backgroundreader::readLinks()
  {
    UrlsType urls;

    cxxtools::MutexLock lock(mutex);

    // look for links and check, if the article has been cached before

    log_debug("new article \"" << currentArticle.getUrl() << "\" requested - read links");

    std::string::size_type pos = 0;
    std::string data = currentArticle.getData();
    while (!stopRunning && (pos = data.find("href=\"A/", pos) ) != std::string::npos)
    {
      std::string::size_type pos2 = data.find('"', pos + 8);
      if (pos2 != std::string::npos)
      {
        QUnicodeString url(std::string(data, pos + 6, pos2 - pos - 6));
        log_debug("url \"" << url << "\" found");
        CachedArticlesType::const_iterator it = prevCachedArticles.find(std::make_pair('A', url));
        if (it == prevCachedArticles.end())
        {
          log_debug("will read article \"" << url << '"');
          urls.insert(std::make_pair('A', QUnicodeString(url)));
        }
        else
        {
          log_debug("article \"" << url << "\" already cached");
          cachedArticles.insert(*it);
        }
        pos = pos2;
      }
    }

    lock.unlock();
    readUrls(urls, files.getFirstFile('A'));
  }

  void Backgroundreader::run()
  {
    while (!stopRunning)
    {
      cxxtools::MutexLock lock(mutex);

      while (!stopRunning
          && lastArticle
          && currentArticle
          && lastArticle.getUrl() == currentArticle.getUrl())
        newArticle.wait(lock);

      currentArticle = lastArticle;
      prevCachedArticles = cachedArticles;
      cachedArticles.clear();

      lock.unlock();
      readImages();
      lock.lock();

      if (stopRunning)
        break;

      lock.unlock();
      sleep(2);
      lock.lock();

      if (!stopRunning && lastArticle.getUrl() == currentArticle.getUrl())
      {
        lock.unlock();
        readLinks();
      }
    }
  }

  zim::Article Backgroundreader::getArticle(char ns, const QUnicodeString& path)
  {
    cxxtools::MutexLock lock(mutex);

    CachedArticlesType::const_iterator it = cachedArticles.find(std::make_pair(ns, path));

    zim::Article article;

    if (it != cachedArticles.end())
    {
      log_debug("cached article \"" << path << "\" namespace " << ns << " found");
      article = it->second;
    }
    else
    {
      log_debug("no cached article \"" << path << "\" namespace " << ns << " found");
      article = files.getArticle(ns, path);
    }

    if (article && article.getLibraryMimeType() == zim::Dirent::zimMimeTextHtml)
    {
      log_debug("html article read - replace current file");

      article.getData();  // make sure, data is read

      lastArticle = article;
      newArticle.signal();
    }

    return article;
  }

  zim::Article Backgroundreader::getArticle(const std::string& fname, char ns, const QUnicodeString& path)
  {
    return fname.empty() ? getArticle(ns, path) : files.getArticle(fname, ns, path);
  }

}
