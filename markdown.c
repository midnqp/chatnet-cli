#include "markdown.h"

#include "str.h"
#include "util.h"

struct render_state {
  cmark_strbuf *ansi;
  cmark_node *plain;
};

#define ANSI_RESET "\033[0;0;0m"
#define ANSI_BOLD "\x1b[1m"
#define ANSI_ITALIC "\x1b[3m"
#define ANSI_UNDERLINE "\x1b[4m"
#define ANSI_REVERSE "\x1b[7m"
#define ANSI_REVERSE_RESET "\x1b[27m"
#define ANSI_YELLOW_BG "\x1b[43m"
#define ANSI_BLUE "\x1b[34m"

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
                cmark_strbuf_putc(ansi, '\0');
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

        case CMARK_NODE_CODE:
            if (entering) {
                cmark_strbuf_puts(ansi, ANSI_YELLOW_BG);
                cmark_strbuf_puts(ansi, cmark_node_get_literal(node));
                cmark_strbuf_puts(ansi, ANSI_RESET);
            }
            break;

        case CMARK_NODE_SOFTBREAK:
        case CMARK_NODE_LINEBREAK:
            cmark_strbuf_puts(ansi, "\n");
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

char* markdown_to_ansi(const char* message) {
    cmark_node* doc = cmark_parse_document(message, strlen(message), 0);
    char* _result = cmark_render_ansi(doc);
    cmark_node_free(doc);
    char* result = strinit(1);
    strappend(&result, _result);
    return result;
}
