// Copyright (c) 2012, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/flags.h"

#include "platform/assert.h"
#include "vm/os.h"

namespace dart {

DEFINE_FLAG(bool, print_flags, false, "Print flags as they are being parsed.");
DEFINE_FLAG(bool, ignore_unrecognized_flags, false,
    "Ignore unrecognized flags.");

// List of registered flags.
Flag* Flags::flags_ = NULL;

bool Flags::initialized_ = false;

class Flag {
 public:
  enum FlagType {
    kBoolean,
    kInteger,
    kString,
    kFunc,
    kNumFlagTypes
  };

  Flag(const char* name, const char* comment, void* addr, FlagType type)
      : name_(name), comment_(comment), addr_(addr), type_(type) {
  }
  Flag(const char* name, const char* comment, FlagHandler handler)
      : name_(name), comment_(comment), handler_(handler), type_(kFunc) {
  }

  void Print() {
    if (IsUnrecognized()) {
      OS::Print("%s: unrecognized\n", name_);
      return;
    }
    switch (type_) {
      case kBoolean: {
        OS::Print("%s: %s (%s)\n",
            name_, *this->bool_ptr_ ? "true" : "false", comment_);
        break;
      }
      case kInteger: {
        OS::Print("%s: %d (%s)\n", name_, *this->int_ptr_, comment_);
        break;
      }
      case kString: {
        if (*this->charp_ptr_ != NULL) {
          OS::Print("%s: '%s' (%s)\n", name_, *this->charp_ptr_, comment_);
        } else {
          OS::Print("%s: (null) (%s)\n", name_, comment_);
        }
        break;
      }
      case kFunc: {
        OS::Print("%s: (%s)\n", name_, comment_);
        break;
      }
      default:
        UNREACHABLE();
        break;
    }
  }

  bool IsUnrecognized() const {
    return (type_ == kBoolean) && (bool_ptr_ == NULL);
  }

  Flag* next_;
  const char* name_;
  const char* comment_;
  union {
    void* addr_;
    bool* bool_ptr_;
    int* int_ptr_;
    charp* charp_ptr_;
    FlagHandler handler_;
  };
  FlagType type_;
};


Flag* Flags::Lookup(const char* name) {
  Flag* cur = Flags::flags_;
  while (cur != NULL) {
    if (strcmp(cur->name_, name) == 0) {
      return cur;
    }
    cur = cur->next_;
  }
  return NULL;
}


bool Flags::Register_bool(bool* addr,
                          const char* name,
                          bool default_value,
                          const char* comment) {
  Flag* flag = Lookup(name);
  if (flag != NULL) {
    ASSERT(flag->IsUnrecognized());
    return default_value;
  }
  flag = new Flag(name, comment, addr, Flag::kBoolean);
  flag->next_ = Flags::flags_;
  Flags::flags_ = flag;

  return default_value;
}


int Flags::Register_int(int* addr,
                        const char* name,
                        int default_value,
                        const char* comment) {
  ASSERT(Lookup(name) == NULL);

  Flag* flag = new Flag(name, comment, addr, Flag::kInteger);
  flag->next_ = Flags::flags_;
  Flags::flags_ = flag;

  return default_value;
}


const char* Flags::Register_charp(charp* addr,
                                  const char* name,
                                  const char* default_value,
                                  const char* comment) {
  ASSERT(Lookup(name) == NULL);
  Flag* flag = new Flag(name, comment, addr, Flag::kString);
  flag->next_ = Flags::flags_;
  Flags::flags_ = flag;
  return default_value;
}


bool Flags::Register_func(FlagHandler handler,
                          const char* name,
                          const char* comment) {
  ASSERT(Lookup(name) == NULL);
  Flag* flag = new Flag(name, comment, handler);
  flag->next_ = Flags::flags_;
  Flags::flags_ = flag;
  return false;
}


static void Normalize(char* s) {
  intptr_t len = strlen(s);
  for (intptr_t i = 0; i < len; i++) {
    if (s[i] == '-') {
      s[i] = '_';
    }
  }
}


void Flags::Parse(const char* option) {
  // Find the beginning of the option argument, if it exists.
  const char* equals = option;
  while ((*equals != '\0') && (*equals != '=')) {
    equals++;
  }

  const char* argument = NULL;

  // Determine if this is an option argument.
  if (*equals != '=') {
    // No explicit option argument. Determine if there is a "no_" prefix
    // preceding the name.
    const char* kNo1Prefix = "no_";
    const char* kNo2Prefix = "no-";
    const intptr_t kNo1PrefixLen = strlen(kNo1Prefix);
    const intptr_t kNo2PrefixLen = strlen(kNo2Prefix);
    if (strncmp(option, kNo1Prefix, kNo1PrefixLen) == 0) {
      option += kNo1PrefixLen;  // Skip the "no_" when looking up the name.
      argument = "false";
    } else if (strncmp(option, kNo2Prefix, kNo2PrefixLen) == 0) {
      option += kNo2PrefixLen;  // Skip the "no-" when looking up the name.
      argument = "false";
    } else {
      argument = "true";
    }
  } else {
    // The argument for the option starts right after the equals sign.
    argument = equals + 1;
  }

  // Initialize the flag name.
  intptr_t name_len = equals - option;
  char* name = new char[name_len + 1];
  strncpy(name, option, name_len);
  name[name_len] = '\0';
  Normalize(name);

  Flag* flag = Flags::Lookup(name);
  if (flag == NULL) {
    // Collect unrecognized flags.
    char* new_flag = new char[name_len + 1];
    strncpy(new_flag, option, name_len);
    new_flag[name_len] = '\0';
    Flags::Register_bool(NULL, new_flag, true, NULL);
  } else {
    // Only set values for recognized flags, skip collected
    // unrecognized flags.
    if (!flag->IsUnrecognized()) {
      switch (flag->type_) {
        case Flag::kBoolean: {
          if (strcmp(argument, "true") == 0) {
            *flag->bool_ptr_ = true;
          } else if (strcmp(argument, "false") == 0) {
            *flag->bool_ptr_ = false;
          } else {
            OS::Print("Ignoring flag: %s is a bool flag.\n", name);
          }
          break;
        }
        case Flag::kString: {
          *flag->charp_ptr_ = argument == NULL ? NULL : strdup(argument);
          break;
        }
        case Flag::kInteger: {
          char* endptr = NULL;
          int val = strtol(argument, &endptr, 10);
          if (endptr != argument) {
            *flag->int_ptr_ = val;
          }
          break;
        }
        case Flag::kFunc: {
          if (strcmp(argument, "true") == 0) {
            (flag->handler_)(true);
          } else if (strcmp(argument, "false") == 0) {
            (flag->handler_)(false);
          } else {
            OS::Print("Ignoring flag: %s is a bool flag.\n", name);
          }
          break;
        }
        default: {
          UNREACHABLE();
          break;
        }
      }
    }
  }

  delete[] name;
}


static bool IsValidFlag(const char* name,
                        const char* prefix,
                        intptr_t prefix_length) {
  intptr_t name_length = strlen(name);
  return ((name_length > prefix_length) &&
          (strncmp(name, prefix, prefix_length) == 0));
}


bool Flags::ProcessCommandLineFlags(int number_of_vm_flags,
                                    const char** vm_flags) {
  if (initialized_) {
    return false;
  }

  initialized_ = true;

  const char* kPrefix = "--";
  const intptr_t kPrefixLen = strlen(kPrefix);

  int i = 0;
  while ((i < number_of_vm_flags) &&
         IsValidFlag(vm_flags[i], kPrefix, kPrefixLen)) {
    const char* option = vm_flags[i] + kPrefixLen;
    Parse(option);
    i++;
  }

  if (!FLAG_ignore_unrecognized_flags) {
    int unrecognized_count = 0;
    Flag* flag = Flags::flags_;
    while (flag != NULL) {
      if (flag->IsUnrecognized()) {
        if (unrecognized_count == 0) {
          OS::PrintErr("Unrecognized flags: %s", flag->name_);
        } else {
          OS::PrintErr(", %s", flag->name_);
        }
        unrecognized_count++;
      }
      flag = flag->next_;
    }
    if (unrecognized_count > 0) {
      OS::PrintErr("\n");
      exit(255);
    }
  }
  if (FLAG_print_flags) {
    PrintFlags();
  }
  return true;
}


int Flags::CompareFlagNames(const void* left, const void* right) {
  const Flag* left_flag = *reinterpret_cast<const Flag* const *>(left);
  const Flag* right_flag = *reinterpret_cast<const Flag* const *>(right);
  return strcmp(left_flag->name_, right_flag->name_);
}


void Flags::PrintFlags() {
    OS::Print("Flag settings:\n");
    Flag* flag = Flags::flags_;
    int num_flags = 0;
    while (flag != NULL) {
      num_flags++;
      flag = flag->next_;
    }
    Flag** flag_array = new Flag*[num_flags];
    flag = Flags::flags_;
    for (int i = 0; i < num_flags; ++i, flag = flag->next_) {
      flag_array[i] = flag;
    }

    qsort(flag_array, num_flags, sizeof flag_array[0], CompareFlagNames);

    for (int i = 0; i < num_flags; ++i) {
      flag_array[i]->Print();
    }
    delete[] flag_array;
  }
}  // namespace dart
