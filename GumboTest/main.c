//
//  main.c
//  GumboTest
//
//  Created by Роман Усачев on 02.12.13.
//  Copyright (c) 2013 Роман Усачев. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gumbo.h"

struct SelectorFind {
    GumboTag tag;
    bool directDescendant;
    char* id;
    char* class;
    struct SelectorFind* next;
    struct SelectorFind* parent;
};

// head
// body
// body h1
// body nav
// body nav ul
// body nav ul li
// body nav ul li a
bool testSelecorAgainst(struct SelectorFind *sel,GumboNode *node) {
    while (sel->next != 0) {
        sel = sel->next;
    }
    // ff => to the end of selector list
    bool match = false;
    
    while (true) {
        
        GumboAttribute *class = gumbo_get_attribute(&node->v.element.attributes, "class");
        GumboAttribute *id = gumbo_get_attribute(&node->v.element.attributes, "id");
        
        if (sel->tag != 0)
            match = node->v.element.tag == sel->tag;
        if (strlen(sel->class)!=0) {
            if (class)
                match = strcmp(class->value,sel->class) == 0;
            else
                match = false;
        }
        
        if (strlen(sel->id)!=0) {
            if (id)
                match = strcmp(id->value,sel->id) == 0;
            else
                match = false;
        }
        
        // <==
        if (!match) {
            if (sel->next == 0)// если первый селектор справа не подошел - забиваем
                return false;
            else if (sel->directDescendant) {
                return false; // требование прямого потомка не выполнено - забиваем
            } else if (node->parent != 0) { // ну теперь можно попробовать уровнем выше с тем же селектором, вдруг повезет?
                node = node->parent;
            } else {
                return false;
            }
        } else { // совпадение
            if (sel->parent == 0) { // селекторы кончились
                return true;
            }
            if (node->parent == 0) { // кончились предки, а селекторы нет, облом
                return false;
            }
            sel = sel->parent;
            node = node->parent;
        }
    }
    // если нагулялись по циклу и не вышли, значит облом :)
    return false;
}

void printNode(GumboNode *node) {
    printf("Yay! <%s>\n",gumbo_normalized_tagname(node->v.element.tag));
}

void findObjects(GumboNode *rootNode, struct SelectorFind *sel) {
    const GumboVector* root_children = &rootNode->v.element.children;
    
    for (int i=0;i < root_children->length;i++) {
        
        GumboNode* child = root_children->data[i];
        
        if (child->type == GUMBO_NODE_ELEMENT) {

            if (child->v.element.children.length > 0) {
                findObjects(child,sel);
            }
            
            // TestForSelector
            if (testSelecorAgainst(sel, child)) {
                printNode(child);
            }
        }
        
        if (child->type == GUMBO_NODE_TEXT) {
            printf("Title: %s\n",child->v.text.text);
        }
        
    }
}


/**
 * Создает память для селектора
 **/
struct SelectorFind* newFindSelector(unsigned long strMaxSize) {
    struct SelectorFind* sel;
    sel = malloc(sizeof(struct SelectorFind));
    sel->tag = 0;
    sel->directDescendant = false;
    sel->id = malloc(strMaxSize);
    sel->class = malloc(strMaxSize);
    sel->next = 0;
    sel->parent = 0;
    return sel;
}

/**
 * парсит селекторы
 **/
struct SelectorFind* parseSelector(char* selector) {
    char* buf;
    struct SelectorFind *sel, *first;
    bool newTag = false;
    
    sel = newFindSelector(strlen(selector));
    first = sel;
    
    buf = malloc(strlen(selector));
    
    for (int i=0; i < strlen(selector); i++ ) {
        if (selector[i] == '.') {
            if (strlen(buf) > 0) {
                sel->tag = gumbo_tag_enum(buf);
            }
            buf = sel->class;
            continue;
        } else if (selector[i] == '#') {
            if (strlen(buf) > 0) {
                sel->tag = gumbo_tag_enum(buf);
            }
            buf = sel->id;
            continue;
        } else if (selector[i] == ' ') {
            if (strlen(buf) > 0) {
                sel->tag = gumbo_tag_enum(buf);
                newTag = true;
            }
           
            continue;
        } else if (selector[i] == '>') {
            if (strlen(buf) > 0) {
                sel->tag = gumbo_tag_enum(buf);
            }
            newTag = true;
            sel->directDescendant = true;
            continue;
        } else {
            if (newTag) {
                buf = malloc(strlen(selector));
                sel->next = newFindSelector(strlen(selector));
                sel->next->parent = sel;
                sel = sel->next;
                newTag = false;
            }
            unsigned long l = strlen(buf);
            buf[l] = selector[i];
            buf[l+1] = 0;
        }
    }
    return first;
}

int main(int argc, const char * argv[])
{

    
    char *selector;
    
    if ( argc > 1 ) {
        for (int i = 1 ; i < argc; i++) {
            selector = realloc(selector,strlen(argv[i]));
            strcat(selector, " ");
            strcat(selector, argv[i]);
        }
        printf("Text selector: %s\n",selector);
    } else {
        printf("Usage: cat htmlfile | %s <selector>\n\tWhere <selector> - ex. body div#div-id a.className > span",argv[0]);
        return 1;
    }
    
    char buf[1024];
    char *page;
    unsigned long readBytes=0, sz = 0;
   // FILE *f = fopen(stdin, "r");
    
    while (!feof(stdin) > 0) {
        readBytes = fread(buf, 1, 1024, stdin);
        sz += readBytes;
        //sz = fread(buf, 1, 1024, f);
        page = realloc(page,sz);
        strncat(page,buf,readBytes);
    }
    //fclose(stdin);
    
    GumboOutput* output = gumbo_parse(page);
    
    struct SelectorFind *pSel = parseSelector(selector);
    
    findObjects(output->root,pSel);
    
    
    
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    
    
    return 0;
}

