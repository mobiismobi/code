#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ncurses.h>
#include <cjson/cJSON.h>

// Subtask structure
typedef struct SubTask {
    int id;
    char title[100];
    bool is_done;
    struct SubTask *next;
} SubTask;

// Task structure
typedef struct Task {
    int id;
    char title[100];
    char note[200];
    char categories[5][50];
    int priority;
    bool is_done;
    time_t deadline;
    SubTask *subtasks;
    struct Task *next;
} Task;

// Task Manager structure
typedef struct {
    Task *head;
    int task_count;
} TaskManager;

void init_task_manager(TaskManager *manager) {
    manager->head = NULL;
    manager->task_count = 0;
}

Task *add_task(TaskManager *manager, const char *title, int priority, time_t deadline) {
    Task *new_task = (Task *)malloc(sizeof(Task));
    if (!new_task) return NULL;

    new_task->id = ++manager->task_count;
    strncpy(new_task->title, title, sizeof(new_task->title) - 1);
    new_task->priority = priority;
    new_task->deadline = deadline;
    new_task->is_done = false;
    new_task->subtasks = NULL;
    new_task->next = manager->head;
    manager->head = new_task;

    return new_task;
}

bool delete_task(TaskManager *manager, int id) {
    Task *prev = NULL, *current = manager->head;

    while (current) {
        if (current->id == id) {
            if (prev) prev->next = current->next;
            else manager->head = current->next;

            SubTask *sub = current->subtasks;
            while (sub) {
                SubTask *temp = sub;
                sub = sub->next;
                free(temp);
            }

            free(current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

void display_tasks(const TaskManager *manager) {
    Task *current = manager->head;
    while (current) {
        printf("[%c] %d. %s (Priority: %d, Deadline: %s)\n",
               current->is_done ? 'x' : ' ',
               current->id,
               current->title,
               current->priority,
               ctime(&current->deadline));
        current = current->next;
    }
}

void free_tasks(TaskManager *manager) {
    Task *current = manager->head;
    while (current) {
        Task *temp = current;
        current = current->next;

        SubTask *sub = temp->subtasks;
        while (sub) {
            SubTask *temp_sub = sub;
            sub = sub->next;
            free(temp_sub);
        }

        free(temp);
    }
    manager->head = NULL;
    manager->task_count = 0;
}

SubTask *add_subtask(Task *task, const char *title) {
    SubTask *new_subtask = (SubTask *)malloc(sizeof(SubTask));
    if (!new_subtask) return NULL;

    new_subtask->id = task->subtasks ? task->subtasks->id + 1 : 1;
    strncpy(new_subtask->title, title, sizeof(new_subtask->title) - 1);
    new_subtask->is_done = false;
    new_subtask->next = task->subtasks;
    task->subtasks = new_subtask;

    return new_subtask;
}

bool delete_subtask(Task *task, int subtask_id) {
    SubTask *prev = NULL, *current = task->subtasks;

    while (current) {
        if (current->id == subtask_id) {
            if (prev) prev->next = current->next;
            else task->subtasks = current->next;

            free(current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

void display_subtasks(const Task *task) {
    SubTask *current = task->subtasks;
    while (current) {
        printf("    [%c] %d. %s\n",
               current->is_done ? 'x' : ' ',
               current->id,
               current->title);
        current = current->next;
    }
}

bool save_tasks_to_file(const TaskManager *manager, const char *filename) {
    cJSON *root = cJSON_CreateArray();

    Task *current = manager->head;
    while (current) {
        cJSON *task_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(task_json, "id", current->id);
        cJSON_AddStringToObject(task_json, "title", current->title);
        cJSON_AddStringToObject(task_json, "note", current->note);
        cJSON_AddNumberToObject(task_json, "priority", current->priority);
        cJSON_AddBoolToObject(task_json, "is_done", current->is_done);
        cJSON_AddNumberToObject(task_json, "deadline", current->deadline);

        cJSON *subtasks_json = cJSON_CreateArray();
        SubTask *subtask = current->subtasks;
        while (subtask) {
            cJSON *subtask_json = cJSON_CreateObject();
            cJSON_AddNumberToObject(subtask_json, "id", subtask->id);
            cJSON_AddStringToObject(subtask_json, "title", subtask->title);
            cJSON_AddBoolToObject(subtask_json, "is_done", subtask->is_done);
            cJSON_AddItemToArray(subtasks_json, subtask_json);
            subtask = subtask->next;
        }

        cJSON_AddItemToObject(task_json, "subtasks", subtasks_json);
        cJSON_AddItemToArray(root, task_json);
        current = current->next;
    }

    char *json_string = cJSON_Print(root);
    FILE *file = fopen(filename, "w");
    if (!file) {
        cJSON_Delete(root);
        return false;
    }
    fprintf(file, "%s", json_string);
    fclose(file);

    cJSON_Delete(root);
    free(json_string);
    return true;
}

bool load_tasks_from_file(TaskManager *manager, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return false;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = (char *)malloc(length + 1);
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    cJSON *root = cJSON_Parse(data);
    free(data);
    if (!root) return false;

    cJSON *task_json;
    cJSON_ArrayForEach(task_json, root) {
        const char *title = cJSON_GetObjectItem(task_json, "title")->valuestring;
        int priority = cJSON_GetObjectItem(task_json, "priority")->valueint;
        time_t deadline = cJSON_GetObjectItem(task_json, "deadline")->valuedouble;

        Task *task = add_task(manager, title, priority, deadline);
        task->is_done = cJSON_GetObjectItem(task_json, "is_done")->valueint;

        cJSON *subtasks_json = cJSON_GetObjectItem(task_json, "subtasks");
        cJSON *subtask_json;
        cJSON_ArrayForEach(subtask_json, subtasks_json) {
            const char *sub_title = cJSON_GetObjectItem(subtask_json, "title")->valuestring;
            SubTask *subtask = add_subtask(task, sub_title);
            subtask->is_done = cJSON_GetObjectItem(subtask_json, "is_done")->valueint;
        }
    }

    cJSON_Delete(root);
    return true;
}

void init_ui() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
}

void cleanup_ui() {
    endwin();
}

void display_main_menu(const TaskManager *manager) {
    clear();
    mvprintw(0, 0, "TUI To-Do List Manager");
    mvprintw(2, 0, "Commands:");
    mvprintw(3, 0, "  a: Add Task");
    mvprintw(4, 0, "  d: Delete Task");
    mvprintw(5, 0, "  w: Save Tasks");
    mvprintw(6, 0, "  l: Load Tasks");
    mvprintw(7, 0, "  q: Quit");

    mvprintw(9, 0, "Tasks:");
    Task *current = manager->head;
    int y = 10;
    while (current) {
        mvprintw(y++, 2, "[%c] %d. %s (Priority: %d)",
                 current->is_done ? 'x' : ' ',
                 current->id,
                 current->title,
                 current->priority);
        current = current->next;
    }
    refresh();
}

void display_task_details(const Task *task) {
    clear();
    mvprintw(0, 0, "Task Details:");
    mvprintw(2, 0, "Title: %s", task->title);
    mvprintw(3, 0, "Priority: %d", task->priority);
    mvprintw(4, 0, "Note: %s", task->note);
    mvprintw(5, 0, "Deadline: %s", ctime(&task->deadline));

    mvprintw(7, 0, "Subtasks:");
    SubTask *current = task->subtasks;
    int y = 8;
    while (current) {
        mvprintw(y++, 2, "[%c] %d. %s",
                 current->is_done ? 'x' : ' ',
                 current->id,
                 current->title);
        current = current->next;
    }
    refresh();
}

int main() {
    TaskManager manager;
    init_task_manager(&manager);

    init_ui();
    display_main_menu(&manager);

    int ch;
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case 'a': {
                time_t now = time(NULL);
                add_task(&manager, "New Task", 5, now);
                break;
            }
            case 'd': {
                delete_task(&manager, 1); // Example: Delete first task
                break;
            }
            case 'w': {
                save_tasks_to_file(&manager, "tasks.json");
                break;
            }
            case 'l': {
                load_tasks_from_file(&manager, "tasks.json");
                break;
            }
        }
        display_main_menu(&manager);
    }

    cleanup_ui();
    free_tasks(&manager);
    return 0;
}
