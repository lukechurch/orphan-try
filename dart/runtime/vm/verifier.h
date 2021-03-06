// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_VERIFIER_H_
#define VM_VERIFIER_H_

#include "vm/flags.h"
#include "vm/globals.h"
#include "vm/handles.h"
#include "vm/visitor.h"

namespace dart {

// Forward declarations.
class Isolate;
class ObjectSet;
class RawObject;

class VerifyObjectVisitor : public ObjectVisitor {
 public:
  VerifyObjectVisitor(Isolate* isolate, ObjectSet* allocated_set)
      : ObjectVisitor(isolate),
        allocated_set_(allocated_set) {
  }

  virtual void VisitObject(RawObject* obj);

 private:
  ObjectSet* allocated_set_;

  DISALLOW_COPY_AND_ASSIGN(VerifyObjectVisitor);
};

// A sample object pointer visitor implementation which verifies that
// the pointers visited are contained in the isolate heap.
class VerifyPointersVisitor : public ObjectPointerVisitor {
 public:
  explicit VerifyPointersVisitor(Isolate* isolate, ObjectSet* allocated_set)
      : ObjectPointerVisitor(isolate),
        allocated_set_(allocated_set) {
  }

  virtual void VisitPointers(RawObject** first, RawObject** last);

  static void VerifyPointers();

 private:
  ObjectSet* allocated_set_;

  DISALLOW_COPY_AND_ASSIGN(VerifyPointersVisitor);
};

class VerifyWeakPointersVisitor : public HandleVisitor {
 public:
  explicit VerifyWeakPointersVisitor(VerifyPointersVisitor* visitor)
      :  HandleVisitor(Isolate::Current()),
         visitor_(visitor) {
  }

  virtual void VisitHandle(uword addr, bool is_prologue_weak);

 private:
  ObjectPointerVisitor* visitor_;

  ObjectSet* allocated_set;

  DISALLOW_COPY_AND_ASSIGN(VerifyWeakPointersVisitor);
};

}  // namespace dart

#endif  // VM_VERIFIER_H_
