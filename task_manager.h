#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdbool.h>
#include <cjson/cJSON.h>

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

#endif
