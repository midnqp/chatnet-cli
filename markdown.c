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

struct render_state {
  cmark_strbuf *ansi;
  cmark_node *plain;
};

#define ANSI_RESET "\x1b[0m"
#define ANSI_BOLD "\x1b[1m"
#define ANSI_ITALIC "\x1b[3m"
#define ANSI_UNDERLINE "\x1b[4m"
#define ANSI_REVERSE "\x1b[7m"
#define ANSI_BLACK "\x1b[30m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_WHITE "\x1b[37m"

void escape_ansi(cmark_strbuf *dest, const unsigned char* source, bufsize_t length) {
    cmark_strbuf_puts(dest, source);
}

void render_node(cmark_node* node, cmark_event_type ev_type, struct render_state *state) {
    cmark_node *parent;
    cmark_node *grandparent;
    cmark_strbuf *ansi = state->ansi;

    bool entering = (ev_type == CMARK_EVENT_ENTER);

    if (state->plain == node) {
        state->plain = NULL;
    }

    if (state->plain != NULL) {
        switch(node->type) {
            case CMARK_NODE_TEXT:
            case CMARK_NODE_CODE:
            case CMARK_NODE_HTML_INLINE:
                escape_ansi(ansi, node->data, node->len);
                break;
            
            case CMARK_NODE_LINEBREAK:
            case CMARK_NODE_SOFTBREAK:
                cmark_strbuf_putc(ansi, ' ');
                break;
            
            default: 
            break;
        }
        return;
    }

    switch(node->type) {
        case CMARK_NODE_DOCUMENT:
        break;

        case CMARK_NODE_TEXT:
            escape_ansi(ansi, node->data, node->len);
            break;

        case CMARK_NODE_STRONG:
            if (entering) {
                cmark_strbuf_puts(ansi, ANSI_BOLD);
            } else {
                cmark_strbuf_puts(ansi,ANSI_RESET);
            }
            break;

        case CMARK_NODE_EMPH:
            if (entering) {
                cmark_strbuf_puts(ansi, ANSI_ITALIC);
            }
            else {
                cmark_strbuf_puts(ansi, ANSI_RESET);
            }
            break;
        
        default: 
        break;
    }
}

char* cmark_render_ansi(cmark_node* root) {
    char* result;
    cmark_strbuf ansi = CMARK_BUF_INIT(root->mem);
    cmark_event_type ev_type;
    cmark_node *cur;
    struct render_state state = {&ansi, NULL};
    cmark_iter *iter = cmark_iter_new(root);

    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cur = cmark_iter_get_node(iter);
        render_node(cur, ev_type, &state);
    }
    result = cmark_strbuf_detach(&ansi);
    cmark_iter_free(iter);
    return result;
}


int main() {
    const char* string = "**Hello**, _world_! What is [up](https://github.com)?";
    printf("%s\n\n", string);
    cmark_node* doc = cmark_parse_document(string, strlen(string), 0);
    char* result = cmark_render_ansi(doc);
    printf("%s\n", result);
    cmark_node_free(doc);
    free(result);
}