#ifndef CHATNET_MARKDOWN_H_
#define CHATNET_MARKDOWN_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "deps/cmark/cmark_ctype.h"
#include "deps/cmark/config.h"
#include "deps/cmark/cmark.h"
#include "deps/cmark/node.h"
#include "deps/cmark/buffer.h"
#include "deps/cmark/houdini.h"
#include "deps/cmark/scanners.h"

char* markdown_to_ansi(const char* message);

#endif