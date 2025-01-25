#include "ui_controll.h"
#include "task_manager.h"

int main() {
    Task task_list[100];
    int total_tasks = 0;
    int selected_task_index = 0;
    int selected_subtask_index = 0;
    bool is_in_subtask_mode = false;

    initialize_ui();
    draw_ui();

    load_tasks_from_file(task_list, &total_tasks, "tasks.json");

    handle_user_input(task_list, &total_tasks, &selected_task_index, &selected_subtask_index, &is_in_subtask_mode);

    save_tasks_to_file(task_list, total_tasks, "tasks.json");

    endwin();
    return 0;
}
