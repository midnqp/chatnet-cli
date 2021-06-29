#!/bin/sh
info="\033[0;32m[info]\033[0m"
err="\033[0;31m[err]\033[0m"




detect_compiler() {
	if echo $(which clang) | grep -qF "clang"; then
		CC="clang"
	elif echo $(which gcc) | grep -qF "gcc"; then
		CC="gcc"
	fi
	echo "$info Using compiler: $CC"
}




detect_os() {
	osfound=$(uname -o)
	case $osfound in
		"GNU/Linux")
			os="gnu"
			;;
		"Android")
			os="android"
			;;
		*)
			echo "$err OS not recognized"
			exit 1
	esac
	echo "$info Found OS: $os"
}




detect_machine(){
	m=$(uname -m)
	echo "$info Found machine: $m"
}




program=$1
if [ "$program" = "" ]; then program="./chatnet.c"; fi

if [ ! -f "$program" ]; then
	echo "$err Main program $program doesn't exist."
	exit
fi
echo "$info Found main program: $program"




### With libcurl inside, it is no longer possible to have static binaries.
### because libcurl contains: getaddrinfo()
#components="-Wall -Wextra -g ./libcurl.a ./libcjson.a -lssl -lcrypto -lpthread"
#components="-g -Wall -Wextra -lcurl "




main() {
	idir="./include"
	# -fsanitize=address turned on at 210629 2311
	components="-Wall -fsanitize=address -Wextra -g $idir/libcurl.a  -lssl -lcrypto -lpthread"
	
	if [ "$program" != "" ]; then
		detect_compiler
		detect_os
		detect_machine

		lenpn=${#program}   #length of Program's name
		leni=$(($lenpn-2))  #len(".c")=2  #length index
		exen=$(echo $program | cut -c1-$leni)  #executable name
		exenfull="$exen-$os-$m"
		$CC $program $components -o $exenfull
	

		if echo $? | grep -qF "0"; then 
			if [ ! -d ./chatnet_role_change/ ]; then
				mkdir ./chatnet_role_change/
			fi
			
			cp ./$exenfull ./chatnet_role_change/
			echo "$info Program copied to ./chatnet_role_change";
			#./$exenfull
		
		else echo "$err Program not compiled"
		fi
	else
		echo "$err Program name missing"
		exit
	fi
}
main
