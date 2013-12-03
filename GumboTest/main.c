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


bool hasClass(const char *classDefinition,const char *className) {
    char *match=0;
    unsigned long end = (int)classDefinition + strlen(classDefinition);
    
    if (strcmp(classDefinition,className)==0)
        return true;
    
    while ( (int)match < end) {
        match = strstr(classDefinition,className);
        if (match == NULL)
            return false;
        if ( (match == classDefinition || match[-1] == ' ') && (match[strlen(className)] == ' ' || match[strlen(className)] == 0))
            return true;
        
        classDefinition = &match[strlen(className)];
    }
    return false;
}

// head
// body
// body h1
// body nav
// body nav ul
// body nav ul li
// body nav ul li a
bool testSelecorAgainst(const struct SelectorFind *sel,GumboNode *node) {
    
    // ff => to the end of selector list
    bool match = false;
    
    while (true) {
        
        GumboAttribute *class = gumbo_get_attribute(&node->v.element.attributes, "class");
        GumboAttribute *id = gumbo_get_attribute(&node->v.element.attributes, "id");
        
        match = true;
        if (sel->tag != 0)
            match = node->v.element.tag == sel->tag;
        
        if (sel->class != NULL) {
            if (class) {
                match &= hasClass(class->value,sel->class);
            } else
                match = false;
        }
        
        if (sel->id != NULL) {
            if (id)
                match &= strcmp(id->value,sel->id) == 0;
            else
                match = false;
        }
        
        // <==
        if (!match) {
            if (sel->next == NULL)// если первый селектор справа не подошел - забиваем
                return false;
            else if (sel->directDescendant) {
                return false; // требование прямого потомка не выполнено - забиваем
            } else if (node->parent != NULL) { // ну теперь можно попробовать уровнем выше с тем же селектором, вдруг повезет?
                node = node->parent;
            } else {
                return false;
            }
        } else { // совпадение
            if (sel->parent == NULL) { // селекторы кончились
                return true;
            }
            if (node->parent == NULL) { // кончились предки, а селекторы нет, облом
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
    if (node->type == GUMBO_NODE_TEXT) {
        printf("{ \"text\": \"%s\" }\n",node->v.text.text);
    } else if (node->type == GUMBO_NODE_ELEMENT) {
        printf("\t{ \"tag\": \"%s\", \"params\": [ ",gumbo_normalized_tagname(node->v.element.tag));
        for (int i =0; i < node->v.element.attributes.length; i++ ) {
            GumboAttribute *a = node->v.element.attributes.data[i];
            if (i>0)
                printf(",\n");
            printf(" \"%s\": \"%s\"",a->name,a->value);
        }
        printf("], \"children\": [\n");
        for (int i =0; i < node->v.element.children.length; i++ ) {
            if (i>0)
                printf(",\n");
            printNode(node->v.element.children.data[i]);
        }
        printf("]}\n");
    }
}

void findObjects(GumboNode *rootNode, const struct SelectorFind *sel) {
    const GumboVector* root_children = &rootNode->v.element.children;
    
    for (int i=0;i < root_children->length;i++) {
        
        GumboNode* child = root_children->data[i];
        
        if (child->type == GUMBO_NODE_ELEMENT) {

            if (child->v.element.children.length > 0) {
                findObjects(child,sel);
            }
            
            // TestForSelector
            if (testSelecorAgainst(sel, child)==true ) {
                printNode(child);
            }
        }
        
        
        
    }
}


/**
 * Создает память для селектора
 **/
struct SelectorFind* newFindSelector(unsigned long strMaxSize) {
    struct SelectorFind* sel;
    sel = calloc(sizeof(struct SelectorFind),sizeof(struct SelectorFind));
    return sel;
}

/**
 * парсит селекторы
 **/
struct SelectorFind* parseSelector(char* selector) {
    char *buf,*nameBuf;
    struct SelectorFind *sel, *first;
    bool newTag = false;
    
    sel = newFindSelector(strlen(selector));
    first = sel;
    
    buf = calloc(strlen(selector),1);
    nameBuf = buf;
    

    
    for (int i=0; i < strlen(selector); i++ ) {
        if (selector[i] == '.') {
            if (strlen(nameBuf) > 0) {
                sel->tag = gumbo_tag_enum(nameBuf);
            }
            buf = sel->class = calloc(strlen(selector),1);
            continue;
        } else if (selector[i] == '#') {
            if (strlen(buf) > 0) {
                sel->tag = gumbo_tag_enum(buf);
            }
            buf = sel->id = calloc(strlen(selector),1);
            continue;
        } else if (selector[i] == ' ') {
            if (strlen(nameBuf) > 0) {
                sel->tag = gumbo_tag_enum(buf);
                buf = calloc(strlen(selector),1);
                nameBuf = buf;
                newTag = true;
            }
            continue;
        } else if (selector[i] == '>') {
            if (strlen(nameBuf) > 0) {
                sel->tag = gumbo_tag_enum(buf);
                buf = calloc(strlen(selector),1);
                nameBuf = buf;
                newTag = true;
                sel->directDescendant = true;
            }
            continue;
        } else {
            if (newTag) {
                buf = calloc(strlen(selector),1);
                nameBuf = buf;
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
    if (strlen(nameBuf) > 0 && sel->tag == 0) {
        sel->tag = gumbo_tag_enum(nameBuf);
    }
    if (sel->tag == GUMBO_TAG_UNKNOWN && sel->id == NULL && sel->class == NULL) {
        sel = sel->parent;
        free(sel->next);
        sel->next = NULL;
    }
    return first;
}

int main(int argc, const char * argv[])
{

    int selSize=0;
    char *selector;
    
    if ( argc > 1 ) {
        for (int i = 1 ; i < argc; i++) {
            selSize += strlen(argv[i])+1;
            selector = realloc(selector,selSize+1);
            strcat(selector, " ");
            strcat(selector, argv[i]);
        }
        printf("Text selector: %s\n",selector);
    } else {
        printf("Usage: cat htmlfile | %s <selector>\n\tWhere <selector> - ex. body div#div-id a.className > span\n",argv[0]);
        return 1;
    }
    
    char *buf = malloc(1024);
    char *page=calloc(1024,1);
    unsigned long readBytes=0, sz = 0;
    FILE *f = fopen("/Users/ramzes/Documents/GumboTest/GumboTest/build/Release/test.html", "r");
    
    while (!feof(f) > 0) {
        readBytes = fread(buf, 1, 1024, f);
        sz += readBytes;
        //sz = fread(buf, 1, 1024, f);
        page = realloc(page,sz+1);
        strncat(page,buf,readBytes);
    }
    printf("Parsing %lu bytes\n",sz);
    fclose(f);
    
    GumboOutput* output = gumbo_parse(page);
    
    struct SelectorFind *pSel = parseSelector(selector);
    
    printf("=> %s#%s.%s\n", gumbo_normalized_tagname(pSel->tag),pSel->id,pSel->class);
    while (pSel->next != 0) {
        pSel = pSel->next;
        printf("=> %s#%s.%s\n",gumbo_normalized_tagname(pSel->tag),pSel->id,pSel->class);
    }
    
    findObjects(output->root,pSel);
    
    
    
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    
    
    return 0;
}

