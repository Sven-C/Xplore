#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>

#include "arguments.h"
#include "inject.h"

void prepare_child_process(char* cmd, int argc, char** args) {
    printf("Requesting to be stalked\n");
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == 1) {
        printf("Error: ptrace: %s\n", strerror(errno));
        exit(-1);
    }
    printf("Executing cmd");
    for (int i = 0; i < argc; i++) {
        printf(" %s", args[i]);
    }
    printf("\n");
    execve(cmd, args, NULL);
}

int main(int argc, char** argv) {
    int arg_index = 0;
    struct arguments arguments = {0};
    arguments.hook_filenames = malloc(sizeof(*arguments.hook_filenames) * argc);
    parse_args(argc, argv, &arguments, &arg_index);

    pid_t child_pid = fork();
    if (child_pid == 0) {
        prepare_child_process(arguments.executable, argc - arg_index + 1, &argv[arg_index - 1]);
    } else {
        perform_hooks_in_child(child_pid, &arguments);
        free(arguments.hook_filenames);
        arguments.hook_filenames = NULL;
        exit(0);
    }
}
