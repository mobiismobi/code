#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <cjson/cJSON.h>
#include <stdio.h>

typedef struct {
    char title[50]; // SUBTASK_NAME_LENGTH
    int is_done; 
} Subtask;

typedef struct {
    char title[50]; // TASK_NAME_LENGTH
    int is_done; 
    int priority_level;
    char due_date[11];
    char details[100];
    char **tags; // Dynamic array for categories
    int tag_count;
    Subtask sub_items[50]; // MAX_SUBTASKS
    int sub_item_count;
} Task;

Task task_list[100]; // MAX_TASKS
int total_tasks = 0; 
int current_task_index = 0; 
int current_subtask_index = 0;
int is_subtask_mode = 0; 
int current_category_index = 0;

WINDOW *task_window, *subtask_window, *category_window, *deadline_window, *description_window;

void clear_message_area() {
    mvprintw(27, 0, "                                                    "); // Clear line 27
    mvprintw(28, 0, "                                                    "); // Clear line 28
    mvprintw(29, 0, "                                                    "); // Clear line 29
}

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

    mvprintw(25, 0, "Keys: 'q' to quit, 'a' to add task, 'j'/'k' to navigate, 'd' to delete, 'SPACE' to toggle status, 's' to sort, 'l' to view subtasks, 'h' to go back to tasks, 'e' to edit task name, 'r' to edit task description, 'n' to set new deadline, 'c' to edit categories, 'w' to save, 'x' to load.");
    refresh();
}

int check_date_format(const char *date) {
    int day, month, year;
    if (sscanf(date, "%d/%d/%d", &day, &month, &year) != 3) {
        return 0; // Invalid format
    }
    
    // Check values
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return 0;
    }

    // Check days in month
    if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) {
        return 0;
    }
    if (month == 2) {
        if ((year % 4 == 0 && day > 29) || (year % 4 != 0 && day > 28)) {
            return 0;
        }
    }

    return 1; // Valid format
}

void create_task() { 
    if (total_tasks >= 100) { // MAX_TASKS
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "Sorry! Task limit reached. Cannot add more tasks.");
        refresh();
        return;
    }

    char task_title[50]; // TASK_NAME_LENGTH
    char due_date[11];
    char details[100];
    int priority_level;

    echo(); 
    curs_set(1); 

    mvprintw(27, 0, "Please enter the task name: ");
    getnstr(task_title, 49); // TASK_NAME_LENGTH - 1 

    Task *new_task = &task_list[total_tasks++];
    new_task->tag_count = 0;
    new_task->tags = NULL; // Initialize the dynamic array

    do {
        char tag[30]; // CATEGORY_NAME_LENGTH
        mvprintw(28 + new_task->tag_count, 0, "Please enter category %d (or type 'done' to finish): ", new_task->tag_count + 1);
        getnstr(tag, 29); // CATEGORY_NAME_LENGTH - 1

        if (strcmp(tag, "done") == 0) {
            break; // Exit if the user types 'done'
        }

        // Reallocate memory for the new category
        new_task->tags = realloc(new_task->tags, sizeof(char*) * (new_task->tag_count + 1));
        new_task->tags[new_task->tag_count] = malloc(strlen(tag) + 1);
        strcpy(new_task->tags[new_task->tag_count], tag);
        new_task->tag_count++;

    } while (1); // Loop until the user types 'done'

    do {
        mvprintw(28 + new_task->tag_count, 0, "Please enter the deadline (DD/MM/YYYY): ");
        getnstr(due_date, 10);
        if (!check_date_format(due_date)) {
            mvprintw(30 + new_task->tag_count, 0, "Invalid date format. Please try again.   ");
        }
    } while (!check_date_format(due_date));

    // Get task description
    mvprintw(31 + new_task->tag_count, 0, "Please enter your description: ");
    getnstr(details, 99);

    // Get task priority
    mvprintw(32 + new_task->tag_count, 0, "Please enter the priority (1-9): ");
    scanw("%d", &priority_level);
    if (priority_level < 1 || priority_level > 9) priority_level = 1; 

    noecho(); 
    curs_set(0); 

    strncpy(new_task->title, task_title, 49); // TASK_NAME_LENGTH - 1
    strncpy(new_task->due_date, due_date, 10);
    strncpy(new_task->details, details, 99);
    new_task->is_done = 0;
    new_task->priority_level = priority_level;
    new_task->sub_item_count = 0;

    clear_message_area(); // Clear previous messages
    mvprintw(27, 0, "Task added successfully!                             ");
    refresh();
}

void create_subtask() { 
    if (total_tasks == 0) {
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "No tasks available to add a subtask.     ");
        refresh();
        return;
    }

    Task *current_task = &task_list[current_task_index];
    if (current_task->sub_item_count >= 50) { // MAX_SUBTASKS
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "Subtask limit reached for this task.     ");
        refresh();
        return;
    }

    char subtask_title[50]; // SUBTASK_NAME_LENGTH
    echo();
    curs_set(1);
    mvprintw(27, 0, "Please enter the subtask name: ");
    getnstr(subtask_title, 49); // SUBTASK_NAME_LENGTH - 1
    noecho();
    curs_set(0);

    Subtask *new_subtask = &current_task->sub_items[current_task->sub_item_count++];
    strncpy(new_subtask->title, subtask_title, 49); // SUBTASK_NAME_LENGTH - 1
    new_subtask->is_done = 0; 

    clear_message_area(); // Clear previous messages
    mvprintw(27, 0, "Subtask added successfully!             ");
    refresh();
}

void remove_task() { 
    if (total_tasks == 0) {
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "No tasks available to delete. ");
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

    clear_message_area(); // Clear previous messages
    mvprintw(27, 0, "Task deleted successfully!           ");
    refresh();
}

void remove_subtask() { 
    if (total_tasks == 0) {
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "No tasks available to delete a subtask. ");
        refresh();
        return;
    }

    Task *current_task = &task_list[current_task_index];
    if (current_task->sub_item_count == 0) {
        clear_message_area(); // Clear previous messages
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

    clear_message_area(); // Clear previous messages
    mvprintw(27, 0, "Subtask deleted successfully!           ");
    refresh();
}

void toggle_task_completion() { 
    if (total_tasks == 0) {
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "No tasks available to toggle completion. ");
        refresh();
        return;
    }

    if (total_tasks > 0) {
        task_list[current_task_index].is_done = !task_list[current_task_index].is_done;  
    } 

    clear_message_area(); // Clear previous messages
    mvprintw(27, 0, "Task completion status toggled successfully!     ");
    refresh();
}

void toggle_subtask_completion() { 
    if (total_tasks == 0) {
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "No tasks available to toggle subtask completion. ");
        refresh();
        return;
    }

    Task *current_task = &task_list[current_task_index];
    if (current_task->sub_item_count == 0) {
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "No subtasks available to toggle.         ");
        refresh();
        return;
    }

    Subtask *current_subtask = &current_task->sub_items[current_subtask_index];
    current_subtask->is_done = !current_subtask->is_done;

    clear_message_area(); // Clear previous messages
    mvprintw(27, 0, "Subtask completion status toggled successfully!     ");
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
        mvwprintw(subtask_window, 1, 2, "No tasks available to display subtasks.");
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

void show_task_metadata() { // Changed function name
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
        mvprintw(27, 0, "Tag management mode enabled. Use 'a' to add, 'd' to delete, 'j/k' to navigate. Press 'c' to exit.");
    } else {
        is_tag_mode = 0;
        mvprintw(27, 0, "Tag management mode disabled. Returning to task view.");
    }
    refresh();

    while (is_tag_mode) {
        int ch = getch();
        switch (ch) {
            case 'a': 
                echo();
                curs_set(1);
                mvprintw(28, 0, "Please enter the new category: ");
                char new_tag[30]; // CATEGORY_NAME_LENGTH
                getnstr(new_tag, 29); // CATEGORY_NAME_LENGTH - 1

                // Reallocate memory for the new category
                task_list[current_task_index].tags = realloc(task_list[current_task_index].tags, sizeof(char*) * (task_list[current_task_index].tag_count + 1));
                task_list[current_task_index].tags[task_list[current_task_index].tag_count] = malloc(strlen(new_tag) + 1);
                strcpy(task_list[current_task_index].tags[task_list[current_task_index].tag_count], new_tag);
                task_list[current_task_index].tag_count++;

                noecho();
                curs_set(0);
                clear_message_area(); // Clear previous messages
                mvprintw(27, 0, "Category added successfully!");
                break;

            case 'd': 
                if (task_list[current_task_index].tag_count == 0) {
                    clear_message_area(); // Clear previous messages
                    mvprintw(27, 0, "No categories available to delete.");
                } else {
                    for (int i = current_category_index; i < task_list[current_task_index].tag_count - 1; i++) {
                        free(task_list[current_task_index].tags[i]); // Free the memory for the category
                        task_list[current_task_index].tags[i] = task_list[current_task_index].tags[i + 1];
                    }
                    free(task_list[current_task_index].tags[task_list[current_task_index].tag_count - 1]); // Free the last category
                    task_list[current_task_index].tag_count--;
                    if (current_category_index >= task_list[current_task_index].tag_count && task_list[current_task_index].tag_count > 0) {
                        current_category_index = task_list[current_task_index].tag_count - 1;
                    }
                    clear_message_area(); // Clear previous messages
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
                mvprintw(27, 0, "Exiting tag management mode...");
                break;
        }

        show_task_metadata(); 
    }
}

// توابع مقایسه برای qsort
int compare_subtasks(const void *a, const void *b) {
    const Subtask *subtaskA = (const Subtask *)a;
    const Subtask *subtaskB = (const Subtask *)b;
    return strcmp(subtaskA->title, subtaskB->title);
}

int compare_tags(const void *a, const void *b) {
    const char *tagA = *(const char **)a;
    const char *tagB = *(const char **)b;
    return strcmp(tagA, tagB);
}

int compare_by_priority(const void *a, const void *b) {
    const Task *taskA = (const Task *)a;
    const Task *taskB = (const Task *)b;
    return taskA->priority_level - taskB->priority_level; // Sort ascending by priority
}

int compare_by_due_date(const void *a, const void *b) {
    const Task *taskA = (const Task *)a;
    const Task *taskB = (const Task *)b;
    return strcmp(taskA->due_date, taskB->due_date); // Sort by due date
}

int compare_by_completion_status(const void *a, const void *b) {
    const Task *taskA = (const Task *)a;
    const Task *taskB = (const Task *)b;
    return taskA->is_done - taskB->is_done; // Sort by completion status (0 for not done, 1 for done)
}

void sort_task_list() { 
    mvprintw(27, 0, "Sort by: 'p' (priority), 'd' (due date), 'c' (completion status): ");
    refresh();

    char sort_choice = getch();
    switch (sort_choice) {
        case 'p':
            qsort(task_list, total_tasks, sizeof(Task), compare_by_priority);
            clear_message_area(); // Clear previous messages
            mvprintw(27, 0, "Tasks sorted by priority successfully!                                ");
            break;
        case 'd':
            qsort(task_list, total_tasks, sizeof(Task), compare_by_due_date);
            clear_message_area(); // Clear previous messages
            mvprintw(27, 0, "Tasks sorted by due date successfully!                           ");
            break;
        case 'c':
            qsort(task_list, total_tasks, sizeof(Task), compare_by_completion_status);
            clear_message_area(); // Clear previous messages
            mvprintw(27, 0, "Tasks sorted by completion status successfully!                           ");
            break;
        default:
            clear_message_area(); // Clear previous messages
            mvprintw(27, 0, "Invalid sort choice.                                 ");
            return;
    }

    refresh();
}

void edit_task_title() { 
    if (total_tasks == 0) {
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter the new task name: ");
    getnstr(task_list[current_task_index].title, 49); // TASK_NAME_LENGTH - 1
    noecho();
    curs_set(0);
    clear_message_area(); // Clear previous messages
    mvprintw(27, 0, "Task name updated successfully!");
    refresh();
}

void edit_task_details() { 
    if (total_tasks == 0) {
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter the new description: ");
    getnstr(task_list[current_task_index].details, 99);
    noecho();
    curs_set(0);
    clear_message_area(); // Clear previous messages
    mvprintw(27, 0, "Task description updated successfully!");
    refresh();
}

void update_due_date() { 
    if (total_tasks == 0) {
        clear_message_area(); // Clear previous messages
        mvprintw(27, 0, "No tasks available to edit.");
        refresh();
        return;
    }

    char new_due_date[11];
    echo();
    curs_set(1);
    do {
        mvprintw(27, 0, "Enter the new deadline (DD/MM/YYYY): ");
        getnstr(new_due_date, 10);
        if (!check_date_format(new_due_date)) {
            mvprintw(28, 0, "Invalid date format. Please try again.");
        }
    } while (!check_date_format(new_due_date));

    strncpy(task_list[current_task_index].due_date, new_due_date, 10);
    noecho();
    curs_set(0);
    clear_message_area(); // Clear previous messages
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
    clear_message_area(); // Clear previous messages
    mvprintw(27, 0, "Tasks saved to file successfully.");
    refresh();
}

void load_tasks_from_file(const char *filename) { 
    FILE *file = fopen(filename, "r"); 
    if (!file) {
        clear_message_area(); // Clear previous messages
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
        clear_message_area(); // Clear previous messages
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
            strncpy(task_list[total_tasks].title, name->valuestring, 49); // TASK_NAME_LENGTH - 1
            strncpy(task_list[total_tasks].details, description->valuestring, 99);
            strncpy(task_list[total_tasks].due_date, deadline->valuestring, 10);

            task_list[total_tasks].tag_count = 0;
            task_list[total_tasks].tags = NULL; // Initialize the dynamic array
            cJSON *category = NULL;
            cJSON_ArrayForEach(category, categories) {
                task_list[total_tasks].tags = realloc(task_list[total_tasks].tags, sizeof(char*) * (task_list[total_tasks].tag_count + 1));
                task_list[total_tasks].tags[task_list[total_tasks].tag_count] = malloc(strlen(category->valuestring) + 1);
                strcpy(task_list[total_tasks].tags[task_list[total_tasks].tag_count], category->valuestring);
                task_list[total_tasks].tag_count++;
            }

            task_list[total_tasks].sub_item_count = 0;
            cJSON *json_subtask = NULL;
            cJSON_ArrayForEach(json_subtask, subtasks) {
                cJSON *subtask_name = cJSON_GetObjectItem(json_subtask, "name");
                cJSON *subtask_status = cJSON_GetObjectItem(json_subtask, "status");

                if (subtask_name && subtask_status && task_list[total_tasks].sub_item_count < 50) { // MAX_SUBTASKS
                    strncpy(task_list[total_tasks].sub_items[task_list[total_tasks].sub_item_count].title, subtask_name->valuestring, 49); // SUBTASK_NAME_LENGTH - 1
                    task_list[total_tasks].sub_items[task_list[total_tasks].sub_item_count].is_done = strcmp(subtask_status->valuestring, "done") == 0;
                    task_list[total_tasks].sub_item_count++;
                }
            }

            total_tasks++;
        }
    }

    cJSON_Delete(json_root);
    free(json_string);
    clear_message_area(); // Clear previous messages
    mvprintw(27, 0, "Tasks loaded from file successfully.");
    refresh();
}

void handle_input() {
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
                show_task_metadata(); // Changed function name
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
                show_task_metadata(); // Changed function name
                break;
        }
        show_tasks();
        show_subtasks();
        show_task_metadata(); // Changed function name
    }
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    initialize_windows();

    handle_input(); // Call input handling function

    endwin();
    return 0;
}
