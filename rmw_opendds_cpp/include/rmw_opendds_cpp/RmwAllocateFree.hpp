// Copyright 2015 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_OPENDDS_CPP__RMWALLOCATEFREE_HPP_
#define RMW_OPENDDS_CPP__RMWALLOCATEFREE_HPP_

#include <rmw/allocators.h>
#include <rmw/error_handling.h>

#include <memory>

// RmwAllocateFree uses rmw_allocate and rmw_free to create and destroy objects.
// No throwing is allowed in these functions.
template<typename T>
class RmwAllocateFree
{
private:
  struct Deleter
  {
    void operator()(T* t) const
    {
      if (t) {
        try {
          t->~T();
        } catch(...) {
          RMW_SET_ERROR_MSG("RmwAllocateFree::Deleter: exception\n");
        }
        rmw_free(t);
      }
    }
  };

  static void* allocate()
  {
    try {
      return rmw_allocate(sizeof(T));
    } catch (...) {
      RMW_SET_ERROR_MSG("RmwAllocateFree::allocate: exception\n");
    }
    return nullptr;
  }

public:
  // for default constructor
  static T* create()
  {
    void* v = allocate();
    if (v) {
      T* t = nullptr;
      try {
        t = new (v) T();
      } catch (const std::exception& e) {
        RMW_SET_ERROR_MSG(e.what());
      } catch (...) {
        RMW_SET_ERROR_MSG("RmwAllocateFree::create: exception\n");
      }
      if (!t) { rmw_free(v); }
      return t;
    }
    return nullptr;
  }

  // for parameterized constructors
  template<typename ... Args>
  static T* create(Args ... args)
  {
    void* v = allocate();
    if (v) {
      T* t = nullptr;
      try {
        t = new (v) T(args ...);
      } catch (const std::exception& e) {
        RMW_SET_ERROR_MSG(e.what());
      } catch (...) {
        RMW_SET_ERROR_MSG("RmwAllocateFree::create: exception\n");
      }
      if (!t) { rmw_free(v); }
      return t;
    }
    return nullptr;
  }

  static void destroy(T*& t)
  {
    if (t) {
      Deleter()(t);
      t = nullptr;
    }
  }

  // managed create and destroy
  typedef std::unique_ptr<T, Deleter> Ptr;

  static Ptr createPtr()
  {
    return Ptr(create());
  }

  template<typename ... Args>
  static Ptr createPtr(Args ... args)
  {
    return Ptr(create(args ...));
  }
};

struct RmwStr
{
  static char * cpy(const char * str)
  {
    if (str) {
      char * t = reinterpret_cast<char *>(rmw_allocate(sizeof(char) * strlen(str) + 1));
      if (t) {
        memcpy(t, str, strlen(str) + 1);
        return t;
      }
    }
    return nullptr;
  }

  static void del(char *& str)
  {
    if (str) {
      rmw_free(str);
      str = nullptr;
    }
  }
  static void del(const char *& str)
  {
    del(const_cast<char *&>(str));
  }
};

#endif  // RMW_OPENDDS_CPP__RMWALLOCATEFREE_HPP_
