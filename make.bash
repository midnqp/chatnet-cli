#!/bin/bash
info="\033[0;32m[info]\033[0m"
err="\033[0;31m[err]\033[0m"



detect_compiler() {
	#if echo $(which clang) | grep -qF "clang"; then
	#	CC="clang"
	#elif echo $(which gcc) | grep -qF "gcc"; then
	#	CC="gcc"
	#fi
	CC=x86_64-linux-gnu-gcc
	echo -e "$info Using compiler: $CC"
}




detect_os() {
	osfound=$(uname -o)
	case $osfound in
		"GNU/Linux")
			os="gnuLinux"
			;;
		"Android")
			os="android"
			;;
		*)
			echo -e "$err OS not recognized"
			exit 1
	esac
	echo -e "$info Found OS: $os"
}




detect_machine(){
	m=$(uname -m)
	echo -e "$info Found machine: $m"
}




program=$1
if [ "$program" = "" ]; then 
	program="./chatnet-client.cpp"; 
	prognAutoSet="true"
fi

if [ ! -f "$program" ]; then
	echo -e "$err Main program $program doesn't exist."
	exit
fi
echo -e "$info Found main program: $program"




static=$2
if [ "$static" = "" ]; then
	echo -e "$err Not compiling statically."
else
	echo -e "$info Compiling statically."
fi




### With libcurl inside, it is no longer possible to have static binaries.
### because libcurl contains: getaddrinfo()
#components="-Wall -Wextra -g ./libcurl.a ./libcjson.a -lssl -lcrypto -lpthread"
#components="-g -Wall -Wextra -lcurl "




main() {
	# -fsanitize=address turned on at 210629 2311
	# components="-Wall -fsanitize=address -Wextra -g $idir/libcurl.a  -lssl -lcrypto -lpthread"
	# curlDeps=("curl" "ssl" "crypto" "brotlienc-static" "brotlidec-static" "brotlicommon-static" "gsasl" "idn2" "ssh2" "nghttp2" "z" "zstd")
	
	curlDeps=(curl ssl crypto brotlienc-static brotlidec-static brotlicommon-static gsasl idn2 ssh2 nghttp2 z zstd)
	_curlDeps=""
	lib=./lib
	for i in ${curlDeps[@]}; do
		_curlDeps="$_curlDeps $lib/lib$i.a "
	done



	if [ "$program" != "" ]; then
		detect_compiler
		detect_os
		detect_machine

		lenpn=${#program}   #length of Program's name
		leni=$(($lenpn-2))  #len(".c")=2  #length index
		exen=$(echo $program | cut -c1-$leni)  #executable name
		exenfull="$exen-$os-$m"
		

		$CC -o $exenfull -Wall -Wextra -g -fsanitize=address -DCURL_STATICLIB $program $_curlDeps
	

		if echo $? | grep -qF "0"; then 
			if [ ! -d ./chatnet_role_change/ ]; then
				mkdir ./chatnet_role_change/
			fi
			
			cp ./$exenfull ./chatnet_role_change/
			echo -e "$info Program copied to ./chatnet_role_change";
			#--------------------
			./$exenfull
			#--------------------
		
		else echo -e "$err Program not compiled"
		fi
	else
		echo -e "$err Program name missing"
		exit
	fi
}
main
