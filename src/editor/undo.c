#include "undo.h"
#include <string.h>

void undo_stack_init(UndoStack *s) { memset(s, 0, sizeof(*s)); }

void undo_stack_push(UndoStack *s, UndoCmd cmd) {
    for (int i = s->top; i < s->cap; i++)
        if (s->cmds[i].free_data) s->cmds[i].free_data(s->cmds[i].data);
    s->cap = s->top;

    if (s->top >= UNDO_STACK_MAX) {
        if (s->cmds[0].free_data) s->cmds[0].free_data(s->cmds[0].data);
        memmove(&s->cmds[0], &s->cmds[1], (UNDO_STACK_MAX - 1) * sizeof(UndoCmd));
        s->top--;
        s->cap--;
    }

    if (cmd.execute) cmd.execute(cmd.data);
    s->cmds[s->top++] = cmd;
    s->cap = s->top;
}

bool undo_stack_undo(UndoStack *s) {
    if (s->top == 0) return false;
    UndoCmd *cmd = &s->cmds[--s->top];
    if (cmd->revert) cmd->revert(cmd->data);
    return true;
}

bool undo_stack_redo(UndoStack *s) {
    if (s->top >= s->cap) return false;
    UndoCmd *cmd = &s->cmds[s->top++];
    if (cmd->execute) cmd->execute(cmd->data);
    return true;
}

void undo_stack_clear(UndoStack *s) {
    for (int i = 0; i < s->cap; i++)
        if (s->cmds[i].free_data) s->cmds[i].free_data(s->cmds[i].data);
    memset(s, 0, sizeof(*s));
}

void undo_stack_push_no_exec(UndoStack *s, UndoCmd cmd) {
    /* discard redo future */
    for (int i = s->top; i < s->cap; i++)
        if (s->cmds[i].free_data) s->cmds[i].free_data(s->cmds[i].data);
    s->cap = s->top;

    if (s->top >= UNDO_STACK_MAX) {
        if (s->cmds[0].free_data) s->cmds[0].free_data(s->cmds[0].data);
        memmove(&s->cmds[0], &s->cmds[1], (UNDO_STACK_MAX - 1) * sizeof(UndoCmd));
        s->top--;
        s->cap--;
    }

    s->cmds[s->top++] = cmd;
    s->cap = s->top;
}