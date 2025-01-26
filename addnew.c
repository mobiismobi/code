void add_new_task(Task task_list[], int *total_tasks) {
    if (*total_tasks >= 100) {
        mvprintw(27, 0, "Task limit reached. Cannot add more tasks.");
        refresh();
        return;
    }

    char task_name[50];
    char category[30];
    char deadline[11];
    char description[100];
    int category_count;
    int priority;

    echo();
    curs_set(1);

    mvprintw(27, 0, "Enter task name: ");
    getnstr(task_name, 49);

    mvprintw(28, 0, "Enter number of categories (max 10): ");
    scanw("%d", &category_count);

    Task *new_task = &task_list[(*total_tasks)++];
    new_task->category_count = category_count > 10 ? 10 : category_count;

    for (int i = 0; i < new_task->category_count; i++) {
        mvprintw(29 + i, 0, "Enter category %d: ", i + 1);
        getnstr(category, 29);
        strncpy(new_task->categories[i], category, 29);
    }

    do {
        mvprintw(29 + category_count, 0, "Enter deadline (DD/MM/YYYY): ");
        getnstr(deadline, 10);
        if (!is_valid_date_format(deadline)) {
            mvprintw(30 + category_count, 0, "Invalid date format. Try again.   ");
        }
    } while (!is_valid_date_format(deadline));

    mvprintw(31 + category_count, 0, "Enter description: ");
    getnstr(description, 99);

    mvprintw(32 + category_count, 0, "Enter priority (1-9): ");
    scanw("%d", &priority);
    if (priority < 1 || priority > 9) priority = 1;

    noecho();
    curs_set(0);

    strncpy(new_task->name, task_name, 49);
    strncpy(new_task->deadline, deadline, 10);
    strncpy(new_task->description, description, 99);
    new_task->is_completed = false;
    new_task->priority = priority;
    new_task->subtask_count = 0;

    mvprintw(27, 0, "Task added successfully!                             ");
    refresh();
}
