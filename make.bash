#!/bin/bash
program="./chatnet-client.cpp"


curlDeps=(curl ssl crypto brotlienc brotlidec brotlicommon gsasl idn2 ssh2 nghttp2 z zstd)
_curlDeps=""
lib=./lib-linux-amd64
for i in ${curlDeps[@]}; do
	_curlDeps="$_curlDeps $lib/lib$i.a "
done


#option="-g -fsanitize=address"
option="-static"
gcc $option -o ./bin/chatnet-linux-amd64 -Wall -Wextra  -DCURL_STATICLIB $program $_curlDeps
