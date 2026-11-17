/*
 * The base exception class and the uncaught-exception path.
 *
 * Exception::Exception stores a const char * in the first field. The
 * names match the symbols the compiler emits for those two constructors.
 * smallcpp_note_throw records the object passed to __cxa_throw. If that
 * throw is not caught, the terminate handler prints the message and a
 * call stack, then aborts.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Exception {
    const char *message;
};

void exception_blank(struct Exception *self)
    __asm__("\"Exception::Exception(class.Exception*)\"");

void exception_text(struct Exception *self, const char *text)
    __asm__("\"Exception::Exception(class.Exception*,i8s*)\"");

void exception_blank(struct Exception *self)
{
    if (self != NULL)
        self->message = NULL;
}

void exception_text(struct Exception *self, const char *text)
{
    if (self != NULL)
        self->message = text;
}

static void *noted_object;

typedef void (*TerminateFn)(void);

TerminateFn install_terminate(TerminateFn next) __asm__("_ZSt13set_terminatePFvvE");

void *__cxa_current_exception_type(void);

void smallcpp_print_stack(FILE *out);

static const char *type_name(void *info)
{
    const char *name;

    if (info == NULL)
        return NULL;
    name = *(const char *const *)((const char *)info + sizeof(void *));
    if (name == NULL || name[0] == '\0')
        return NULL;
    return name;
}

static const char *noted_message(void)
{
    const char *name = type_name(__cxa_current_exception_type());
    struct Exception *object;

    if (name == NULL || noted_object == NULL)
        return NULL;
    if (strcmp(name, "class.Exception") == 0)
        return *(const char *const *)noted_object;
    if (strcmp(name, "class.Exception*") != 0)
        return NULL;
    object = *(struct Exception **)noted_object;
    if (object == NULL)
        return NULL;
    return object->message;
}

static void uncaught(void)
{
    const char *message = noted_message();

    if (message != NULL && message[0] != '\0')
        fprintf(stderr, "uncaught exception: %s\n", message);
    else
        fprintf(stderr, "uncaught exception\n");
    smallcpp_print_stack(stderr);
    fflush(stderr);
    abort();
}

void smallcpp_uncaught_init(void)
{
    install_terminate(uncaught);
}

void smallcpp_note_throw(void *object)
{
    noted_object = object;
    smallcpp_uncaught_init();
}

__attribute__((constructor)) static void install_uncaught(void)
{
    smallcpp_uncaught_init();
}
