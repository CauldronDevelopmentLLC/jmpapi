/******************************************************************************\
                 This file is part of the JmpAPI software.
                Copyright (c) 2014-2026, Cauldron Development Oy
                          All rights reserved.
\******************************************************************************/

// Test driver for JmpAPI::ContextResolver.  Reads a JSON document on stdin:
//
//   {"root": {"args": {...}, ...},   // namespaces on the parent resolver
//    "ctx":  {...},                  // the local context
//    "queries": ["name", "./x", "../y", ".."]}
//
// Prints each query and the resolved value (or <null>), exercising the
// context-relative "./", "../" and ".." selection.

#include <cbang/Catch.h>
#include <cbang/json/Reader.h>
#include <cbang/json/Dict.h>
#include <cbang/api/Resolver.h>

#include <jmpapi/ContextResolver.h>

#include <iostream>

using namespace cb;
using namespace std;


int main(int argc, char *argv[]) {
  try {
    auto input = JSON::Reader::parse(cin);

    API::ResolverPtr root = new API::Resolver();
    if (input->has("root"))
      for (auto e: input->get("root")->entries())
        root->set(e.key(), e.value());

    JSON::ValuePtr ctx =
      input->has("ctx") ? input->get("ctx") : new JSON::Dict;

    auto resolver = SmartPtr(new JmpAPI::ContextResolver(root, ctx));

    for (auto &q: *input->get("queries")) {
      string name = q->asString();
      cout << name << " = ";
      try {
        auto value = resolver->select(name);
        cout << (value.isNull() ? string("<null>") : value->toString(0, true));
      } catch (const Exception &e) {cout << "<error: " << e.getMessage() << ">";}
      cout << "\n";
    }

    return 0;
  } CATCH_ERROR;

  return 1;
}
