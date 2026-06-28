#pragma once
#include <stdbool.h>

#define UNDO_STACK_MAX 256

typedef struct {
    void (*execute)(void *data);
    void (*revert) (void *data);
    void (*free_data)(void *data);
    void *data;
} UndoCmd;

typedef struct {
    UndoCmd cmds[UNDO_STACK_MAX];
    int     top;
    int     cap;
} UndoStack;

void undo_stack_init(UndoStack *s);
void undo_stack_push(UndoStack *s, UndoCmd cmd);
bool undo_stack_undo(UndoStack *s);
bool undo_stack_redo(UndoStack *s);
void undo_stack_clear(UndoStack *s);

void undo_stack_push_no_exec(UndoStack *s, UndoCmd cmd);