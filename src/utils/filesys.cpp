// Copyright (c) 2020 Devn Smith <devin@devinsmith.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#endif

#include "utils/filesys.h"

namespace filesys {

static const char PATH_SEPERATOR = '/';

std::string get_home_directory()
{
  passwd* pw = getpwuid(getuid());
  if (pw == nullptr || pw->pw_dir == nullptr)
    return ""; // exception?

  return pw->pw_dir;
}

std::string get_config_path(const char *fname)
{
  std::string home_path = get_home_directory();
  if (!home_path.empty() && home_path.back() != PATH_SEPERATOR)
    home_path += PATH_SEPERATOR;
  return home_path + fname;
}

void create_dir_then_change(const std::string& dir)
{
  int rc = access(dir.c_str(), 6);
  if (rc == -1 && errno == ENOENT) {
#ifdef _WIN32
    mkdir(dir.c_str());
#else
    mkdir(dir.c_str(), 0700);
#endif
  }
  chdir(dir.c_str());
}

void go_up()
{
  chdir("..");
}

} // namespace utils

