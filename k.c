#include <ncurses.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cjson/cJSON.h>

// تعریف ساختارها
typedef struct {
    char name[50];
    bool is_completed;
} Subtask;

typedef struct {
    char name[50];
    bool is_completed;
    int priority;
    char deadline[11];
    char description[100];
    char categories[10][30];
    int category_count;
    Subtask subtasks[50];
    int subtask_count;
} Task;

// توابع مربوط به مدیریت تسک‌ها
void add_new_task(Task task_list[], int *total_tasks);
void delete_selected_task(Task task_list[], int *total_tasks, int selected_task_index);
void edit_task_name(Task task_list[], int selected_task_index);
void edit_task_description(Task task_list[], int selected_task_index);
void add_new_deadline(Task task_list[], int selected_task_index);
void manage_categories(Task task_list[], int selected_task_index);
void sort_tasks(Task task_list[], int total_tasks);
void search_tasks(Task task_list[], int total_tasks, const char *query, int *selected_task_index);
void save_tasks_to_file(Task task_list[], int total_tasks, const char *filename);
void load_tasks_from_file(Task task_list[], int *total_tasks, const char *filename);
void add_new_subtask(Task task_list[], int selected_task_index);
void delete_selected_subtask(Task task_list[], int selected_task_index);
void toggle_subtask_status(Task task_list[], int selected_task_index, int selected_subtask_index);
void display_subtasks(Task task_list[], int selected_task_index);
void display_tasks(Task task_list[], int total_tasks, int selected_task_index, bool is_in_subtask_mode);
void display_metadata(Task task_list[], int selected_task_index);

// توابع مربوط به رابط کاربری
void initialize_ui();
void draw_ui();
void handle_user_input(Task task_list[], int *total_tasks, int *selected_task_index, int *selected_subtask_index, bool *is_in_subtask_mode);

// تابع اصلی
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

// پیاده‌سازی توابع

int is_valid_date_format(const char *date) {
    if (strlen(date) != 10) return 0;

    if (!isdigit(date[0]) || !isdigit(date[1]) || date[2] != '/' ||
        !isdigit(date[3]) || !isdigit(date[4]) || date[5] != '/' ||
        !isdigit(date[6]) || !isdigit(date[7]) ||
        !isdigit(date[8]) || !isdigit(date[9])) {
        return 0;
    }

    int day = (date[0] - '0') * 10 + (date[1] - '0');
    int month = (date[3] - '0') * 10 + (date[4] - '0');
    int year = (date[6] - '0') * 1000 + (date[7] - '0') * 100 +
               (date[8] - '0') * 10 + (date[9] - '0');

    if (day < 1 || day > 31 || month < 1 || month > 12 || year < 1) {
        return 0;
    }

    return 1;
}

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

void delete_selected_task(Task task_list[], int *total_tasks, int selected_task_index) {
    if (*total_tasks == 0) {
        mvprintw(27, 0, "No tasks available to delete.");
        refresh();
        return;
    }

    for (int i = selected_task_index; i < *total_tasks - 1; i++) {
        task_list[i] = task_list[i + 1];
    }
    (*total_tasks)--;
    if (selected_task_index >= *total_tasks && *total_tasks > 0) {
        selected_task_index = *total_tasks - 1;
    }

    mvprintw(27, 0, "Task deleted successfully!                           ");
    refresh();
}

void edit_task_name(Task task_list[], int selected_task_index) {
    if (selected_task_index < 0 || selected_task_index >= 100) {
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter new task name: ");
    getnstr(task_list[selected_task_index].name, 49);
    noecho();
    curs_set(0);
    mvprintw(27, 0, "Task name updated successfully!");
    refresh();
}

void edit_task_description(Task task_list[], int selected_task_index) {
    if (selected_task_index < 0 || selected_task_index >= 100) {
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter new description: ");
    getnstr(task_list[selected_task_index].description, 99);
    noecho();
    curs_set(0);
    mvprintw(27, 0, "Task description updated successfully!");
    refresh();
}

void add_new_deadline(Task task_list[], int selected_task_index) {
    if (selected_task_index < 0 || selected_task_index >= 100) {
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    char new_deadline[11];
    echo();
    curs_set(1);
    do {
        mvprintw(27, 0, "Enter new deadline (DD/MM/YYYY): ");
        getnstr(new_deadline, 10);
        if (!is_valid_date_format(new_deadline)) {
            mvprintw(28, 0, "Invalid date format. Try again.");
        }
    } while (!is_valid_date_format(new_deadline));

    strncpy(task_list[selected_task_index].deadline, new_deadline, 10);
    noecho();
    curs_set(0);
    mvprintw(27, 0, "Deadline updated successfully!");
    refresh();
}

void manage_categories(Task task_list[], int selected_task_index) {
    static bool category_mode = false;
    if (!category_mode) {
        category_mode = true;
        mvprintw(27, 0, "Category mode enabled. Use 'a' to add, 'd' to delete, 'j/k' to navigate. Press 'c' to exit.");
    } else {
        category_mode = false;
        mvprintw(27, 0, "Category mode disabled. Returning to task view.");
    }
    refresh();

    while (category_mode) {
        int ch = getch();
        switch (ch) {
            case 'a':
                if (task_list[selected_task_index].category_count >= 10) {
                    mvprintw(27, 0, "Category limit reached for this task.");
                } else {
                    echo();
                    curs_set(1);
                    mvprintw(28, 0, "Enter new category: ");
                    getnstr(task_list[selected_task_index].categories[task_list[selected_task_index].category_count++], 29);
                    noecho();
                    curs_set(0);
                    mvprintw(27, 0, "Category added successfully!");
                }
                break;

            case 'd':
                if (task_list[selected_task_index].category_count == 0) {
                    mvprintw(27, 0, "No categories to delete.");
                } else {
                    for (int i = selected_task_index; i < task_list[selected_task_index].category_count - 1; i++) {
                        strncpy(task_list[selected_task_index].categories[i], task_list[selected_task_index].categories[i + 1], 30);
                    }
                    task_list[selected_task_index].category_count--;
                    if (selected_task_index >= task_list[selected_task_index].category_count && task_list[selected_task_index].category_count > 0) {
                        selected_task_index = task_list[selected_task_index].category_count - 1;
                    }
                    mvprintw(27, 0, "Category deleted successfully!");
                }
                break;

            case 'j':
                if (selected_task_index < task_list[selected_task_index].category_count - 1) {
                    selected_task_index++;
                }
                break;

            case 'k':
                if (selected_task_index > 0) {
                    selected_task_index--;
                }
                break;

            case 'c':
                category_mode = false;
                mvprintw(27, 0, "Exiting category mode...");
                break;
        }

        display_metadata(task_list, selected_task_index);
    }
}

void sort_tasks(Task task_list[], int total_tasks) {
    for (int i = 0; i < total_tasks - 1; i++) {
        for (int j = i + 1; j < total_tasks; j++) {
            if (task_list[i].priority > task_list[j].priority) {
                Task temp = task_list[i];
                task_list[i] = task_list[j];
                task_list[j] = temp;
            }
        }
    }
}

void search_tasks(Task task_list[], int total_tasks, const char *query, int *selected_task_index) {
    for (int i = 0; i < total_tasks; i++) {
        if (strstr(task_list[i].name, query) != NULL) {
            *selected_task_index = i;
            break;
        }
    }
}

void save_tasks_to_file(Task task_list[], int total_tasks, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        mvprintw(27, 0, "Error opening file for writing.");
        refresh();
        return;
    }

    for (int i = 0; i < total_tasks; i++) {
        fprintf(file, "%s\n", task_list[i].name);
        fprintf(file, "%d\n", task_list[i].is_completed ? 1 : 0); // Save bool as int
        fprintf(file, "%d\n", task_list[i].priority);
        fprintf(file, "%s\n", task_list[i].deadline);
        fprintf(file, "%s\n", task_list[i].description);
        fprintf(file, "%d\n", task_list[i].category_count);
        for (int j = 0; j < task_list[i].category_count; j++) {
            fprintf(file, "%s\n", task_list[i].categories[j]);
        }
        fprintf(file, "%d\n", task_list[i].subtask_count);
        for (int j = 0; j < task_list[i].subtask_count; j++) {
            fprintf(file, "%s\n", task_list[i].subtasks[j].name);
            fprintf(file, "%d\n", task_list[i].subtasks[j].is_completed ? 1 : 0); // Save bool as int
        }
    }

    fclose(file);
    mvprintw(27, 0, "Tasks saved successfully!                             ");
    refresh();
}

void load_tasks_from_file(Task task_list[], int *total_tasks, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        mvprintw(27, 0, "Error opening file for reading.");
        refresh();
        return;
    }

    *total_tasks = 0;
    while (fscanf(file, "%s\n", task_list[*total_tasks].name) != EOF) {
        int is_completed_temp;
        fscanf(file, "%d\n", &is_completed_temp);
        task_list[*total_tasks].is_completed = is_completed_temp ? true : false;

        fscanf(file, "%d\n", &task_list[*total_tasks].priority);
        fscanf(file, "%s\n", task_list[*total_tasks].deadline);
        fscanf(file, "%s\n", task_list[*total_tasks].description);
        fscanf(file, "%d\n", &task_list[*total_tasks].category_count);
        for (int j = 0; j < task_list[*total_tasks].category_count; j++) {
            fscanf(file, "%s\n", task_list[*total_tasks].categories[j]);
        }
        fscanf(file, "%d\n", &task_list[*total_tasks].subtask_count);
        for (int j = 0; j < task_list[*total_tasks].subtask_count; j++) {
            fscanf(file, "%s\n", task_list[*total_tasks].subtasks[j].name);
            int subtask_completed_temp;
            fscanf(file, "%d\n", &subtask_completed_temp);
            task_list[*total_tasks].subtasks[j].is_completed = subtask_completed_temp ? true : false;
        }
        (*total_tasks)++;
    }

    fclose(file);
    mvprintw(27, 0, "Tasks loaded successfully!                             ");
    refresh();
}

void add_new_subtask(Task task_list[], int selected_task_index) {
    if (task_list[selected_task_index].subtask_count >= 50) {
        mvprintw(27, 0, "Subtask limit reached. Cannot add more subtasks.");
        refresh();
        return;
    }

    char subtask_name[50];
    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter subtask name: ");
    getnstr(subtask_name, 49);
    noecho();
    curs_set(0);

    Subtask *new_subtask = &task_list[selected_task_index].subtasks[task_list[selected_task_index].subtask_count++];
    strncpy(new_subtask->name, subtask_name, 49);
    new_subtask->is_completed = false;

    mvprintw(27, 0, "Subtask added successfully!                             ");
    refresh();
}

void delete_selected_subtask(Task task_list[], int selected_task_index) {
    if (task_list[selected_task_index].subtask_count == 0) {
        mvprintw(27, 0, "No subtasks available to delete.");
        refresh();
        return;
    }

    for (int i = selected_task_index; i < task_list[selected_task_index].subtask_count - 1; i++) {
        task_list[selected_task_index].subtasks[i] = task_list[selected_task_index].subtasks[i + 1];
    }
    task_list[selected_task_index].subtask_count--;
    if (selected_task_index >= task_list[selected_task_index].subtask_count && task_list[selected_task_index].subtask_count > 0) {
        selected_task_index = task_list[selected_task_index].subtask_count - 1;
    }

    mvprintw(27, 0, "Subtask deleted successfully!                           ");
    refresh();
}

void toggle_subtask_status(Task task_list[], int selected_task_index, int selected_subtask_index) {
    if (selected_task_index >= 0 && selected_task_index < 100 && selected_subtask_index >= 0 && selected_subtask_index < task_list[selected_task_index].subtask_count) {
        task_list[selected_task_index].subtasks[selected_subtask_index].is_completed = !task_list[selected_task_index].subtasks[selected_subtask_index].is_completed;
    }
}

void display_subtasks(Task task_list[], int selected_task_index) {
    clear();
    for (int i = 0; i < task_list[selected_task_index].subtask_count; i++) {
        mvprintw(i, 0, "%d. [%c] %s", i + 1, task_list[selected_task_index].subtasks[i].is_completed ? 'x' : ' ', task_list[selected_task_index].subtasks[i].name);
    }
    refresh();
}

void display_tasks(Task task_list[], int total_tasks, int selected_task_index, bool is_in_subtask_mode) {
    (void)is_in_subtask_mode; // برای جلوگیری از هشدار استفاده نشدن پارامتر
    clear();
    for (int i = 0; i < total_tasks; i++) {
        if (i == selected_task_index) {
            attron(A_REVERSE);
        }
        mvprintw(i, 0, "%d. [%c] %s", i + 1, task_list[i].is_completed ? 'x' : ' ', task_list[i].name);
        if (i == selected_task_index) {
            attroff(A_REVERSE);
        }
    }
    refresh();
}

void display_metadata(Task task_list[], int selected_task_index) {
    clear();
    mvprintw(0, 0, "Task: %s", task_list[selected_task_index].name);
    mvprintw(1, 0, "Description: %s", task_list[selected_task_index].description);
    mvprintw(2, 0, "Deadline: %s", task_list[selected_task_index].deadline);
    refresh();
}

void initialize_ui() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
}

void draw_ui() {
    clear();
    refresh();

    WINDOW *task_window = newwin(15, 40, 0, 0);
    box(task_window, 0, 0);
    mvwprintw(task_window, 0, 2, "Tasks");
    wrefresh(task_window);

    WINDOW *subtask_window = newwin(15, 40, 0, 40);
    box(subtask_window, 0, 0);
    mvwprintw(subtask_window, 0, 2, "Subtasks");
    wrefresh(subtask_window);

    WINDOW *category_window = newwin(5, 40, 15, 0);
    box(category_window, 0, 0);
    mvwprintw(category_window, 0, 2, "Categories");
    wrefresh(category_window);

    WINDOW *deadline_window = newwin(5, 40, 15, 40);
    box(deadline_window, 0, 0);
    mvwprintw(deadline_window, 0, 2, "Deadline");
    wrefresh(deadline_window);

    WINDOW *description_window = newwin(5, 80, 20, 0);
    box(description_window, 0, 0);
    mvwprintw(description_window, 0, 2, "Description");
    wrefresh(description_window);

    mvprintw(26, 0, "Keys: 'q' to quit, 'a' to add task, 'j'/'k' to navigate, 'd' to delete, 'SPACE' to toggle status, 's' to sort, 'l' to point subtasks, 'h' to back task,\n 'e' to edit task's name, 'r' to edit task's desciption, 'n' to add new deadline, 'c' to edit categories, 'w' to save, 'x' to retrive.");
    refresh();
}

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
                display_tasks(task_list, *total_tasks, *selected_task_index, *is_in_subtask_mode); // تغییر این خط
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
        display_tasks(task_list, *total_tasks, *selected_task_index, *is_in_subtask_mode); // تغییر این خط
        display_subtasks(task_list, *selected_task_index);
        display_metadata(task_list, *selected_task_index);
    }
}
