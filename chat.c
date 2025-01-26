#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <time.h> // برای استفاده از strptime

#define MAX_TASKS 100
#define MAX_SUBTASKS 50
#define TASK_NAME_LENGTH 50
#define SUBTASK_NAME_LENGTH 50
#define MAX_CATEGORIES 10
#define CATEGORY_NAME_LENGTH 30

typedef struct {
    char title[SUBTASK_NAME_LENGTH];
    int is_done; 
} Subtask;

typedef struct {
    char title[TASK_NAME_LENGTH];
    int is_done; 
    int priority_level;
    char due_date[11];
    char details[100];
    char tags[MAX_CATEGORIES][CATEGORY_NAME_LENGTH];
    int tag_count;
    Subtask sub_items[MAX_SUBTASKS];
    int sub_item_count;
} Task;

Task task_list[MAX_TASKS];
int total_tasks = 0; 
int current_task_index = 0; 
int current_subtask_index = 0;
int is_subtask_mode = 0; 
int current_category_index = 0;

WINDOW *task_window, *subtask_window, *category_window, *deadline_window, *description_window;

void initialize_windows() {
    clear();
    refresh();

    task_window = newwin(15, 40, 0, 0);
    box(task_window, 0, 0);
    mvwprintw(task_window, 0, 2, "Tasks");
    wrefresh(task_window);

    category_window = newwin(10, 40, 15, 0);
    box(category_window, 0, 0);
    mvwprintw(category_window, 0, 2, "Categories");
    wrefresh(category_window);

    description_window = newwin(10, 40, 0, 40);
    box(description_window, 0, 0);
    mvwprintw(description_window, 0, 2, "Description");
    wrefresh(description_window);

    subtask_window = newwin(10, 40, 10, 40);
    box(subtask_window, 0, 0);
    mvwprintw(subtask_window, 0, 2, "Subtasks");
    wrefresh(subtask_window);

    deadline_window = newwin(5, 40, 20, 40);
    box(deadline_window, 0, 0);
    mvwprintw(deadline_window, 0, 2, "Deadline");
    wrefresh(deadline_window);

    mvprintw(25, 0, "Keys: 'q' to quit, 'a' to add task, 'j'/'k' to navigate, 'd' to delete, 'SPACE' to toggle status, 's' to sort, 'l' to point subtasks, 'h' to back task, 'e' to edit task's name, 'r' to edit task's desciption, 'n' to add new deadline, 'c' to edit categories, 'w' to save, 'x' to retrive.");
    refresh();
}

int check_date_format(const char *date) { 
    struct tm tm;
    return strptime(date, "%d/%m/%Y", &tm) != NULL;
}

void create_task() { 
    if (total_tasks >= MAX_TASKS) {
        mvprintw(27, 0, "Task limit reached. Cannot add more tasks.");
        refresh();
        return;
    }

    char task_title[TASK_NAME_LENGTH];
    char tag[CATEGORY_NAME_LENGTH];
    char due_date[11];
    char details[100];
    int tag_count;
    int priority_level;

    echo(); 
    curs_set(1); 

    mvprintw(27, 0, "Enter task name: ");
    getnstr(task_title, TASK_NAME_LENGTH - 1); 

    mvprintw(28, 0, "Enter number of categories (max %d): ", MAX_CATEGORIES);
    scanw("%d", &tag_count); 

    Task *new_task = &task_list[total_tasks++];

    if (tag_count > MAX_CATEGORIES) {
        new_task->tag_count = MAX_CATEGORIES;
    } else {
        new_task->tag_count = tag_count;
    }

    for (int i = 0; i < new_task->tag_count; i++) {
        mvprintw(29 + i, 0, "Enter category %d: ", i + 1);
        getnstr(tag, CATEGORY_NAME_LENGTH - 1);
        strncpy(new_task->tags[i], tag, CATEGORY_NAME_LENGTH - 1);
    }

    do {
        mvprintw(29 + tag_count, 0, "Enter deadline (DD/MM/YYYY): ");
        getnstr(due_date, 10);
        if (!check_date_format(due_date)) {
            mvprintw(30 + tag_count, 0, "Invalid date format. Try again.   ");
        }
    } while (!check_date_format(due_date));

    mvprintw(31 + tag_count, 0, "Enter description: ");
    getnstr(details, 99);

    mvprintw(32 + tag_count, 0, "Enter priority (1-9): ");
    scanw("%d", &priority_level);
    if (priority_level < 1 || priority_level > 9) priority_level = 1; 

    noecho(); 
    curs_set(0); 

    strncpy(new_task->title, task_title, TASK_NAME_LENGTH - 1);
    strncpy(new_task->due_date, due_date, 10);
    strncpy(new_task->details, details, 99);
    new_task->is_done = 0;
    new_task->priority_level = priority_level;
    new_task->sub_item_count = 0;

    mvprintw(27, 0, "                             Task added successfully!                             ");
    refresh();
}

void create_subtask() { 
    if (total_tasks == 0) {
        mvprintw(27, 0, "No tasks available to add a subtask.     ");
        refresh();
        return;
    }

    Task *current_task = &task_list[current_task_index];
    if (current_task->sub_item_count >= MAX_SUBTASKS) {
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

    Subtask *new_subtask = &current_task->sub_items[current_task->sub_item_count++];
    strncpy(new_subtask->title, subtask_title, SUBTASK_NAME_LENGTH - 1);
    new_subtask->is_done = 0; 

    mvprintw(27, 0, "Subtask added successfully!             ");
    refresh();
}

void remove_task() { 
    if (total_tasks == 0) {
        mvprintw(27, 0, "No tasks available to delete a subtask. ");
        refresh();
        return;
    }

    if (total_tasks > 0) {
        for (int i = current_task_index; i < total_tasks - 1; i++) {
            task_list[i] = task_list[i + 1];
        }
        total_tasks--;
        if (current_task_index >= total_tasks && total_tasks > 0) {
            current_task_index = total_tasks - 1;
        }
    }

    mvprintw(27, 0, "Subtask deleted successfully!           ");
    refresh();
}

void remove_subtask() { 
    if (total_tasks == 0) {
        mvprintw(27, 0, "No tasks available to delete a subtask. ");
        refresh();
        return;
    }

    Task *current_task = &task_list[current_task_index];
    if (current_task->sub_item_count == 0) {
        mvprintw(27, 0, "No subtasks available to delete.         ");
        refresh();
        return;
    }

    for (int i = current_subtask_index; i < current_task->sub_item_count - 1; i++) {
        current_task->sub_items[i] = current_task->sub_items[i + 1];
    }
    current_task->sub_item_count--;
    if (current_subtask_index >= current_task->sub_item_count && current_task->sub_item_count > 0) {
        current_subtask_index = current_task->sub_item_count - 1;
    }

    mvprintw(27, 0, "Subtask deleted successfully!           ");
    refresh();
}

void toggle_task_completion() { 
    if (total_tasks == 0) {
        mvprintw(27, 0, "No tasks available to toggle a subtask. ");
        refresh();
        return;
    }

    if (total_tasks > 0) {
        task_list[current_task_index].is_done = !task_list[current_task_index].is_done;  
    } 

    mvprintw(27, 0, "Subtask status toggled successfully!     ");
    refresh();
}

void toggle_subtask_completion() { 
    if (total_tasks == 0) {
        mvprintw(27, 0, "No tasks available to toggle a subtask. ");
        refresh();
        return;
    }

    Task *current_task = &task_list[current_task_index];
    if (current_task->sub_item_count == 0) {
        mvprintw(27, 0, "No subtasks available to toggle.         ");
        refresh();
        return;
    }

    Subtask *current_subtask = &current_task->sub_items[current_subtask_index];
    current_subtask->is_done = !current_subtask->is_done;

    mvprintw(27, 0, "Subtask status toggled successfully!     ");
    refresh();
}

void show_tasks() { 
    werase(task_window);
    box(task_window, 0, 0);
    mvwprintw(task_window, 0, 2, "Tasks");

    start_color(); 
    init_pair(2, COLOR_BLACK, COLOR_BLUE); 

    for (int i = 0; i < total_tasks; i++) {
        if (i == current_task_index && !is_subtask_mode) {
            wattron(task_window, COLOR_PAIR(2)); 
        }
        mvwprintw(task_window, i + 1, 2, "%d. [%c] %s",
                  task_list[i].priority_level,
                  task_list[i].is_done ? 'x' : ' ',
                  task_list[i].title);

        if (i == current_task_index && !is_subtask_mode) {
            wattroff(task_window, COLOR_PAIR(2));
        }
    }
    wrefresh(task_window);
}

void show_subtasks() { 
    werase(subtask_window);
    box(subtask_window, 0, 0);
    mvwprintw(subtask_window, 0, 2, "Subtasks");

    start_color(); 
    init_pair(2, COLOR_BLACK, COLOR_BLUE); 

    if (total_tasks == 0) {
        mvwprintw(subtask_window, 1, 2, "No tasks available.");
    } else {
        Task *current_task = &task_list[current_task_index];
        for (int i = 0; i < current_task->sub_item_count; i++) {
            if (i == current_subtask_index && is_subtask_mode) {
                wattron(subtask_window, COLOR_PAIR(2)); 
            }
            mvwprintw(subtask_window, i + 1, 2, "%d. [%c] %s",
                      i + 1,
                      current_task->sub_items[i].is_done ? 'x' : ' ', 
                      current_task->sub_items[i].title);
            if (i == current_subtask_index && is_subtask_mode) {
                wattroff(subtask_window, COLOR_PAIR(2));
            }
        }
    }

    wrefresh(subtask_window);
}

void show_metadata() { 
    werase(category_window);
    box(category_window, 0, 0);
    mvwprintw(category_window, 0, 2, "Categories");

    werase(deadline_window);
    box(deadline_window, 0, 0);
    mvwprintw(deadline_window, 0, 2, "Deadline");

    werase(description_window);
    box(description_window, 0, 0);
    mvwprintw(description_window, 0, 2, "Description");

    start_color(); 
    init_pair(2, COLOR_BLACK, COLOR_BLUE);

    if (total_tasks > 0) {
        Task *current_task = &task_list[current_task_index];
        for (int i = 0; i < current_task->tag_count; i++) {
            if (i == current_category_index) {
                wattron(category_window, COLOR_PAIR(2)); 
                mvwprintw(category_window, i + 1, 2, "- %s", current_task->tags[i]);
                wattroff(category_window, COLOR_PAIR(2));
            } else {
                mvwprintw(category_window, i + 1, 2, "- %s", current_task->tags[i]);
            }
        }
        mvwprintw(deadline_window, 1, 2, "%s", current_task->due_date);
        mvwprintw(description_window, 1, 2, "%s", current_task->details);
    }

    wrefresh(category_window);
    wrefresh(deadline_window);
    wrefresh(description_window);
}

void manage_tags() { 
    static int is_tag_mode = 0; 
    if (!is_tag_mode) {
        is_tag_mode = 1;
        mvprintw(27, 0, "Category mode enabled. Use 'a' to add, 'd' to delete, 'j/k' to navigate. Press 'c' to exit.");
    } else {
        is_tag_mode = 0;
        mvprintw(27, 0, "Category mode disabled. Returning to task view.");
    }
    refresh();

    while (is_tag_mode) {
        int ch = getch();
        switch (ch) {
            case 'a': 
                if (task_list[current_task_index].tag_count >= MAX_CATEGORIES) {
                    mvprintw(27, 0, "Category limit reached for this task.");
                } else {
                    echo();
                    curs_set(1);
                    mvprintw(28, 0, "Enter new category: ");
                    char new_tag[CATEGORY_NAME_LENGTH]; 
                    getnstr(new_tag, CATEGORY_NAME_LENGTH - 1); 
                    strncpy(task_list[current_task_index].tags[task_list[current_task_index].tag_count], new_tag, CATEGORY_NAME_LENGTH - 1);
                    task_list[current_task_index].tag_count++;

                    noecho();
                    curs_set(0);
                    mvprintw(27, 0, "Category added successfully!");
                }
                break;

            case 'd': 
                if (task_list[current_task_index].tag_count == 0) {
                    mvprintw(27, 0, "No categories to delete.");
                } else {
                    for (int i = current_category_index; i < task_list[current_task_index].tag_count - 1; i++) {
                        strncpy(task_list[current_task_index].tags[i], task_list[current_task_index].tags[i + 1], CATEGORY_NAME_LENGTH);
                    }
                    task_list[current_task_index].tag_count--;
                    if (current_category_index >= task_list[current_task_index].tag_count && task_list[current_task_index].tag_count > 0) {
                        current_category_index = task_list[current_task_index].tag_count - 1;
                    }
                    mvprintw(27, 0, "Category deleted successfully!");
                }
                break;

            case 'j': 
                if (current_category_index < task_list[current_task_index].tag_count - 1) {
                    current_category_index++;
                }
                break;

            case 'k':
                if (current_category_index > 0) {
                    current_category_index--;
                }
                break;

            case 'c': 
                is_tag_mode = 0;
                mvprintw(27, 0, "Exiting category mode...");
                break;
        }

        show_metadata(); 
    }
}

void swap_tasks(Task *a, Task *b) {
    Task temp = *a;
    *a = *b;
    *b = temp;
}

void sort_tasks_by_title(Task *task_list, int n) { 
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (strcmp(task_list[i].title, task_list[j].title) > 0) {
                swap_tasks(&task_list[i], &task_list[j]);
            }
        }
    }
}

void sort_tasks_by_due_date(Task *task_list, int n) { 
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            struct tm tm1, tm2;
            strptime(task_list[i].due_date, "%d/%m/%Y", &tm1);
            strptime(task_list[j].due_date, "%d/%m/%Y", &tm2);

            if (difftime(mktime(&tm1), mktime(&tm2)) > 0) {
                swap_tasks(&task_list[i], &task_list[j]);
            }
        }
    }
}

void sort_tasks_by_priority_level(Task *task_list, int n) { 
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (task_list[i].priority_level > task_list[j].priority_level) {
                swap_tasks(&task_list[i], &task_list[j]);
            }
        }
    }
}

void sort_task_list() { 
    mvprintw(27, 0, "Sort by: 'n' (name), 'd' (deadline), 'p' (priority): ");
    refresh();

    char sort_choice = getch();
    switch (sort_choice) {
        case 'n':
            sort_tasks_by_title(task_list, total_tasks);
            mvprintw(27, 0, "Tasks sorted by name!                                ");
            break;
        case 'd':
            sort_tasks_by_due_date(task_list, total_tasks);
            mvprintw(27, 0, "Tasks sorted by deadline!                           ");
            break;
        case 'p':
            sort_tasks_by_priority_level(task_list, total_tasks);
            mvprintw(27, 0, "Tasks sorted by priority!                           ");
            break;
        default:
            mvprintw(27, 0, "Invalid sort choice.                                 ");
            return;
    }

    refresh();
}

void edit_task_title() { 
    if (total_tasks == 0) {
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter new task name: ");
    getnstr(task_list[current_task_index].title, TASK_NAME_LENGTH - 1);
    noecho();
    curs_set(0);
    mvprintw(27, 0, "Task name updated successfully!");
    refresh();
}

void edit_task_details() { 
    if (total_tasks == 0) {
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter new description: ");
    getnstr(task_list[current_task_index].details, 99);
    noecho();
    curs_set(0);
    mvprintw(27, 0, "Task description updated successfully!");
    refresh();
}

void update_due_date() { 
    if (total_tasks == 0) {
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
        if (!check_date_format(new_due_date)) {
            mvprintw(28, 0, "Invalid date format. Try again.");
        }
    } while (!check_date_format(new_due_date));

    strncpy(task_list[current_task_index].due_date, new_due_date, 10);
    noecho();
    curs_set(0);
    mvprintw(27, 0, "Deadline updated successfully!");
    refresh();
}

void save_tasks_to_file(const char *filename) { 
    cJSON *json_root = cJSON_CreateArray(); 

    for (int i = 0; i < total_tasks; i++) {
        cJSON *json_task = cJSON_CreateObject(); 
        cJSON_AddStringToObject(json_task, "name", task_list[i].title);
        cJSON_AddNumberToObject(json_task, "priority", task_list[i].priority_level);
        cJSON_AddStringToObject(json_task, "description", task_list[i].details);
        cJSON_AddStringToObject(json_task, "deadline", task_list[i].due_date);
        
        cJSON *json_categories = cJSON_CreateArray();
        for (int j = 0; j < task_list[i].tag_count; j++) {
            cJSON_AddItemToArray(json_categories, cJSON_CreateString(task_list[i].tags[j]));
        }
        cJSON_AddItemToObject(json_task, "categories", json_categories);

        cJSON *json_subtasks = cJSON_CreateArray();
        for (int j = 0; j < task_list[i].sub_item_count; j++) {
            cJSON *json_subtask = cJSON_CreateObject();
            cJSON_AddStringToObject(json_subtask, "name", task_list[i].sub_items[j].title);
            cJSON_AddStringToObject(json_subtask, "status", task_list[i].sub_items[j].is_done ? "done" : "pending");
            cJSON_AddItemToArray(json_subtasks, json_subtask);
        }
        cJSON_AddItemToObject(json_task, "subtasks", json_subtasks);

        cJSON_AddItemToArray(json_root, json_task);
    }

    char *json_string = cJSON_Print(json_root); 
    FILE *file = fopen(filename, "w"); 
    if (file) {
        fputs(json_string, file);
        fclose(file);
    }

    cJSON_Delete(json_root);
    free(json_string);
    mvprintw(27, 0, "Tasks saved to file.");
    refresh();
}

void load_tasks_from_file(const char *filename) { 
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

    cJSON *json_root = cJSON_Parse(json_string); 
    if (!json_root) {
        mvprintw(27, 0, "Failed to parse tasks from file.");
        free(json_string);
        refresh();
        return;
    }

    total_tasks = 0;

    cJSON *json_task = NULL;
    cJSON_ArrayForEach(json_task, json_root) { 
        cJSON *name = cJSON_GetObjectItem(json_task, "name");
        cJSON *priority = cJSON_GetObjectItem(json_task, "priority");
        cJSON *description = cJSON_GetObjectItem(json_task, "description");
        cJSON *deadline = cJSON_GetObjectItem(json_task, "deadline");
        cJSON *categories = cJSON_GetObjectItem(json_task, "categories");
        cJSON *subtasks = cJSON_GetObjectItem(json_task, "subtasks");

        if (priority && name && description && deadline && categories && subtasks) {
            task_list[total_tasks].priority_level = priority->valueint;
            strncpy(task_list[total_tasks].title, name->valuestring, TASK_NAME_LENGTH - 1);
            strncpy(task_list[total_tasks].details, description->valuestring, 99);
            strncpy(task_list[total_tasks].due_date, deadline->valuestring, 10);

            task_list[total_tasks].tag_count = 0;
            cJSON *category = NULL;
            cJSON_ArrayForEach(category, categories) {
                if (task_list[total_tasks].tag_count < MAX_CATEGORIES) {
                    strncpy(task_list[total_tasks].tags[task_list[total_tasks].tag_count++], category->valuestring, CATEGORY_NAME_LENGTH - 1);
                }
            }

            task_list[total_tasks].sub_item_count = 0;
            cJSON *json_subtask = NULL;
            cJSON_ArrayForEach(json_subtask, subtasks) {
                cJSON *subtask_name = cJSON_GetObjectItem(json_subtask, "name");
                cJSON *subtask_status = cJSON_GetObjectItem(json_subtask, "status");

                if (subtask_name && subtask_status && task_list[total_tasks].sub_item_count < MAX_SUBTASKS) {
                    strncpy(task_list[total_tasks].sub_items[task_list[total_tasks].sub_item_count].title, subtask_name->valuestring, SUBTASK_NAME_LENGTH - 1);
                    task_list[total_tasks].sub_items[task_list[total_tasks].sub_item_count].is_done = strcmp(subtask_status->valuestring, "done") == 0;
                    task_list[total_tasks].sub_item_count++;
                }
            }

            total_tasks++;
        }
    }

    cJSON_Delete(json_root);
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

    initialize_windows();

    char ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case 'a': 
                if (is_subtask_mode) {
                    create_subtask();
                } else {
                    create_task();
                }
                break;
            case 'd': 
                if (is_subtask_mode) {
                    remove_subtask();
                } else {
                    remove_task();
                }
                break;
            case 'j': 
                if (is_subtask_mode) {
                    if (current_subtask_index < task_list[current_task_index].sub_item_count - 1) {
                        current_subtask_index++;
                    }
                } else if (current_task_index < total_tasks - 1) {
                    current_task_index++;
                }
                break;
            case 'k': 
                if (is_subtask_mode) {
                    if (current_subtask_index > 0) {
                        current_subtask_index--;
                    }
                } else if (current_task_index > 0) {
                    current_task_index--;
                }
                break;
            case 'l': 
                if (!is_subtask_mode && total_tasks > 0) {
                    is_subtask_mode = 1;
                    current_subtask_index = 0;
                }
                break;
            case 'h': 
                if (is_subtask_mode) {
                    is_subtask_mode = 0;
                }
                break;
            case ' ': 
                if (is_subtask_mode) {
                    toggle_subtask_completion();
                } else {
                    toggle_task_completion();
                }
                break;
            case 's': 
                sort_task_list();
                show_tasks();
                show_metadata();
                break;
            case 'e':
                edit_task_title();
                break;
            case 'r':
                edit_task_details();
                break;
            case 'n':
                update_due_date();
                break;
            case 'c':
                manage_tags();
                break;
            case 'w': 
                save_tasks_to_file("tasks.json");
                break;
            case 'x': 
                load_tasks_from_file("tasks.json");
                show_metadata();
                break;
        }
        show_tasks();
        show_subtasks();
        show_metadata();
    }

    endwin();
    return 0;
}
