#ifndef UI_CONTROLL_H
#define UI_CONTROLL_H

#include <ncurses.h>
#include "task_manager.h"

void initialize_ui();
void draw_ui();
void handle_user_input(Task task_list[], int *total_tasks, int *selected_task_index, int *selected_subtask_index, bool *is_in_subtask_mode);

#endif
