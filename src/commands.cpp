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

#include <cstring>

#include "commands.h"
#include "configuration/configuration.h"
#include "services/tfsproxy.h"
#include "utils/filesys.h"

static void get_contents(const TfsProxy& tfs, const std::string& project, const std::string& path)
{
  auto files = tfs.GetPathInfo(project, path);
  for (const auto& file : files) {

    if (!file.IsFolder) {
      printf("Getting: %s\n", file.Path.c_str());
      tfs.GetDirectFile(file.Url);
      continue;
    }

    std::string::size_type last_slash = file.Path.rfind('/');
    std::string dir_name;
    if (last_slash != std::string::npos) {
      dir_name = file.Path.substr(last_slash + 1);
    }
    filesys::create_dir_then_change(dir_name);
    get_contents(tfs, project, file.Path);
    filesys::go_up();
  }
}

static void cmd_clone(const std::vector<std::string>& args)
{
  if (args.size() < 1) {
    fprintf(stderr, "You must specify an argument: tf get $/Folder1/Folder2/File.cs\n");
    return;
  }

  // Assume the first argument is the path we want to get, we will recursively
  // iterate through that tree and fetch each file.
  const std::string& path = args[0];
  std::string dest;
  if (args.size() > 1) {
    dest = args[1];
  }
  TfsProxy tfs(AppConfig.Get("tfs", "base_url"), "unused",
      AppConfig.Get("tfs", "username"), AppConfig.Get("tfs", "password"));
  std::string project = AppConfig.Get("tfs", "default_project");

  get_contents(tfs, project, path);
}

struct cmd_operation {
  const char *name;
  void (*operation)(const std::vector<std::string>& args);
};

cmd_operation operations[] = {
  { "clone", cmd_clone },
  { nullptr, nullptr }
};

bool execute_cmd(const char *cmd, char *argv[], int num_args)
{
  cmd_operation *p = operations;

  while (p->name != nullptr) {
    if (strcmp(cmd, p->name) == 0) {
      break;
    }
    p++;
  }

  if (p->operation == nullptr) {
    return false;
  }

  // Convert C style args to vector;
  std::vector<std::string> args;
  for (int i = 0; i < num_args; i++) {
    args.push_back(argv[i]);
  }
  p->operation(args);
  return true;
}
