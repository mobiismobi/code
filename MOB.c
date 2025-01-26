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
    char title[SUBTASK_NAME_LENGTH];
    int is_done; // وضعیت انجام شده یا نشده 0 1
} Subtask;

typedef struct {
    char title[TASK_NAME_LENGTH];
    int is_done; // وضعیت انجام شده یا نشده 0 1
    int priority_level;
    char due_date[11];
    char details[100];
    char tags[MAX_CATEGORIES][CATEGORY_NAME_LENGTH];
    int tag_count;
    Subtask sub_items[MAX_SUBTASKS];
    int sub_item_count;
} Task;

Task task_list[MAX_TASKS];
int total_tasks = 0; // تعداد تسک موجود
int current_task_index = 0; // ایندکس تسکی که در حال حاضر انتخاب شده است
int current_subtask_index = 0;
int is_subtask_mode = 0; //درحال ویرایش سابتسک هستیم یا نه 0 1
int current_category_index = 0;

WINDOW *task_window, *subtask_window, *category_window, *deadline_window, *description_window;

void initialize_windows() {  //تابع رسم پنجره ها
    clear();
    refresh();

    task_window = newwin(15, 40, 0, 0);
    box(task_window, 0, 0);
    mvwprintw(task_window, 0, 2, "Tasks");
    wrefresh(task_window);

    subtask_window = newwin(15, 40, 0, 40);
    box(subtask_window, 0, 0);
    mvwprintw(subtask_window, 0, 2, "Subtasks");
    wrefresh(subtask_window);

    category_window = newwin(5, 40, 15, 0);
    box(category_window, 0, 0);
    mvwprintw(category_window, 0, 2, "Categories");
    wrefresh(category_window);

    deadline_window = newwin(5, 40, 15, 40);
    box(deadline_window, 0, 0);
    mvwprintw(deadline_window, 0, 2, "Deadline");
    wrefresh(deadline_window);

    description_window = newwin(5, 80, 20, 0);
    box(description_window, 0, 0);
    mvwprintw(description_window, 0, 2, "Description");
    wrefresh(description_window);

    mvprintw(25, 0, "Keys: 'q' to quit, 'a' to add task, 'j'/'k' to navigate, 'd' to delete, 'SPACE' to toggle status, 's' to sort, 'l' to point subtasks, 'h' to back task, 'e' to edit task's name, 'r' to edit task's desciption, 'n' to add new deadline, 'c' to edit categories, 'w' to save, 'x' to retrive.");
    refresh();
}

int check_date_format(const char *date) { //تابع بررسی فزمت تاریخ
    if (strlen(date) != 10) return 0; 

    // بررسی فرمت DD/MM/YYYY
    if (!isdigit(date[0]) || !isdigit(date[1]) || date[2] != '/' ||
        !isdigit(date[3]) || !isdigit(date[4]) || date[5] != '/' ||
        !isdigit(date[6]) || !isdigit(date[7]) ||
        !isdigit(date[8]) || !isdigit(date[9])) {
        return 0; // فرمت نادرست است
    }

    // تبدیل رشته به عدد
    int day = (date[0] - '0') * 10 + (date[1] - '0');
    int month = (date[3] - '0') * 10 + (date[4] - '0');
    int year = (date[6] - '0') * 1000 + (date[7] - '0') * 100 +
               (date[8] - '0') * 10 + (date[9] - '0');

    if (month < 1 || month > 12 || year < 1) {
        return 0;
    }

    int days_in_month[] = {31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29};

    // بررسی سال کبیسه شمسی
    int leap_years[] = {1, 5, 9, 13, 17, 22, 26, 30}; // سال‌های کبیسه در یک دوره‌ی ۳۳ ساله
    for (int i = 0; i < 8; i++) {
        if (year % 33 == leap_years[i]) {
            days_in_month[11] = 30; 
            break;
        }
    }

    // بررسی تعداد روز معتبر در ماه
    if (day < 1 || day > days_in_month[month - 1]) {
        return 0;
    }

    return 1; 
}

void create_task() { //گرفتن یه تسک با کلید a
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

    echo(); //نمایش ورودی برای کاربر
    curs_set(1); //نمایش مکانما برای کاربر

    mvprintw(27, 0, "Enter task name: ");
    getnstr(task_title, TASK_NAME_LENGTH - 1); //گرفتن ورودی رشته از کاربر

    mvprintw(28, 0, "Enter number of categories (max %d): ", MAX_CATEGORIES);
    scanw("%d", &tag_count); //گرفتن ورودی عدد

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
    if (priority_level < 1 || priority_level > 9) priority_level = 1; //پیشفرض

    noecho(); //غیرفعال کردن نمایش ورودی
    curs_set(0); //پنهان کردن مکانما

    strncpy(new_task->title, task_title, TASK_NAME_LENGTH - 1);
    strncpy(new_task->due_date, due_date, 10);
    strncpy(new_task->details, details, 99);
    new_task->is_done = 0; //پیشفرض انجام نشده 
    new_task->priority_level = priority_level;
    new_task->sub_item_count = 0;

    mvprintw(27, 0, "                             Task added successfully!                             ");
    refresh();
}

void create_subtask() { // اضافه کردن سایتسک
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
    new_subtask->is_done = 0; // پیشفرض انجام نشده

    mvprintw(27, 0, "Subtask added successfully!             ");
    refresh();
}

void remove_task() { //حذف تسک
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

void remove_subtask() { //حذف سابتسک 
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

void toggle_task_completion() { // تغییر وضعیت انجام نشده به شده
    if (total_tasks == 0) {
        mvprintw(27, 0, "No tasks available to toggle a subtask. ");
        refresh();
        return;
    }

    if (total_tasks > 0) {
        task_list[current_task_index].is_done = !task_list[current_task_index].is_done; //تغییر وضعیت انجام شده و نشده 
    } 

    mvprintw(27, 0, "Subtask status toggled successfully!     ");
    refresh();
}

void toggle_subtask_completion() { // تغییر وضعیت انجام نشده به شده
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
    current_subtask->is_done = !current_subtask->is_done; //اگه 0 باشه میشه 1 اگه 1 باشه میشه 0

    mvprintw(27, 0, "Subtask status toggled successfully!     ");
    refresh();
}

void show_tasks() { //نمایش تسک ها
    werase(task_window);
    box(task_window, 0, 0);
    mvwprintw(task_window, 0, 2, "Tasks");

    start_color(); // فعال کردن رنگ‌ها
    init_pair(2, COLOR_BLACK, COLOR_GREEN); //رنگ هایلایت

    for (int i = 0; i < total_tasks; i++) {
        if (i == current_task_index && !is_subtask_mode) {
            wattron(task_window, COLOR_PAIR(2)); //هایلایت
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

    start_color(); // فعال کردن رنگ‌ها
    init_pair(2, COLOR_BLACK, COLOR_GREEN);

    if (total_tasks == 0) {
        mvwprintw(subtask_window, 1, 2, "No tasks available.");
    } else {
        Task *current_task = &task_list[current_task_index];
        for (int i = 0; i < current_task->sub_item_count; i++) {
            if (i == current_subtask_index && is_subtask_mode) {
                wattron(subtask_window, COLOR_PAIR(2));
            }
            mvwprintw(subtask_window, i + 1, 2, "%d. [%c] %s",
                      i+1,
                      current_task->sub_items[i].is_done ? 'x' : ' ', //نمایش وضعیت انجام شده یا نشده
                      current_task->sub_items[i].title);
            if (i == current_subtask_index && is_subtask_mode) {
                wattroff(subtask_window, COLOR_PAIR(2));
            }
        }
    }

    wrefresh(subtask_window);
}

void show_metadata() { // نمایش اظلاعات ریز تسکها
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
    init_pair(2, COLOR_BLACK, COLOR_GREEN);

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

void manage_tags() { //مدیریت دسته بندی ها
    static int is_tag_mode = 0; // چک میکنه تو حالت دسته بندی هست یا نه 0 نیس 1 هس
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
            case 'a': // اضافه کردن دسته بندی جدید
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

            case 'd': // حذف دسته بندی
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

            case 'j': // پایین اومدن
                if (current_category_index < task_list[current_task_index].tag_count - 1) {
                    current_category_index++;
                }
                break;

            case 'k': // بالا رفتن
                if (current_category_index > 0) {
                    current_category_index--;
                }
                break;

            case 'c': // خارج شدن از حالت دسته بندی
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

void sort_tasks_by_title(Task *task_list, int n) { // مرتب‌سازی بر اساس نام
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (strcmp(task_list[i].title, task_list[j].title) > 0) {
                swap_tasks(&task_list[i], &task_list[j]);
            }
        }
    }
}

void sort_tasks_by_due_date(Task *task_list, int n) { // مرتب‌سازی بر اساس ددلاین
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            int day_a, month_a, year_a;
            int day_b, month_b, year_b;

            sscanf(task_list[i].due_date, "%d/%d/%d", &day_a, &month_a, &year_a);
            sscanf(task_list[j].due_date, "%d/%d/%d", &day_b, &month_b, &year_b);

            if (year_a != year_b) {
                if (year_a > year_b) {
                    swap_tasks(&task_list[i], &task_list[j]);
                }
            }
            else if (month_a != month_b) {
                if (month_a > month_b) {
                    swap_tasks(&task_list[i], &task_list[j]);
                }
            }
            else if (day_a != day_b) {
                if (day_a > day_b) {
                    swap_tasks(&task_list[i], &task_list[j]);
                }
            }
        }
    }
}


void sort_tasks_by_priority_level(Task *task_list, int n) { // مرتب سازی بر اساس اولویت
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (task_list[i].priority_level > task_list[j].priority_level) {
                swap_tasks(&task_list[i], &task_list[j]);
            }
        }
    }
}

void sort_task_list() { // تابع مرتب سازی
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

void edit_task_title() { //ادیت اسم تسک ها
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

void edit_task_details() { // ادیت توضیحات تسک ها
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

void update_due_date() { // ادیت ددلاین تسک ها
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

void save_tasks_to_file(const char *filename) { //ذخیره اطلاعات در یک فایل

    cJSON *json_root = cJSON_CreateArray(); // ساخت یک ارایه جیسون برای ذخیره اطلاعات

    for (int i = 0; i < total_tasks; i++) {
        cJSON *json_task = cJSON_CreateObject(); // هر عضو تسک ها
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
            cJSON_AddStringToObject(json_subtask, "status", task_list[i].sub_items[j].is_done ? "done" : "pending"); //چک کردن وضعیت T  F
            cJSON_AddItemToArray(json_subtasks, json_subtask);
        }
        cJSON_AddItemToObject(json_task, "subtasks", json_subtasks);

        cJSON_AddItemToArray(json_root, json_task);
    }

    char *json_string = cJSON_Print(json_root); //تبدیل به فرمت متنی
    FILE *file = fopen(filename, "w"); // باز کردن یک فایل
    if (file) {
        fputs(json_string, file);
        fclose(file);
    }

    cJSON_Delete(json_root);
    free(json_string);
    mvprintw(27, 0, "Tasks saved to file.");
    refresh();
}

void load_tasks_from_file(const char *filename) { // بازیابی اطلاعات موجود در یک فایل

    FILE *file = fopen(filename, "r"); //خواندن یک فایل
    if (!file) {
        mvprintw(27, 0, "No file found to load tasks.");
        refresh();
        return;
    }
    // خواندن محتوای یک فایل
    fseek(file, 0, SEEK_END); //انتهای فایل
    long length = ftell(file);
    fseek(file, 0, SEEK_SET); //ابتدای فایل
    char *json_string = malloc(length + 1); //ایجاد فضایی برای محتوای فایل
    fread(json_string, 1, length, file);
    fclose(file);
    json_string[length] = '\0';

    cJSON *json_root = cJSON_Parse(json_string); //تبدیل رشته به یک شی جیسون
    if (!json_root) {
        mvprintw(27, 0, "Failed to parse tasks from file.");
        free(json_string);
        refresh();
        return;
    }

    total_tasks = 0;

    cJSON *json_task = NULL;
    cJSON_ArrayForEach(json_task, json_root) { //بارگذاری تسک ها
        cJSON *name = cJSON_GetObjectItem(json_task, "name");
        cJSON *priority = cJSON_GetObjectItem(json_task, "priority");
        cJSON *description = cJSON_GetObjectItem(json_task, "description");
        cJSON *deadline = cJSON_GetObjectItem(json_task, "deadline");
        cJSON *categories = cJSON_GetObjectItem(json_task, "categories");
        cJSON *subtasks = cJSON_GetObjectItem(json_task, "subtasks");

        if (priority && name && description && deadline && categories && subtasks) { //فیلدها موجود باشند !NULL 
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
            case 'a': // اضافه کردن
                if (is_subtask_mode) {
                    create_subtask();
                } else {
                    create_task();
                }
                break;
            case 'd': // حذف
                if (is_subtask_mode) {
                    remove_subtask();
                } else {
                    remove_task();
                }
                break;
            case 'j': // حرکت به پایین
                if (is_subtask_mode) {
                    if (current_subtask_index < task_list[current_task_index].sub_item_count - 1) {
                        current_subtask_index++;
                    }
                } else if (current_task_index < total_tasks - 1) {
                    current_task_index++;
                }
                break;
            case 'k': // حرکت به بالا
                if (is_subtask_mode) {
                    if (current_subtask_index > 0) {
                        current_subtask_index--;
                    }
                } else if (current_task_index > 0) {
                    current_task_index--;
                }
                break;
            case 'l': // ورود به حالت مدیریت سابتسک
                if (!is_subtask_mode && total_tasks > 0) {
                    is_subtask_mode = 1;
                    current_subtask_index = 0;
                }
                break;
            case 'h': // خروج از حالت مدیریت سابتسک
                if (is_subtask_mode) {
                    is_subtask_mode = 0;
                }
                break;
            case ' ': // تغییر وضعیت
                if (is_subtask_mode) {
                    toggle_subtask_completion();
                } else {
                    toggle_task_completion();
                }
                break;
            case 's':  // برای مرتب‌سازی
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
            case 'w': // سیو تسک ها
                save_tasks_to_file("tasks.json");
                break;
            case 'x': // بازیابی تسک ها
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
