#!/bin/bash

TheSkyX_Install=~/Library/Application\ Support/Software\ Bisque/TheSkyX\ Professional\ Edition/TheSkyXInstallPath.txt
echo "TheSkyX_Install = $TheSkyX_Install"

if [ ! -f "$TheSkyX_Install" ]; then
    echo TheSkyXInstallPath.txt not found
    exit 1
fi


TheSkyX_Path=$(<"$TheSkyX_Install")
echo "Installing to $TheSkyX_Path"

if [ ! -d "$TheSkyX_Path" ]; then
    echo TheSkyX Install dir not exist
    exit 1
fi

cp "./focuserlist aaf2.txt" "$TheSkyX_Path/Resources/Common/Miscellaneous Files/"
cp "./aaf2.ui" "$TheSkyX_Path/Resources/Common/PlugInsARM32/FocuserPlugins/"
cp "./libaaf2.so" "$TheSkyX_Path/Resources/Common/PlugInsARM32/FocuserPlugins/"

app_owner=`/usr/bin/stat -c "%u" "$TheSkyX_Path" | xargs id -n -u`
if [ ! -z "$app_owner" ]; then
	chown $app_owner "$TheSkyX_Path/Resources/Common/Miscellaneous Files/focuserlist aaf2.txt"
	chown $app_owner "$TheSkyX_Path/Resources/Common/PlugInsARM32/FocuserPlugins/aaf2.ui"
	chown $app_owner "$TheSkyX_Path/Resources/Common/PlugInsARM32/FocuserPlugins/libaaf2.so"
fi
chmod  755 "$TheSkyX_Path/Resources/Common/PlugInsARM32/FocuserPlugins/libaaf2.so"