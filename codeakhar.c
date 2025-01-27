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
    char tags[10][30]; // MAX_CATEGORIES and CATEGORY_NAME_LENGTH
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

    mvprintw(25, 0, "Keys: 'q' to quit, 'a' to add task, 'j'/'k' to navigate, 'd' to delete, 'SPACE' to toggle status, 's' to sort, 'l' to point subtasks, 'h' to back task, 'e' to edit task's name, 'r' to edit task's description, 'n' to add new deadline, 'c' to edit categories, 'w' to save, 'x' to retrieve.");
    refresh();
}

int check_date_format(const char *date) {
    int day, month, year;
    if (sscanf(date, "%d/%d/%d", &day, &month, &year) != 3) {
        return 0; // فرمت نادرست
    }
    
    // بررسی مقادیر
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return 0;
    }

    // بررسی روزهای ماه
    if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) {
        return 0;
    }
    if (month == 2) {
        if ((year % 4 == 0 && day > 29) || (year % 4 != 0 && day > 28)) {
            return 0;
        }
    }

    return 1; // فرمت صحیح
}

void create_task() { 
    if (total_tasks >= 100) { // MAX_TASKS
        mvprintw(27, 0, "Task limit reached. Cannot add more tasks.");
        refresh();
        return;
    }

    char task_title[50]; // TASK_NAME_LENGTH
    char tag[30]; // CATEGORY_NAME_LENGTH
    char due_date[11];
    char details[100];
    int tag_count;
    int priority_level;

    echo(); 
    curs_set(1); 

    mvprintw(27, 0, "Enter task name: ");
    getnstr(task_title, 49); // TASK_NAME_LENGTH - 1 

    mvprintw(28, 0, "Enter number of categories (max %d): ", 10); // MAX_CATEGORIES
    scanw("%d", &tag_count); 

    Task *new_task = &task_list[total_tasks++];

    if (tag_count > 10) { // MAX_CATEGORIES
        new_task->tag_count = 10;
    } else {
        new_task->tag_count = tag_count;
    }

    for (int i = 0; i < new_task->tag_count; i++) {
        mvprintw(29 + i, 0, "Enter category %d: ", i + 1);
        getnstr(tag, 29); // CATEGORY_NAME_LENGTH - 1
        strncpy(new_task->tags[i], tag, 29);
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

    strncpy(new_task->title, task_title, 49); // TASK_NAME_LENGTH - 1
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
    if (current_task->sub_item_count >= 50) { // MAX_SUBTASKS
        mvprintw(27, 0, "Subtask limit reached for this task.     ");
        refresh();
        return;
    }

    char subtask_title[50]; // SUBTASK_NAME_LENGTH
    echo();
    curs_set(1);
    mvprintw(27, 0, "Enter subtask name: ");
    getnstr(subtask_title, 49); // SUBTASK_NAME_LENGTH - 1
    noecho();
    curs_set(0);

    Subtask *new_subtask = &current_task->sub_items[current_task->sub_item_count++];
    strncpy(new_subtask->title, subtask_title, 49); // SUBTASK_NAME_LENGTH - 1
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

void show_task_metadata() { // تغییر نام تابع
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

// ... سایر توابع بدون تغییر ...

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    initialize_windows();

    handle_input(); // فراخوانی تابع مدیریت ورودی

    endwin();
    return 0;
}
