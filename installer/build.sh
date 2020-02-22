#!/bin/bash

PACKAGE_NAME="aaf2_X2.pkg"
BUNDLE_NAME="org.rti-zone.aaf2X2"

if [ ! -z "$app_id_signature" ]; then
    codesign -f -s "$app_id_signature" --verbose ../build/Release/libaaf2.dylib
fi

mkdir -p ROOT/tmp/aaf2_X2/
cp "../aaf2.ui" ROOT/tmp/aaf2_X2/
cp "../focuserlist aaf2.txt" ROOT/tmp/aaf2_X2/
cp "../build/Release/libaaf2.dylib" ROOT/tmp/aaf2_X2/


if [ ! -z "$installer_signature" ]; then
	# signed package using env variable installer_signature
	pkgbuild --root ROOT --identifier $BUNDLE_NAME --sign "$installer_signature" --scripts Scripts --version 1.0 $PACKAGE_NAME
	pkgutil --check-signature ./${PACKAGE_NAME}

else
    pkgbuild --root ROOT --identifier $BUNDLE_NAME --scripts Scripts --version 1.0 $PACKAGE_NAME
fi

rm -rf ROOT
