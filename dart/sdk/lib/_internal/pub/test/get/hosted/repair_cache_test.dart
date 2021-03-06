// Copyright (c) 2012, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

library pub_tests;

import '../../descriptor.dart' as d;
import '../../test_pub.dart';

main() {
  initConfig();
  integration('re-gets a package if it has an empty "lib" directory', () {
    servePackages([packageMap("foo", "1.2.3")]);

    // Set up a cache with a broken foo package.
    d.dir(cachePath, [
      d.dir('hosted', [
        d.async(port.then((p) => d.dir('127.0.0.1%58$p', [
          d.dir("foo-1.2.3", [
            d.libPubspec("foo", "1.2.3"),
            // Note: empty "lib" directory.
            d.dir("lib", [])
          ])
        ])))
      ])
    ]).create();

    d.appDir({"foo": "1.2.3"}).create();

    pubGet();

    d.cacheDir({"foo": "1.2.3"}).validate();
    d.packagesDir({"foo": "1.2.3"}).validate();
  });

  integration('re-gets a package if it has no pubspec', () {
    servePackages([packageMap("foo", "1.2.3")]);

    // Set up a cache with a broken foo package.
    d.dir(cachePath, [
      d.dir('hosted', [
        d.async(port.then((p) => d.dir('127.0.0.1%58$p', [
          d.dir("foo-1.2.3", [
            d.libDir("foo")
            // Note: no pubspec.
          ])
        ])))
      ])
    ]).create();

    d.appDir({"foo": "1.2.3"}).create();

    pubGet();

    d.cacheDir({"foo": "1.2.3"}).validate();
    d.packagesDir({"foo": "1.2.3"}).validate();
  });
}
