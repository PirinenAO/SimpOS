#include "idt.h"
#include "config.h"
#include "memory/memory.h"
#include "../io/io.h"
#include "../terminal/terminal.h"
#include "../task/task.h"
#include "../kernel.h"

/* defined structures */
struct idt_desc idt_descriptors[SIMPOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

static ISR80_COMMAND isr80h_commands[SIMPOS_MAX_ISR80H_COMMANDS];

/* external assembly functions */
extern void idt_load(struct idtr_desc *ptr);
extern void int21h();
extern void no_interrupt();
extern void isr80h_wrapper();

/* interrupt 21h handler: handles the keyboard interrupt */
void int21h_handler()
{
    print("keyboard pressed\n");
    outb(0x20, 0x20); // acknowledge the interrupt
}

/* default handler which doesn't have any functionality */
void no_interrupt_handler()
{
    outb(0x20, 0x20); // acknowledge the interrupt
}

/* function for setting up interrupt service routines */
void idt_set(int interrupt_no, void *address)
{
    struct idt_desc *desc = &idt_descriptors[interrupt_no];
    desc->offset_1 = (uint32_t)address & 0x0000ffff; // zero out the upper bits
    desc->selector = KERNEL_CODE_SELECTOR;
    desc->zero = 0x00;
    desc->type_attr = 0xEE;
    desc->offset_2 = (uint32_t)address >> 16; // shift the upper 16 bits into the lower 16 bits
}

/* initializes IDT and IDTR  */
void idt_init()
{
    memset(idt_descriptors, 0, sizeof(idt_descriptors)); // sets all the IDT entries to zero
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1; // sets the IDT size
    idtr_descriptor.base = (uint32_t)idt_descriptors;    // sets the starting address of the IDT

    /* initialize every IDT entry with default handler */
    for (int i = 0; i < SIMPOS_TOTAL_INTERRUPTS; i++)
    {
        idt_set(i, no_interrupt);
    }

    /* create ISR entries */
    idt_set(0x21, int21h);
    idt_set(0x80, isr80h_wrapper);

    /* loads the IDT into IDTR */
    idt_load(&idtr_descriptor);
}

void isr80h_register_command(int command_id, ISR80_COMMAND command)
{
    if (command_id <= 0 || command_id >= SIMPOS_MAX_ISR80H_COMMANDS)
    {
        kernel_panic("Command out of bounds!\n");
    }

    if (isr80h_commands[command_id])
    {
        kernel_panic("You're attempting to overwrite an existing command!\n");
    }

    isr80h_commands[command_id] = command;
}

void *isr80h_handle_command(int command, struct interrupt_frame *frame)
{
    void *result = 0;

    /* if invalid command */
    if (command <= 0 || command >= SIMPOS_MAX_ISR80H_COMMANDS)
    {
        return 0;
    }

    ISR80_COMMAND command_func = isr80h_commands[command];
    /* if command doesn't exist */
    if (!command_func)
    {
        return 0;
    }

    result = command_func(frame);
    return result;
}

void *isr80h_handler(int command, struct interrupt_frame *frame)
{
    void *res = 0;
    kernel_page();
    task_current_save_state(frame); // saves the registers
    res = isr80h_handle_command(command, frame);
    task_page();
    return res;
}