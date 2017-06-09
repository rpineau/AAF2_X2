#!/bin/bash

mkdir -p ROOT/tmp/aaf2_X2/
cp "../aaf2.ui" ROOT/tmp/aaf2_X2/
cp "../focuserlist aaf2.txt" ROOT/tmp/aaf2_X2/
cp "../build/Release/libaaf2.dylib" ROOT/tmp/aaf2_X2/

if [ ! -z "$installer_signature" ]; then
# signed package using env variable installer_signature
pkgbuild --root ROOT --identifier org.rti-zone.aaf2_X2 --sign "$installer_signature" --scripts Scripts --version 1.0 aaf2_X2.pkg
pkgutil --check-signature ./aaf2_X2.pkg
else
pkgbuild --root ROOT --identifier org.rti-zone.aaf2_X2 --scripts Scripts --version 1.0 aaf2_X2.pkg
fi

rm -rf ROOT
