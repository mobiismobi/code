void handle_user_input(Task task_list[], int *total_tasks, int *selected_task_index, int *selected_subtask_index, bool *is_in_subtask_mode) {
    char ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case 'a':
                if (*is_in_subtask_mode) {
                    add_new_subtask(task_list, *selected_task_index);
                } else {
                    add_new_task(task_list, total_tasks);
                }
                break;
            case 'd':
                if (*is_in_subtask_mode) {
                    delete_selected_subtask(task_list, *selected_task_index);
                } else {
                    delete_selected_task(task_list, total_tasks, *selected_task_index);
                }
                break;
            case 'j':
                if (*is_in_subtask_mode) {
                    if (*selected_subtask_index < task_list[*selected_task_index].subtask_count - 1) {
                        (*selected_subtask_index)++;
                    }
                } else if (*selected_task_index < *total_tasks - 1) {
                    (*selected_task_index)++;
                }
                break;
            case 'k':
                if (*is_in_subtask_mode) {
                    if (*selected_subtask_index > 0) {
                        (*selected_subtask_index)--;
                    }
                } else if (*selected_task_index > 0) {
                    (*selected_task_index)--;
                }
                break;
            case 'l':
                if (!*is_in_subtask_mode && *total_tasks > 0) {
                    *is_in_subtask_mode = true;
                    *selected_subtask_index = 0;
                }
                break;
            case 'h':
                if (*is_in_subtask_mode) {
                    *is_in_subtask_mode = false;
                }
                break;
            case ' ':
                if (*is_in_subtask_mode) {
                    toggle_subtask_status(task_list, *selected_task_index, *selected_subtask_index);
                } else {
                    if (*total_tasks > 0) {
                        task_list[*selected_task_index].is_completed = !task_list[*selected_task_index].is_completed;
                    }
                }
                break;
            case 's':
                sort_tasks(task_list, *total_tasks);
                display_tasks(task_list, *total_tasks, *selected_task_index, *is_in_subtask_mode);
                display_metadata(task_list, *selected_task_index);
                break;
            case 'e':
                edit_task_name(task_list, *selected_task_index);
                break;
            case 'r':
                edit_task_description(task_list, *selected_task_index);
                break;
            case 'n':
                add_new_deadline(task_list, *selected_task_index);
                break;
            case 'c':
                manage_categories(task_list, *selected_task_index);
                break;
            case 'w':
                save_tasks_to_file(task_list, *total_tasks, "tasks.json");
                break;
            case 'x':
                load_tasks_from_file(task_list, total_tasks, "tasks.json");
                display_metadata(task_list, *selected_task_index);
                break;
            case '/':
                echo();
                curs_set(1);
                char query[100];
                mvprintw(27, 0, "Enter search query: ");
                getnstr(query, 99);
                noecho();
                curs_set(0);
                search_tasks(task_list, *total_tasks, query, selected_task_index);
                break;
        }
        display_tasks(task_list, *total_tasks, *selected_task_index, *is_in_subtask_mode);
        display_subtasks(task_list, *selected_task_index);
        display_metadata(task_list, *selected_task_index);
    }
}
