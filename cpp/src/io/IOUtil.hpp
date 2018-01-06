/*
Copyright 2011-2017 Frederic Langlet
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
you may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _IOUtil_
#define _IOUtil_

#include <errno.h>
#include <sys/stat.h>
#include <direct.h>
#include "IOException.hpp"
#include "../types.hpp"
#include "../Error.hpp"

#ifdef _MSC_VER
#include "../msvc_dirent.hpp"
#else
#include <dirent.h>
#endif

using namespace std;

static void createFileList(string& target, vector<string>& files) THROW
{
    struct stat buffer;

    if (target[target.size()-1] == PATH_SEPARATOR)
        target = target.substr(0, target.size()-1);

    if (stat(target.c_str(), &buffer) != 0) {
        stringstream ss;
        ss << "Cannot access input file '" << target << "'";
        throw IOException(ss.str(), Error::ERR_OPEN_FILE);
    }

    if ((buffer.st_mode & S_IFREG) != 0) {
        // Target is regular file
        if (target[0] != '.')
           files.push_back(target);

        return;
    }

    if ((buffer.st_mode & S_IFDIR) == 0) {
        // Target is neither regular file nor directory
        stringstream ss;
        ss << "Invalid file type '" << target << "'";
        throw IOException(ss.str(), Error::ERR_OPEN_FILE);
    }

    bool isRecursive = (target.size() <= 2) || (target[target.size()-1] != '.') ||
               (target[target.size()-2] != PATH_SEPARATOR);

    if (isRecursive) {
       if (target[target.size()-1] != PATH_SEPARATOR) {
          stringstream ss;
          ss << target << PATH_SEPARATOR;
          target = ss.str();
       }
    } else {
       target = target.substr(0, target.size()-1);
    }

    DIR* dir = opendir(target.c_str());
    struct dirent* ent;

    if (dir != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            string fullpath = target + ent->d_name;

            if (stat(fullpath.c_str(), &buffer) != 0) {
                stringstream ss;
                ss << "Cannot access input file '" << target << ent->d_name << "'";
                throw IOException(ss.str(), Error::ERR_OPEN_FILE);
            }

            if (ent->d_name[0] != '.')
            {
               if ((buffer.st_mode & S_IFREG) != 0){
                   files.push_back(fullpath);
               }
               else if ((isRecursive) && ((buffer.st_mode & S_IFDIR) != 0)) {
                   createFileList(fullpath, files);
               }
            }
        }

        closedir(dir);
    }
    else {
        stringstream ss;
        ss << "Cannot read directory '" << target << "'";
        throw IOException(ss.str(), Error::ERR_READ_FILE);
    }
}


static int mkdirAll(const string& path) {
    errno = 0;

    for (uint i=0; i<path.size(); i++) {
        if (path[i] == PATH_SEPARATOR) {
            string curPath = path.substr(0, i);

            if (_mkdir(curPath.c_str()) != 0) {
                if (errno != EEXIST)
                    return -1; 
            }
        }
    }   

    if (_mkdir(path.c_str()) != 0) {
        if (errno != EEXIST)
            return -1; 
    }   

    return 0;
}

#endif