#include "process.h"
#include "../config.h"
#include "memory/memory.h"
#include "../status.h"
#include "task.h"
#include "../fs/file.h"
#include "../string/string.h"
#include "../memory/heap/kernel_heap.h"
#include "../kernel.h"
#include "../memory/paging/paging.h"

struct process *current_process = 0;
static struct process *processes[SIMPOS_MAX_PROCESSES] = {};

static void process_init(struct process *process)
{
    memset(process, 0, sizeof(struct process));
}

struct process *process_current()
{
    return current_process;
}

struct process *process_get(int process_id)
{
    /* ensures that the process id is in boundaries */
    if (process_id < 0 || process_id >= SIMPOS_MAX_PROCESSES)
    {
        return NULL;
    }

    return processes[process_id];
}

/* loads the binary file into memory */
static int process_load_binary(const char *filename, struct process *process)
{
    int res = 0;

    int fd = fopen(filename, "r");
    if (!fd)
    {
        return -EIO;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res != SIMPOS_ALL_OK)
    {
        return res;
    }

    void *program_data_ptr = kzalloc(stat.file_size);
    if (!program_data_ptr)
    {
        return -ENOMEM;
    }

    if (fread(program_data_ptr, stat.file_size, 1, fd) != 1)
    {
        return -EIO;
    }

    process->ptr = program_data_ptr;
    process->size = stat.file_size;

    fclose(fd);
    return res;
}

static int process_load_data(const char *filename, struct process *process)
{
    int res = 0;
    res = process_load_binary(filename, process);
    return res;
}

int process_map_binary(struct process *process)
{
    int res = 0;
    res = paging_map_to(process->task->page_directory, (void *)SIMPOS_PROGRAM_VIRTUAL_ADDRESS, process->ptr, paging_align_address(process->ptr + process->size), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);
    return res;
}

int process_map_memory(struct process *process)
{
    int res = 0;
    res = process_map_binary(process);
    return res;
}

/* finds a free slot from the processes array */
int process_get_free_slot()
{
    for (int i = 0; i < SIMPOS_MAX_PROCESSES; i++)
    {
        if (processes[i] == 0)
        {
            return i;
        }
    }

    return -EISTKN;
}

/* find a free slot for the process and loads to the process there */
int process_load(const char *filename, struct process **process)
{
    int process_slot = process_get_free_slot();
    if (process_slot < 0)
    {
        return -EISTKN;
    }

    return process_load_for_slot(filename, process, process_slot);
}

/* load process for the given slot */
int process_load_for_slot(const char *filename, struct process **process, int process_slot)
{
    int res = 0;
    struct task *task = 0;
    struct process *_process;
    void *program_stack_ptr = 0;

    if (process_get(process_slot) != 0)
    {
        res = EISTKN;
        goto out;
    }

    _process = kzalloc(sizeof(struct process));
    if (!_process)
    {
        res = -ENOMEM;
        goto out;
    }

    process_init(_process);
    res = process_load_data(filename, _process);
    if (res < 0)
    {
        goto out;
    }

    /* allocate stack memory for the program */
    program_stack_ptr = kzalloc(SIMPOS_USER_PROGRAM_STACK_SIZE);
    if (!program_stack_ptr)
    {
        res = -ENOMEM;
        goto out;
    }

    strncpy(_process->filename, filename, sizeof(_process->filename));

    _process->stack = program_stack_ptr;
    _process->id = process_slot;

    /* create a task */
    task = task_new(_process);

    if (ERROR_I(task) == 0)
    {
        res = ERROR_I(task);
        goto out;
    }

    _process->task = task;

    res = process_map_memory(_process);
    if (res < 0)
    {
        goto out;
    }

    *process = _process;

    /* add the process to the array */
    process[process_slot] = _process;
out:
    if (ISERR(res))
    {
        if (_process && _process->task)
        {
            task_free(_process->task);
        }

        /* FREE THE PROCESS DATA */
    }

    return res;
}