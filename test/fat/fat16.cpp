// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os>
#include <stdio.h>
#include <cassert>

#include <memdisk>

void Service::start()
{
  INFO("FAT16", "Running tests for FAT16");
  auto disk = fs::new_shared_memdisk();
  assert(disk);
  
  // verify that the size is indeed N sectors
  CHECKSERT(disk->dev().size() == 16500, "Disk size 16500 sectors");
  
  // which means that the disk can't be empty
  CHECKSERT(!disk->empty(), "Disk not empty");
  
  // auto-mount filesystem
  disk->mount(
  [disk] (fs::error_t err)
  {
    CHECKSERT(!err, "Filesystem auto-mounted");
    
    auto& fs = disk->fs();
    printf("\t\t%s filesystem\n", fs.name().c_str());
    
    auto vec = fs::new_shared_vector();
    err = fs.ls("/", vec);
    CHECKSERT(!err, "List root directory");
    
    CHECKSERT(vec->size() == 1, "Exactly one ent in root dir");
    
    auto& e = vec->at(0);
    CHECKSERT(e.is_file(), "Ent is a file");
    CHECKSERT(e.name() == "banana.txt", "Ents name is 'banana.txt'");
    
  });
  // re-mount on VBR1
  disk->mount(disk->VBR1,
  [disk] (fs::error_t err)
  {
    CHECKSERT(!err, "Filesystem mounted on VBR1");
    
    // verify that we can read file
    auto& fs = disk->fs();
    auto ent = fs.stat("/banana.txt");
    CHECKSERT(ent.is_valid(), "Stat file in root dir");
    CHECKSERT(ent.is_file(), "Entity is file");
    CHECKSERT(!ent.is_dir(), "Entity is not directory");
    CHECKSERT(ent.name() == "banana.txt", "Name is 'banana.txt'");
    
    // try reading banana-file
    auto buf = fs.read(ent, 0, ent.size);
    std::string banana((char*) buf.buffer.get(), buf.len);
    
    std::string internal_banana = 
    R"(     ____                           ___
    |  _ \  ___              _   _.' _ `.
 _  | [_) )' _ `._   _  ___ ! \ | | (_) |    _
|:;.|  _ <| (_) | \ | |' _ `|  \| |  _  |  .:;|
|   `.[_) )  _  |  \| | (_) |     | | | |.',..|
':.   `. /| | | |     |  _  | |\  | | |.' :;::'
 !::,   `-!_| | | |\  | | | | | \ !_!.'   ':;!
 !::;       ":;:!.!.\_!_!_!.!-'-':;:''    '''!
 ';:'        `::;::;'             ''     .,  .
   `:     .,.    `'    .::... .      .::;::;'
     `..:;::;:..      ::;::;:;:;,    :;::;'
       "-:;::;:;:      ':;::;:''     ;.-'
           ""`---...________...---'""
)";
    printf("%s\n", internal_banana.c_str());
    CHECKSERT(banana == internal_banana, "Correct banana #1");
    
    bool test = true;
    
    for (size_t i = 0; i < internal_banana.size(); i++)
    {
      // read one byte at a time
      buf = fs.read(ent, i, 1);
      
      // verify that it matches the same location in test-string
      test = ((char) buf.buffer.get()[0] == internal_banana[i]);
      if (!test)
      {
        printf("!! Random access read test failed on i = %u\n", i);
        break;
      }
    }
    CHECKSERT(test, "Validate random access sync read");
    
    buf = fs.readFile("/banana.txt");
    banana = std::string((char*) buf.buffer.get(), buf.len);
    CHECKSERT(banana == internal_banana, "Correct banana #2");
  });
  
  INFO("FAT16", "SUCCESS");
}
