#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cjson/cJSON.h>
#include <stdio.h>

#define MAX_TASKS 100
#define MAX_SUBTASKS 50
#define TASK_NAME_LENGTH 50
#define SUBTASK_NAME_LENGTH 50
#define MAX_CATEGORIES 10
#define CATEGORY_NAME_LENGTH 30

typedef struct {
    char name[SUBTASK_NAME_LENGTH];
    int completed; 
} Subtask;

typedef struct {
    char name[TASK_NAME_LENGTH];
    int completed; 
    int priority;
    char due_date[11];
    char description[100];
    char categories[MAX_CATEGORIES][CATEGORY_NAME_LENGTH];
    int category_count;
    Subtask subtasks[MAX_SUBTASKS];
    int subtask_count;
} Task;

Task tasks[MAX_TASKS];
int task_count = 0; 
int selected_task = 0; 
int selected_subtask = 0;
int in_subtask_mode = 0; 
int selected_category = 0;

WINDOW *task_win, *subtask_win, *category_win, *deadline_win, *description_win;

void setup_windows() {
    clear();
    refresh();

    task_win = newwin(15, 40, 0, 0);
    box(task_win, 0, 0);
    mvwprintw(task_win, 0, 2, "Tasks");
    wrefresh(task_win);

    category_win = newwin(10, 40, 15, 0);
    box(category_win, 0, 0);
    mvwprintw(category_win, 0, 2, "Categories");
    wrefresh(category_win);

    description_win = newwin(10, 40, 0, 40);
    box(description_win, 0, 0);
    mvwprintw(description_win, 0, 2, "Description");
    wrefresh(description_win);

    subtask_win = newwin(10, 40, 10, 40);
    box(subtask_win, 0, 0);
    mvwprintw(subtask_win, 0, 2, "Subtasks");
    wrefresh(subtask_win);

    deadline_win = newwin(5, 40, 20, 40);
    box(deadline_win, 0, 0);
    mvwprintw(deadline_win, 0, 2, "Deadline");
    wrefresh(deadline_win);

    mvprintw(25, 0, "Keys: 'q' to quit, 'a' to add task, 'j'/'k' to navigate, 'd' to delete, 'SPACE' to toggle status, 's' to sort, 'l' to point subtasks, 'h' to back task, 'e' to edit task's name, 'r' to edit task's description, 'n' to add new deadline, 'c' to edit categories, 'w' to save, 'x' to retrieve.");
    refresh();
}

int validate_date_format(const char *date) { 
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

    if (month < 1 || month > 12 || year < 1) {
        return 0;
    }

    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // Number of days in each month
    if (day < 1 || day > (month == 2 && (year % 4 == 0) ? 29 : days_in_month[month - 1])) {
        return 0;
    }

    return 1; 
}

void add_task() { 
    if (task_count >= MAX_TASKS) {
        mvprintw(27, 0, "Task limit reached. Cannot add more tasks.");
        refresh();
        return;
    }

    char title[TASK_NAME_LENGTH];
    char category[CATEGORY_NAME_LENGTH];
    char due_date[11];
    char description[100];
    int category_count;
    int priority;

    echo(); 
    curs_set(1); 

    mvprintw(27, 0, "Enter task name: ");
    getnstr(title, TASK_NAME_LENGTH - 1); 

    mvprintw(28, 0, "Enter number of categories (max %d): ", MAX_CATEGORIES);
    scanw("%d", &category_count); 

    Task *new_task = &tasks[task_count++];

    new_task->category_count = category_count > MAX_CATEGORIES ? MAX_CATEGORIES : category_count;

    for (int i = 0; i < new_task->category_count; i++) {
        mvprintw(29 + i, 0, "Enter category %d: ", i + 1);
        getnstr(category, CATEGORY_NAME_LENGTH - 1);
        strncpy(new_task->categories[i], category, CATEGORY_NAME_LENGTH - 1);
    }

    do {
        mvprintw(29 + category_count, 0, "Enter deadline (DD/MM/YYYY): ");
        getnstr(due_date, 10);
        if (!validate_date_format(due_date)) {
            mvprintw(30 + category_count, 0, "Invalid date format. Try again.   ");
        }
    } while (!validate_date_format(due_date));

    mvprintw(31 + category_count, 0, "Enter description: ");
    getnstr(description, 99);

    mvprintw(32 + category_count, 0, "Enter priority (1-9): ");
    scanw("%d", &priority);
    if (priority < 1 || priority > 9) priority = 1; 

    noecho(); 
    curs_set(0); 

    strncpy(new_task->name, title, TASK_NAME_LENGTH - 1);
    strncpy(new_task->due_date, due_date, 10);
    strncpy(new_task->description, description, 99);
    new_task->completed = 0;
    new_task->priority = priority;
    new_task->subtask_count = 0;

    mvprintw(27, 0, "                             Task added successfully!                             ");
    refresh();
}

void add_subtask() { 
    if (task_count == 0) {
        mvprintw(27, 0, "No tasks available to add a subtask.     ");
        refresh();
        return;
    }

    Task *current_task = &tasks[selected_task];
    if (current_task->subtask_count >= MAX_SUBTASKS) {
        mvprintw(27, 0, "Subtask limit reached for this task.     ");
        refresh();
        return;
    }

    char subtask_title[SUBTASK_NAME_LENGTH];
    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter subtask name: ");
    getnstr(subtask_title, SUBTASK_NAME_LENGTH - 1);
    noecho();
    curs_set(0);

    Subtask *new_subtask = &current_task->subtasks[current_task->subtask_count++];
    strncpy(new_subtask->name, subtask_title, SUBTASK_NAME_LENGTH - 1);
    new_subtask->completed = 0; 

    mvprintw(27, 0, "Subtask added successfully!             ");
    refresh();
}

void delete_task() { 
    if (task_count == 0) {
        mvprintw(27, 0, "No tasks available to delete a task. ");
        refresh();
        return;
    }

    for (int i = selected_task; i < task_count - 1; i++) {
        tasks[i] = tasks[i + 1];
    }
    task_count--;
    if (selected_task >= task_count && task_count > 0) {
        selected_task = task_count - 1;
    }

    mvprintw(27, 0, "Task deleted successfully!           ");
    refresh();
}

void delete_subtask() { 
    if (task_count == 0) {
        mvprintw(27, 0, "No tasks available to delete a subtask. ");
        refresh();
        return;
    }

    Task *current_task = &tasks[selected_task];
    if (current_task->subtask_count == 0) {
        mvprintw(27, 0, "No subtasks available to delete.         ");
        refresh();
        return;
    }

    for (int i = selected_subtask; i < current_task->subtask_count - 1; i++) {
        current_task->subtasks[i] = current_task->subtasks[i + 1];
    }
    current_task->subtask_count--;
    if (selected_subtask >= current_task->subtask_count && current_task->subtask_count > 0) {
        selected_subtask = current_task->subtask_count - 1;
    }

    mvprintw(27, 0, "Subtask deleted successfully!           ");
    refresh();
}

void toggle_task_status() { 
    if (task_count == 0) {
        mvprintw(27, 0, "No tasks available to toggle status. ");
        refresh();
        return;
    }

    tasks[selected_task].completed = !tasks[selected_task].completed;  

    mvprintw(27, 0, "Task status toggled successfully!     ");
    refresh();
}

void toggle_subtask_status() { 
    if (task_count == 0) {
        mvprintw(27, 0, "No tasks available to toggle a subtask. ");
        refresh();
        return;
    }

    Task *current_task = &tasks[selected_task];
    if (current_task->subtask_count == 0) {
        mvprintw(27, 0, "No subtasks available to toggle.         ");
        refresh();
        return;
    }

    Subtask *current_subtask = &current_task->subtasks[selected_subtask];
    current_subtask->completed = !current_subtask->completed;

    mvprintw(27, 0, "Subtask status toggled successfully!     ");
    refresh();
}

void display_tasks() { 
    werase(task_win);
    box(task_win, 0, 0);
    mvwprintw(task_win, 0, 2, "Tasks");

    start_color(); 
    init_pair(2, COLOR_BLACK, COLOR_BLUE); 

    for (int i = 0; i < task_count; i++) {
        if (i == selected_task && !in_subtask_mode) {
            wattron(task_win, COLOR_PAIR(2)); 
        }
        mvwprintw(task_win, i + 1, 2, "%d. [%c] %s",
                  tasks[i].priority,
                  tasks[i].completed ? 'x' : ' ',
                  tasks[i].name);

        if (i == selected_task && !in_subtask_mode) {
            wattroff(task_win, COLOR_PAIR(2));
        }
    }
    wrefresh(task_win);
}

void display_subtasks() { 
    werase(subtask_win);
    box(subtask_win, 0, 0);
    mvwprintw(subtask_win, 0, 2, "Subtasks");

    start_color(); 
    init_pair(2, COLOR_BLACK, COLOR_BLUE); 

    if (task_count == 0) {
        mvwprintw(subtask_win, 1, 2, "No tasks available.");
    } else {
        Task *current_task = &tasks[selected_task];
        for (int i = 0; i < current_task->subtask_count; i++) {
            if (i == selected_subtask && in_subtask_mode) {
                wattron(subtask_win, COLOR_PAIR(2)); 
            }
            mvwprintw(subtask_win, i + 1, 2, "%d. [%c] %s",
                      i + 1,
                      current_task->subtasks[i].completed ? 'x' : ' ', 
                      current_task->subtasks[i].name);
            if (i == selected_subtask && in_subtask_mode) {
                wattroff(subtask_win, COLOR_PAIR(2));
            }
        }
    }

    wrefresh(subtask_win);
}

void display_metadata() { 
    werase(category_win);
    box(category_win, 0, 0);
    mvwprintw(category_win, 0, 2, "Categories");

    werase(deadline_win);
    box(deadline_win, 0, 0);
    mvwprintw(deadline_win, 0, 2, "Deadline");

    werase(description_win);
    box(description_win, 0, 0);
    mvwprintw(description_win, 0, 2, "Description");

    start_color(); 
    init_pair(2, COLOR_BLACK, COLOR_BLUE);

    if (task_count > 0) {
        Task *current_task = &tasks[selected_task];
        for (int i = 0; i < current_task->category_count; i++) {
            if (i == selected_category) {
                wattron(category_win, COLOR_PAIR(2)); 
                mvwprintw(category_win, i + 1, 2, "- %s", current_task->categories[i]);
                wattroff(category_win, COLOR_PAIR(2));
            } else {
                mvwprintw(category_win, i + 1, 2, "- %s", current_task->categories[i]);
            }
        }
        mvwprintw(deadline_win, 1, 2, "%s", current_task->due_date);
        mvwprintw(description_win, 1, 2, "%s", current_task->description);
    }

    wrefresh(category_win);
    wrefresh(deadline_win);
    wrefresh(description_win);
}

void manage_categories() { 
    static int category_mode = 0; 
    if (!category_mode) {
        category_mode = 1;
        mvprintw(27, 0, "Category mode enabled. Use 'a' to add, 'd' to delete, 'j/k' to navigate. Press 'c' to exit.");
    } else {
        category_mode = 0;
        mvprintw(27, 0, "Category mode disabled. Returning to task view.");
    }
    refresh();

    while (category_mode) {
        int ch = getch();
        switch (ch) {
            case 'a': 
                if (tasks[selected_task].category_count >= MAX_CATEGORIES) {
                    mvprintw(27, 0, "Category limit reached for this task.");
                } else {
                    echo();
                    curs_set(1);
                    mvprintw(28, 0, "Enter new category: ");
                    char new_category[CATEGORY_NAME_LENGTH]; 
                    getnstr(new_category, CATEGORY_NAME_LENGTH - 1); 
                    strncpy(tasks[selected_task].categories[tasks[selected_task].category_count], new_category, CATEGORY_NAME_LENGTH - 1);
                    tasks[selected_task].category_count++;

                    noecho();
                    curs_set(0);
                    mvprintw(27, 0, "Category added successfully!");
                }
                break;

            case 'd': 
                if (tasks[selected_task].category_count == 0) {
                    mvprintw(27, 0, "No categories to delete.");
                } else {
                    for (int i = selected_category; i < tasks[selected_task].category_count - 1; i++) {
                        strncpy(tasks[selected_task].categories[i], tasks[selected_task].categories[i + 1], CATEGORY_NAME_LENGTH);
                    }
                    tasks[selected_task].category_count--;
                    if (selected_category >= tasks[selected_task].category_count && tasks[selected_task].category_count > 0) {
                        selected_category = tasks[selected_task].category_count - 1;
                    }
                    mvprintw(27, 0, "Category deleted successfully!");
                }
                break;

            case 'j': 
                if (selected_category < tasks[selected_task].category_count - 1) {
                    selected_category++;
                }
                break;

            case 'k':
                if (selected_category > 0) {
                    selected_category--;
                }
                break;

            case 'c': 
                category_mode = 0;
                mvprintw(27, 0, "Exiting category mode...");
                break;
        }

        display_metadata(); 
    }
}

void swap(Task *a, Task *b) {
    Task temp = *a;
    *a = *b;
    *b = temp;
}

void sort_tasks_by_name(Task *task_list, int count) { 
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcmp(task_list[i].name, task_list[j].name) > 0) {
                swap(&task_list[i], &task_list[j]);
            }
        }
    }
}

void sort_tasks_by_deadline(Task *task_list, int count) { 
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            int day_a, month_a, year_a;
            int day_b, month_b, year_b;

            sscanf(task_list[i].due_date, "%d/%d/%d", &day_a, &month_a, &year_a);
            sscanf(task_list[j].due_date, "%d/%d/%d", &day_b, &month_b, &year_b);

            if (year_a != year_b) {
                if (year_a > year_b) {
                    swap(&task_list[i], &task_list[j]);
                }
            }
            else if (month_a != month_b) {
                if (month_a > month_b) {
                    swap(&task_list[i], &task_list[j]);
                }
            }
            else if (day_a != day_b) {
                if (day_a > day_b) {
                    swap(&task_list[i], &task_list[j]);
                }
            }
        }
    }
}

void sort_tasks_by_priority(Task *task_list, int count) { 
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (task_list[i].priority > task_list[j].priority) {
                swap(&task_list[i], &task_list[j]);
            }
        }
    }
}

void sort_task_list() { 
    mvprintw(27, 0, "Sort by: 'n' (name), 'd' (deadline), 'p' (priority): ");
    refresh();

    char choice = getch();
    switch (choice) {
        case 'n':
            sort_tasks_by_name(tasks, task_count);
            mvprintw(27, 0, "Tasks sorted by name!                                ");
            break;
        case 'd':
            sort_tasks_by_deadline(tasks, task_count);
            mvprintw(27, 0, "Tasks sorted by deadline!                           ");
            break;
        case 'p':
            sort_tasks_by_priority(tasks, task_count);
            mvprintw(27, 0, "Tasks sorted by priority!                           ");
            break;
        default:
            mvprintw(27, 0, "Invalid sort choice.                                 ");
            return;
    }

    refresh();
}

void update_task_name() { 
    if (task_count == 0) {
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter new task name: ");
    getnstr(tasks[selected_task].name, TASK_NAME_LENGTH - 1);
    noecho();
    curs_set(0);
    mvprintw(27, 0, "Task name updated successfully!");
    refresh();
}

void update_task_description() { 
    if (task_count == 0) {
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter new description: ");
    getnstr(tasks[selected_task].description, 99);
    noecho();
    curs_set(0);
    mvprintw(27, 0, "Task description updated successfully!");
    refresh();
}

void change_due_date() { 
    if (task_count == 0) {
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    char new_due_date[11];
    echo();
    curs_set(1);
    do {
        mvprintw(27, 0, "Enter new deadline (DD/MM/YYYY): ");
        getnstr(new_due_date, 10);
        if (!validate_date_format(new_due_date)) {
            mvprintw(28, 0, "Invalid date format. Try again.");
        }
    } while (!validate_date_format(new_due_date));

    strncpy(tasks[selected_task].due_date, new_due_date, 10);
    noecho();
    curs_set(0);
    mvprintw(27, 0, "Deadline updated successfully!");
    refresh();
}

void save_tasks(const char *filename) { 
    cJSON *json_array = cJSON_CreateArray(); 

    for (int i = 0; i < task_count; i++) {
        cJSON *json_task = cJSON_CreateObject(); 
        cJSON_AddStringToObject(json_task, "name", tasks[i].name);
        cJSON_AddNumberToObject(json_task, "priority", tasks[i].priority);
        cJSON_AddStringToObject(json_task, "description", tasks[i].description);
        cJSON_AddStringToObject(json_task, "deadline", tasks[i].due_date);
        
        cJSON *json_categories = cJSON_CreateArray();
        for (int j = 0; j < tasks[i].category_count; j++) {
            cJSON_AddItemToArray(json_categories, cJSON_CreateString(tasks[i].categories[j]));
        }
        cJSON_AddItemToObject(json_task, "categories", json_categories);

        cJSON *json_subtasks = cJSON_CreateArray();
        for (int j = 0; j < tasks[i].subtask_count; j++) {
            cJSON *json_subtask = cJSON_CreateObject();
            cJSON_AddStringToObject(json_subtask, "name", tasks[i].subtasks[j].name);
            cJSON_AddStringToObject(json_subtask, "status", tasks[i].subtasks[j].completed ? "done" : "pending");
            cJSON_AddItemToArray(json_subtasks, json_subtask);
        }
        cJSON_AddItemToObject(json_task, "subtasks", json_subtasks);

        cJSON_AddItemToArray(json_array, json_task);
    }

    char *json_string = cJSON_Print(json_array); 
    FILE *file = fopen(filename, "w"); 
    if (file) {
        fputs(json_string, file);
        fclose(file);
    }

    cJSON_Delete(json_array);
    free(json_string);
    mvprintw(27, 0, "Tasks saved to file.");
    refresh();
}

void load_tasks(const char *filename) { 
    FILE *file = fopen(filename, "r"); 
    if (!file) {
        mvprintw(27, 0, "No file found to load tasks.");
        refresh();
        return;
    }
   
    fseek(file, 0, SEEK_END); 
    long length = ftell(file);
    fseek(file, 0, SEEK_SET); 
    char *json_string = malloc(length + 1); 
    fread(json_string, 1, length, file);
    fclose(file);
    json_string[length] = '\0';

    cJSON *json_array = cJSON_Parse(json_string); 
    if (!json_array) {
        mvprintw(27, 0, "Failed to parse tasks from file.");
        free(json_string);
        refresh();
        return;
    }

    task_count = 0;

    cJSON *json_task = NULL;
    cJSON_ArrayForEach(json_task, json_array) { 
        cJSON *name = cJSON_GetObjectItem(json_task, "name");
        cJSON *priority = cJSON_GetObjectItem(json_task, "priority");
        cJSON *description = cJSON_GetObjectItem(json_task, "description");
        cJSON *deadline = cJSON_GetObjectItem(json_task, "deadline");
        cJSON *categories = cJSON_GetObjectItem(json_task, "categories");
        cJSON *subtasks = cJSON_GetObjectItem(json_task, "subtasks");

        if (priority && name && description && deadline && categories && subtasks) { 
            tasks[task_count].priority = priority->valueint;
            strncpy(tasks[task_count].name, name->valuestring, TASK_NAME_LENGTH - 1);
            strncpy(tasks[task_count].description, description->valuestring, 99);
            strncpy(tasks[task_count].due_date, deadline->valuestring, 10);

            tasks[task_count].category_count = 0;
            cJSON *category = NULL;
            cJSON_ArrayForEach(category, categories) {
                if (tasks[task_count].category_count < MAX_CATEGORIES) {
                    strncpy(tasks[task_count].categories[tasks[task_count].category_count++], category->valuestring, CATEGORY_NAME_LENGTH - 1);
                }
            }

            tasks[task_count].subtask_count = 0;
            cJSON *json_subtask = NULL;
            cJSON_ArrayForEach(json_subtask, subtasks) {
                cJSON *subtask_name = cJSON_GetObjectItem(json_subtask, "name");
                cJSON *subtask_status = cJSON_GetObjectItem(json_subtask, "status");

                if (subtask_name && subtask_status && tasks[task_count].subtask_count < MAX_SUBTASKS) {
                    strncpy(tasks[task_count].subtasks[tasks[task_count].subtask_count].name, subtask_name->valuestring, SUBTASK_NAME_LENGTH - 1);
                    tasks[task_count].subtasks[tasks[task_count].subtask_count].completed = strcmp(subtask_status->valuestring, "done") == 0;
                    tasks[task_count].subtask_count++;
                }
            }

            task_count++;
        }
    }

    cJSON_Delete(json_array);
    free(json_string);
    mvprintw(27, 0, "Tasks loaded from file.");
    refresh();
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    load_tasks("tasks.json");
    setup_windows();

    char input;
    while ((input = getch()) != 'q') {
        switch (input) {
            case 'a': 
                if (in_subtask_mode) {
                    add_subtask();
                } else {
                    add_task();
                }
                break;
            case 'd': 
                if (in_subtask_mode) {
                    delete_subtask();
                } else {
                    delete_task();
                }
                break;
            case 'j': 
                if (in_subtask_mode) {
                    if (selected_subtask < tasks[selected_task].subtask_count - 1) {
                        selected_subtask++;
                    }
                } else if (selected_task < task_count - 1) {
                    selected_task++;
                }
                break;
            case 'k': 
                if (in_subtask_mode) {
                    if (selected_subtask > 0) {
                        selected_subtask--;
                    }
                } else if (selected_task > 0) {
                    selected_task--;
                }
                break;
            case 'l': 
                if (!in_subtask_mode && task_count > 0) {
                    in_subtask_mode = 1;
                    selected_subtask = 0;
                }
                break;
            case 'h': 
                if (in_subtask_mode) {
                    in_subtask_mode = 0;
                }
                break;
            case ' ': 
                if (in_subtask_mode) {
                    toggle_subtask_status();
                } else {
                    toggle_task_status();
                }
                break;
            case 's':  
                sort_task_list();
                display_tasks();
                display_metadata();
                break;
            case 'e':
                update_task_name();
                break;
            case 'r':
                update_task_description();
                break;
            case 'n':
                change_due_date();
                break;
            case 'c':
                manage_categories();
                break;
            case 'w': 
                save_tasks("tasks.json");
                break;
            case 'x': 
                load_tasks("tasks.json");
                display_metadata();
                break;
        }
        display_tasks();
        display_subtasks();
        display_metadata();
    }

    endwin();
    return 0;
}
